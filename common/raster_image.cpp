/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <raster_image.h>

#include <cmath>
#include <csetjmp>
#include <cstdio>
#include <cstring>

#include <png.h>
#include <jpeglib.h>

#include <wx/ffile.h>


struct RASTER_IMAGE::DATA
{
    int                        width = 0;
    int                        height = 0;
    TYPE                       type = TYPE::INVALID;
    std::vector<unsigned char> rgb;
    std::vector<unsigned char> alpha;       ///< Empty: no alpha channel
    bool                       hasMask = false;
    unsigned char              maskRed = 0;
    unsigned char              maskGreen = 0;
    unsigned char              maskBlue = 0;
    double                     dpiX = 0.0;
    double                     dpiY = 0.0;
};


static RASTER_IMAGE::DECODER& fallbackDecoder()
{
    static RASTER_IMAGE::DECODER decoder;
    return decoder;
}


void RASTER_IMAGE::SetFallbackDecoder( DECODER aDecoder )
{
    fallbackDecoder() = std::move( aDecoder );
}


RASTER_IMAGE::RASTER_IMAGE()
{
}


RASTER_IMAGE::RASTER_IMAGE( int aWidth, int aHeight ) :
        m_data( std::make_shared<DATA>() )
{
    m_data->width = std::max( aWidth, 0 );
    m_data->height = std::max( aHeight, 0 );
    m_data->rgb.assign( (size_t) m_data->width * m_data->height * 3, 0 );
}


bool RASTER_IMAGE::IsOk() const
{
    return m_data && m_data->width > 0 && m_data->height > 0;
}


int RASTER_IMAGE::GetWidth() const
{
    return m_data ? m_data->width : 0;
}


int RASTER_IMAGE::GetHeight() const
{
    return m_data ? m_data->height : 0;
}


RASTER_IMAGE::TYPE RASTER_IMAGE::GetType() const
{
    return m_data ? m_data->type : TYPE::INVALID;
}


void RASTER_IMAGE::SetType( TYPE aType )
{
    if( m_data )
    {
        unshare();
        m_data->type = aType;
    }
}


void RASTER_IMAGE::unshare()
{
    if( m_data && m_data.use_count() > 1 )
        m_data = std::make_shared<DATA>( *m_data );
}


unsigned char* RASTER_IMAGE::GetData()
{
    if( !m_data )
        return nullptr;

    unshare();
    return m_data->rgb.data();
}


const unsigned char* RASTER_IMAGE::GetData() const
{
    return m_data ? m_data->rgb.data() : nullptr;
}


unsigned char RASTER_IMAGE::GetRed( int aX, int aY ) const
{
    return m_data->rgb[( (size_t) aY * m_data->width + aX ) * 3];
}


unsigned char RASTER_IMAGE::GetGreen( int aX, int aY ) const
{
    return m_data->rgb[( (size_t) aY * m_data->width + aX ) * 3 + 1];
}


unsigned char RASTER_IMAGE::GetBlue( int aX, int aY ) const
{
    return m_data->rgb[( (size_t) aY * m_data->width + aX ) * 3 + 2];
}


void RASTER_IMAGE::SetRGB( int aX, int aY, unsigned char aRed, unsigned char aGreen,
                           unsigned char aBlue )
{
    unshare();

    unsigned char* p = &m_data->rgb[( (size_t) aY * m_data->width + aX ) * 3];
    p[0] = aRed;
    p[1] = aGreen;
    p[2] = aBlue;
}


bool RASTER_IMAGE::HasAlpha() const
{
    return m_data && !m_data->alpha.empty();
}


void RASTER_IMAGE::InitAlpha()
{
    if( !m_data || HasAlpha() )
        return;

    unshare();
    m_data->alpha.assign( (size_t) m_data->width * m_data->height, 255 );

    // A mask becomes transparent pixels
    if( m_data->hasMask )
    {
        for( size_t i = 0; i < m_data->alpha.size(); ++i )
        {
            const unsigned char* p = &m_data->rgb[i * 3];

            if( p[0] == m_data->maskRed && p[1] == m_data->maskGreen && p[2] == m_data->maskBlue )
                m_data->alpha[i] = 0;
        }

        m_data->hasMask = false;
    }
}


unsigned char* RASTER_IMAGE::GetAlpha()
{
    if( !HasAlpha() )
        return nullptr;

    unshare();
    return m_data->alpha.data();
}


const unsigned char* RASTER_IMAGE::GetAlpha() const
{
    return HasAlpha() ? m_data->alpha.data() : nullptr;
}


unsigned char RASTER_IMAGE::GetAlpha( int aX, int aY ) const
{
    return HasAlpha() ? m_data->alpha[(size_t) aY * m_data->width + aX] : 255;
}


void RASTER_IMAGE::SetAlpha( int aX, int aY, unsigned char aAlpha )
{
    if( !HasAlpha() )
        InitAlpha();

    unshare();
    m_data->alpha[(size_t) aY * m_data->width + aX] = aAlpha;
}


bool RASTER_IMAGE::HasMask() const
{
    return m_data && m_data->hasMask;
}


void RASTER_IMAGE::SetMaskColour( unsigned char aRed, unsigned char aGreen, unsigned char aBlue )
{
    if( !m_data )
        return;

    unshare();
    m_data->hasMask = true;
    m_data->maskRed = aRed;
    m_data->maskGreen = aGreen;
    m_data->maskBlue = aBlue;
}


void RASTER_IMAGE::RemoveMask()
{
    if( m_data && m_data->hasMask )
    {
        unshare();
        m_data->hasMask = false;
    }
}


unsigned char RASTER_IMAGE::GetMaskRed() const
{
    return m_data ? m_data->maskRed : 0;
}


unsigned char RASTER_IMAGE::GetMaskGreen() const
{
    return m_data ? m_data->maskGreen : 0;
}


unsigned char RASTER_IMAGE::GetMaskBlue() const
{
    return m_data ? m_data->maskBlue : 0;
}


double RASTER_IMAGE::GetResolutionX() const
{
    return m_data ? m_data->dpiX : 0.0;
}


double RASTER_IMAGE::GetResolutionY() const
{
    return m_data ? m_data->dpiY : 0.0;
}


void RASTER_IMAGE::SetResolution( double aDpiX, double aDpiY )
{
    if( !m_data )
        return;

    unshare();
    m_data->dpiX = aDpiX;
    m_data->dpiY = aDpiY;
}


bool RASTER_IMAGE::IsSameAs( const RASTER_IMAGE& aOther ) const
{
    return m_data && m_data == aOther.m_data;
}


RASTER_IMAGE RASTER_IMAGE::Mirror( bool aHorizontally ) const
{
    RASTER_IMAGE result;

    if( !IsOk() )
        return result;

    result.m_data = std::make_shared<DATA>( *m_data );

    const int w = m_data->width;
    const int h = m_data->height;

    for( int y = 0; y < h; ++y )
    {
        for( int x = 0; x < w; ++x )
        {
            const int    sx = aHorizontally ? w - 1 - x : x;
            const int    sy = aHorizontally ? y : h - 1 - y;
            const size_t src = (size_t) sy * w + sx;
            const size_t dst = (size_t) y * w + x;

            memcpy( &result.m_data->rgb[dst * 3], &m_data->rgb[src * 3], 3 );

            if( HasAlpha() )
                result.m_data->alpha[dst] = m_data->alpha[src];
        }
    }

    return result;
}


RASTER_IMAGE RASTER_IMAGE::Rotate90( bool aClockwise ) const
{
    RASTER_IMAGE result;

    if( !IsOk() )
        return result;

    const int w = m_data->width;
    const int h = m_data->height;

    result.m_data = std::make_shared<DATA>( *m_data );
    result.m_data->width = h;
    result.m_data->height = w;

    for( int y = 0; y < h; ++y )
    {
        for( int x = 0; x < w; ++x )
        {
            // Destination of the source pixel (x, y) in the h x w image
            const int    dx = aClockwise ? h - 1 - y : y;
            const int    dy = aClockwise ? x : w - 1 - x;
            const size_t src = (size_t) y * w + x;
            const size_t dst = (size_t) dy * h + dx;

            memcpy( &result.m_data->rgb[dst * 3], &m_data->rgb[src * 3], 3 );

            if( HasAlpha() )
                result.m_data->alpha[dst] = m_data->alpha[src];
        }
    }

    return result;
}


RASTER_IMAGE RASTER_IMAGE::Rescale( int aWidth, int aHeight ) const
{
    RASTER_IMAGE result;

    if( !IsOk() || aWidth <= 0 || aHeight <= 0 )
        return result;

    result.m_data = std::make_shared<DATA>( *m_data );
    result.m_data->width = aWidth;
    result.m_data->height = aHeight;
    result.m_data->rgb.assign( (size_t) aWidth * aHeight * 3, 0 );

    if( HasAlpha() )
        result.m_data->alpha.assign( (size_t) aWidth * aHeight, 0 );

    // Fixed point steps, as wxImage::ResampleNearest() computes them
    const unsigned long xDelta = ( (unsigned long) m_data->width << 16 ) / aWidth;
    const unsigned long yDelta = ( (unsigned long) m_data->height << 16 ) / aHeight;
    unsigned long       y = 0;

    for( int j = 0; j < aHeight; ++j, y += yDelta )
    {
        const size_t  srcRow = ( y >> 16 ) * m_data->width;
        unsigned long x = 0;

        for( int i = 0; i < aWidth; ++i, x += xDelta )
        {
            const size_t src = srcRow + ( x >> 16 );
            const size_t dst = (size_t) j * aWidth + i;

            memcpy( &result.m_data->rgb[dst * 3], &m_data->rgb[src * 3], 3 );

            if( HasAlpha() )
                result.m_data->alpha[dst] = m_data->alpha[src];
        }
    }

    return result;
}


RASTER_IMAGE RASTER_IMAGE::ConvertToGreyscale() const
{
    RASTER_IMAGE result;

    if( !IsOk() )
        return result;

    result.m_data = std::make_shared<DATA>( *m_data );

    const size_t count = (size_t) m_data->width * m_data->height;

    for( size_t i = 0; i < count; ++i )
    {
        unsigned char* p = &result.m_data->rgb[i * 3];

        // Keep the mask colour, or the masked pixels would be shown
        if( m_data->hasMask && p[0] == m_data->maskRed && p[1] == m_data->maskGreen
                && p[2] == m_data->maskBlue )
        {
            continue;
        }

        const double        luma = p[0] * 0.299 + p[1] * 0.587 + p[2] * 0.114;
        const unsigned char grey = (unsigned char) std::lround( luma );

        p[0] = p[1] = p[2] = grey;
    }

    return result;
}


bool RASTER_IMAGE::Load( const void* aData, size_t aSize )
{
    const unsigned char* data = static_cast<const unsigned char*>( aData );

    if( !data || aSize < 4 )
        return false;

    static const unsigned char pngSignature[8] = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };

    if( aSize >= 8 && memcmp( data, pngSignature, 8 ) == 0 )
        return loadPng( data, aSize );

    if( data[0] == 0xFF && data[1] == 0xD8 && loadJpeg( data, aSize ) )
        return true;

    RASTER_IMAGE decoded;

    if( fallbackDecoder() && fallbackDecoder()( aData, aSize, decoded ) && decoded.IsOk() )
    {
        *this = decoded;
        m_data->type = TYPE::OTHER;
        return true;
    }

    return false;
}


bool RASTER_IMAGE::LoadFile( const wxString& aFileName )
{
    wxFFile file( aFileName, wxT( "rb" ) );

    if( !file.IsOpened() )
        return false;

    std::vector<unsigned char> buffer( (size_t) file.Length() );

    if( buffer.empty() || file.Read( buffer.data(), buffer.size() ) != buffer.size() )
        return false;

    return Load( buffer.data(), buffer.size() );
}


bool RASTER_IMAGE::Save( std::vector<unsigned char>& aOutput, TYPE aType, int aQuality ) const
{
    aOutput.clear();

    if( !IsOk() )
        return false;

    if( aType == TYPE::JPEG )
        return saveJpeg( aOutput, aQuality );

    return savePng( aOutput );
}


// ------------------------------------------------------------------------------------------
// PNG
// ------------------------------------------------------------------------------------------

namespace
{

struct PNG_READ_SOURCE
{
    const unsigned char* data;
    size_t               size;
    size_t               offset;
};


void pngReadFromMemory( png_structp aPng, png_bytep aOut, png_size_t aCount )
{
    PNG_READ_SOURCE* src = static_cast<PNG_READ_SOURCE*>( png_get_io_ptr( aPng ) );

    if( src->offset + aCount > src->size )
        png_error( aPng, "unexpected end of PNG data" );

    memcpy( aOut, src->data + src->offset, aCount );
    src->offset += aCount;
}


void pngWriteToVector( png_structp aPng, png_bytep aData, png_size_t aCount )
{
    auto* out = static_cast<std::vector<unsigned char>*>( png_get_io_ptr( aPng ) );
    out->insert( out->end(), aData, aData + aCount );
}


void pngFlush( png_structp )
{
}


void pngWarning( png_structp, png_const_charp )
{
}

} // namespace


bool RASTER_IMAGE::loadPng( const unsigned char* aData, size_t aSize )
{
    png_structp png = png_create_read_struct( PNG_LIBPNG_VER_STRING, nullptr, nullptr, pngWarning );

    if( !png )
        return false;

    png_infop info = png_create_info_struct( png );

    if( !info )
    {
        png_destroy_read_struct( &png, nullptr, nullptr );
        return false;
    }

    auto                     data = std::make_shared<DATA>();
    std::vector<png_byte>    pixels;
    std::vector<png_bytep>   rows;
    PNG_READ_SOURCE          source{ aData, aSize, 0 };

    if( setjmp( png_jmpbuf( png ) ) )
    {
        png_destroy_read_struct( &png, &info, nullptr );
        return false;
    }

    png_set_read_fn( png, &source, pngReadFromMemory );
    png_read_info( png, info );

    png_uint_32 width = png_get_image_width( png, info );
    png_uint_32 height = png_get_image_height( png, info );
    int         colorType = png_get_color_type( png, info );
    int         bitDepth = png_get_bit_depth( png, info );
    bool        hasAlpha = ( colorType & PNG_COLOR_MASK_ALPHA )
                           || png_get_valid( png, info, PNG_INFO_tRNS );

    if( bitDepth == 16 )
        png_set_strip_16( png );

    if( colorType == PNG_COLOR_TYPE_PALETTE )
        png_set_palette_to_rgb( png );

    if( colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8 )
        png_set_expand_gray_1_2_4_to_8( png );

    if( png_get_valid( png, info, PNG_INFO_tRNS ) )
        png_set_tRNS_to_alpha( png );

    if( colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_GRAY_ALPHA )
        png_set_gray_to_rgb( png );

    if( !hasAlpha )
        png_set_strip_alpha( png );

    png_set_interlace_handling( png );
    png_read_update_info( png, info );

    const int channels = hasAlpha ? 4 : 3;

    pixels.resize( (size_t) width * height * channels );
    rows.resize( height );

    for( png_uint_32 y = 0; y < height; ++y )
        rows[y] = pixels.data() + (size_t) y * width * channels;

    png_read_image( png, rows.data() );
    png_read_end( png, nullptr );

    png_uint_32 resX = 0, resY = 0;
    int         unit = 0;

    if( png_get_pHYs( png, info, &resX, &resY, &unit ) && unit == PNG_RESOLUTION_METER )
    {
        // Whole dots per cm, as wxImage reads them: the sizes of the images do not change
        data->dpiX = ( resX / 100 ) * 2.54;
        data->dpiY = ( resY / 100 ) * 2.54;
    }

    png_destroy_read_struct( &png, &info, nullptr );

    data->width = (int) width;
    data->height = (int) height;
    data->type = TYPE::PNG;
    data->rgb.resize( (size_t) width * height * 3 );

    if( hasAlpha )
    {
        data->alpha.resize( (size_t) width * height );

        for( size_t i = 0; i < data->alpha.size(); ++i )
        {
            memcpy( &data->rgb[i * 3], &pixels[i * 4], 3 );
            data->alpha[i] = pixels[i * 4 + 3];
        }
    }
    else
    {
        data->rgb.assign( pixels.begin(), pixels.end() );
    }

    m_data = data;
    return true;
}


bool RASTER_IMAGE::savePng( std::vector<unsigned char>& aOutput ) const
{
    png_structp png = png_create_write_struct( PNG_LIBPNG_VER_STRING, nullptr, nullptr, pngWarning );

    if( !png )
        return false;

    png_infop info = png_create_info_struct( png );

    if( !info )
    {
        png_destroy_write_struct( &png, nullptr );
        return false;
    }

    // A mask is written as an alpha channel
    const bool                 withAlpha = HasAlpha() || HasMask();
    const int                  channels = withAlpha ? 4 : 3;
    const int                  w = m_data->width;
    const int                  h = m_data->height;
    std::vector<unsigned char> pixels( (size_t) w * h * channels );
    std::vector<png_bytep>     rows( h );

    for( size_t i = 0; i < (size_t) w * h; ++i )
    {
        const unsigned char* p = &m_data->rgb[i * 3];

        memcpy( &pixels[i * channels], p, 3 );

        if( withAlpha )
        {
            unsigned char a = HasAlpha() ? m_data->alpha[i] : 255;

            if( HasMask() && p[0] == m_data->maskRed && p[1] == m_data->maskGreen
                    && p[2] == m_data->maskBlue )
            {
                a = 0;
            }

            pixels[i * channels + 3] = a;
        }
    }

    for( int y = 0; y < h; ++y )
        rows[y] = pixels.data() + (size_t) y * w * channels;

    if( setjmp( png_jmpbuf( png ) ) )
    {
        png_destroy_write_struct( &png, &info );
        aOutput.clear();
        return false;
    }

    png_set_write_fn( png, &aOutput, pngWriteToVector, pngFlush );
    png_set_IHDR( png, info, w, h, 8, withAlpha ? PNG_COLOR_TYPE_RGB_ALPHA : PNG_COLOR_TYPE_RGB,
                  PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT );

    if( m_data->dpiX > 0 && m_data->dpiY > 0 )
    {
        png_set_pHYs( png, info, (png_uint_32) std::lround( m_data->dpiX / 0.0254 ),
                      (png_uint_32) std::lround( m_data->dpiY / 0.0254 ), PNG_RESOLUTION_METER );
    }

    png_write_info( png, info );
    png_write_image( png, rows.data() );
    png_write_end( png, nullptr );
    png_destroy_write_struct( &png, &info );

    return true;
}


// ------------------------------------------------------------------------------------------
// JPEG
// ------------------------------------------------------------------------------------------

namespace
{

struct JPEG_ERROR_MANAGER
{
    jpeg_error_mgr pub;
    jmp_buf        jump;
};


void jpegErrorExit( j_common_ptr aInfo )
{
    JPEG_ERROR_MANAGER* err = reinterpret_cast<JPEG_ERROR_MANAGER*>( aInfo->err );
    longjmp( err->jump, 1 );
}


void jpegOutputMessage( j_common_ptr )
{
}

} // namespace


bool RASTER_IMAGE::loadJpeg( const unsigned char* aData, size_t aSize )
{
    jpeg_decompress_struct cinfo;
    JPEG_ERROR_MANAGER     jerr;
    auto                   data = std::make_shared<DATA>();

    cinfo.err = jpeg_std_error( &jerr.pub );
    jerr.pub.error_exit = jpegErrorExit;
    jerr.pub.output_message = jpegOutputMessage;

    if( setjmp( jerr.jump ) )
    {
        jpeg_destroy_decompress( &cinfo );
        return false;
    }

    jpeg_create_decompress( &cinfo );
    jpeg_mem_src( &cinfo, aData, (unsigned long) aSize );

    if( jpeg_read_header( &cinfo, TRUE ) != JPEG_HEADER_OK )
    {
        jpeg_destroy_decompress( &cinfo );
        return false;
    }

    // Grey images are expanded to RGB; CMYK images are left to the fallback decoder
    if( cinfo.jpeg_color_space == JCS_CMYK || cinfo.jpeg_color_space == JCS_YCCK )
    {
        jpeg_destroy_decompress( &cinfo );
        return false;
    }

    cinfo.out_color_space = JCS_RGB;
    jpeg_start_decompress( &cinfo );

    data->width = (int) cinfo.output_width;
    data->height = (int) cinfo.output_height;
    data->type = TYPE::JPEG;
    data->rgb.resize( (size_t) data->width * data->height * 3 );

    while( cinfo.output_scanline < cinfo.output_height )
    {
        JSAMPROW row = &data->rgb[(size_t) cinfo.output_scanline * data->width * 3];
        jpeg_read_scanlines( &cinfo, &row, 1 );
    }

    if( cinfo.density_unit == 1 )           // dots per inch
    {
        data->dpiX = cinfo.X_density;
        data->dpiY = cinfo.Y_density;
    }
    else if( cinfo.density_unit == 2 )      // dots per cm
    {
        data->dpiX = cinfo.X_density * 2.54;
        data->dpiY = cinfo.Y_density * 2.54;
    }

    jpeg_finish_decompress( &cinfo );
    jpeg_destroy_decompress( &cinfo );

    m_data = data;
    return true;
}


bool RASTER_IMAGE::saveJpeg( std::vector<unsigned char>& aOutput, int aQuality ) const
{
    jpeg_compress_struct cinfo;
    JPEG_ERROR_MANAGER   jerr;
    unsigned char*       buffer = nullptr;
    unsigned long        bufferSize = 0;

    cinfo.err = jpeg_std_error( &jerr.pub );
    jerr.pub.error_exit = jpegErrorExit;
    jerr.pub.output_message = jpegOutputMessage;

    if( setjmp( jerr.jump ) )
    {
        jpeg_destroy_compress( &cinfo );
        free( buffer );
        return false;
    }

    jpeg_create_compress( &cinfo );
    jpeg_mem_dest( &cinfo, &buffer, &bufferSize );

    cinfo.image_width = m_data->width;
    cinfo.image_height = m_data->height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults( &cinfo );
    jpeg_set_quality( &cinfo, aQuality, TRUE );

    if( m_data->dpiX > 0 && m_data->dpiY > 0 )
    {
        cinfo.density_unit = 1;
        cinfo.X_density = (UINT16) std::lround( m_data->dpiX );
        cinfo.Y_density = (UINT16) std::lround( m_data->dpiY );
    }

    jpeg_start_compress( &cinfo, TRUE );

    while( cinfo.next_scanline < cinfo.image_height )
    {
        JSAMPROW row = const_cast<JSAMPROW>( &m_data->rgb[(size_t) cinfo.next_scanline
                                                          * m_data->width * 3] );
        jpeg_write_scanlines( &cinfo, &row, 1 );
    }

    jpeg_finish_compress( &cinfo );
    jpeg_destroy_compress( &cinfo );

    aOutput.assign( buffer, buffer + bufferSize );
    free( buffer );

    return true;
}
