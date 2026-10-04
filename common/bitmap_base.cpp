/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 jean-pierre.charras
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "bitmap_base.h"

#include <cstring>
#include <math/util.h>    // for KiROUND
#include <plotters/plotter.h>
#include <richio.h>
#include <wx/mstream.h>
#include <wx/stream.h>
#include <wx/string.h>
#include <wx/wfstream.h>


BITMAP_BASE::BITMAP_BASE( const VECTOR2I& pos )
{
    m_scale  = 1.0;                     // 1.0 = original bitmap size
    m_imageType = RASTER_IMAGE::TYPE::INVALID;
    m_ppi    = 300;                     // the bitmap definition. the default is 300PPI
    m_pixelSizeIu = 254000.0 / m_ppi;   // a pixel size value OK for bitmaps using 300 PPI
                                        // for Eeschema which uses currently 254000PPI
    m_isMirroredX = false;
    m_isMirroredY = false;
    m_rotation   = ANGLE_0;
}


BITMAP_BASE::BITMAP_BASE( const BITMAP_BASE& aSchBitmap )
{
    m_scale = aSchBitmap.m_scale;
    m_ppi   = aSchBitmap.m_ppi;
    m_pixelSizeIu = aSchBitmap.m_pixelSizeIu;
    m_isMirroredX = aSchBitmap.m_isMirroredX;
    m_isMirroredY = aSchBitmap.m_isMirroredY;
    m_rotation = aSchBitmap.m_rotation;
    m_imageType = aSchBitmap.m_imageType;

    if( aSchBitmap.m_image.IsOk() )
    {
        m_image = aSchBitmap.m_image;
        m_originalImage = aSchBitmap.m_originalImage;
        m_imageData = aSchBitmap.m_imageData;
        m_imageId = aSchBitmap.m_imageId;
    }
}


void BITMAP_BASE::updatePPI()
{
    // Todo: eventually we need to support dpi / scaling in both dimensions
    double dpiX = m_originalImage.GetResolutionX();

    if( dpiX > 1 )
        m_ppi = KiROUND( dpiX );
}


void BITMAP_BASE::ImportData( BITMAP_BASE& aItem )
{
    m_image = aItem.m_image;
    m_originalImage = aItem.m_originalImage;
    m_imageId = aItem.m_imageId;
    m_scale = aItem.m_scale;
    m_ppi = aItem.m_ppi;
    m_pixelSizeIu = aItem.m_pixelSizeIu;
    m_isMirroredX = aItem.m_isMirroredX;
    m_isMirroredY = aItem.m_isMirroredY;
    m_rotation = aItem.m_rotation;
    m_imageType = aItem.m_imageType;
    m_imageData = aItem.m_imageData;
}


bool BITMAP_BASE::ReadImageFile( wxInputStream& aInStream )
{
    // Store the original image data in m_imageData
    size_t dataSize = aInStream.GetLength();

    m_imageData.SetBufSize( dataSize );
    aInStream.Read( m_imageData.GetData(), dataSize );
    m_imageData.SetDataLen( dataSize );

    RASTER_IMAGE image;

    if( !image.Load( m_imageData.GetData(), dataSize ) )
        return false;

    return setImage( image );
}


bool BITMAP_BASE::ReadImageFile( wxMemoryBuffer& aBuf )
{
    // Store the original image data in m_imageData
    m_imageData = aBuf;

    RASTER_IMAGE image;

    if( !image.Load( m_imageData.GetData(), m_imageData.GetBufSize() ) )
        return false;

    return setImage( image );
}


bool BITMAP_BASE::ReadImageFile( const wxString& aFullFilename )
{
    wxFileInputStream file_stream( aFullFilename );

    // Check if the file could be opened successfully
    if( !file_stream.IsOk() )
        return false;

    return ReadImageFile( file_stream );
}


bool BITMAP_BASE::SetImage( const RASTER_IMAGE& aImage )
{
    // The encoded data of a previous image does not describe this one
    m_imageData.Clear();
    return setImage( aImage );
}


bool BITMAP_BASE::setImage( const RASTER_IMAGE& aImage )
{
    if( !aImage.IsOk() )
        return false;

    m_image = aImage;
    m_originalImage = aImage;
    m_imageType = aImage.GetType();
    m_imageId = KIID();
    updatePPI();

    return true;
}


bool BITMAP_BASE::SaveImageData( wxOutputStream& aOutStream ) const
{
    if( m_imageData.IsEmpty() )
    {
        // No encoded data (the image was changed or created): encode m_image
        std::vector<unsigned char> data;
        RASTER_IMAGE::TYPE         type = m_imageType == RASTER_IMAGE::TYPE::JPEG
                                                  ? RASTER_IMAGE::TYPE::JPEG
                                                  : RASTER_IMAGE::TYPE::PNG;

        if( !m_image.Save( data, type ) )
            return false;

        aOutStream.Write( data.data(), data.size() );
    }
    else
    {
        // Write the contents of m_imageData to the stream.
        aOutStream.Write( m_imageData.GetData(), m_imageData.GetDataLen() );
    }

    return true;
}


bool BITMAP_BASE::LoadLegacyData( LINE_READER& aLine, wxString& aErrorMsg )
{
    std::vector<unsigned char> data;
    char*                      line;

    while( true )
    {
        if( !aLine.ReadLine() )
        {
            aErrorMsg = wxT( "Unexpected end of data" );
            return false;
        }

        line = aLine.Line();

        if( strncasecmp( line, "EndData", 4 ) == 0 )
        {
            // all the PNG date is read.
            RASTER_IMAGE image;

            image.Load( data.data(), data.size() );

            m_image = image;
            m_originalImage = image;
            m_imageType = image.GetType();
            updateImageDataBuffer();
            break;
        }

        // Read PNG data, stored in hexadecimal,
        // each byte = 2 hexadecimal digits and a space between 2 bytes
        // and put it in memory stream buffer
        int len = strlen( line );

        for( ; len > 0; len -= 3, line += 3 )
        {
            int value = 0;

            if( sscanf( line, "%X", &value ) == 1 )
                data.push_back( (unsigned char) value );
            else
                break;
        }
    }

    return true;
}


const BOX2I BITMAP_BASE::GetBoundingBox() const
{
    BOX2I    bbox;
    VECTOR2I size = GetSize();

    bbox.Inflate( size.x / 2, size.y / 2 );

    return bbox;
}


VECTOR2I BITMAP_BASE::GetSize() const
{
    VECTOR2I size;

    if( m_image.IsOk() )
    {
        size.x = KiROUND( m_image.GetWidth() * GetScalingFactor() );
        size.y = KiROUND( m_image.GetHeight() * GetScalingFactor() );
    }

    return size;
}


void BITMAP_BASE::Mirror( FLIP_DIRECTION aFlipDirection )
{
    if( m_image.IsOk() )
    {
        m_image = m_image.Mirror( aFlipDirection == FLIP_DIRECTION::LEFT_RIGHT );

        if( aFlipDirection == FLIP_DIRECTION::TOP_BOTTOM )
            m_isMirroredY = !m_isMirroredY;
        else
            m_isMirroredX = !m_isMirroredX;

        m_imageData.Clear();
    }
}


void BITMAP_BASE::Rotate( bool aRotateCCW )
{
    if( m_image.IsOk() )
    {
        // RASTER_IMAGE::Rotate90() keeps the resolution
        m_image = m_image.Rotate90( !aRotateCCW );

        m_rotation += ( aRotateCCW ? ANGLE_90 : -ANGLE_90 );
        m_imageData.Clear();
    }
}


void BITMAP_BASE::ConvertToGreyscale()
{
    if( m_image.IsOk() )
    {
        m_image = m_image.ConvertToGreyscale();
        m_originalImage = m_originalImage.ConvertToGreyscale();
        m_imageData.Clear();
        m_imageId = KIID();
    }
}


void BITMAP_BASE::PlotImage( PLOTTER*       aPlotter, const VECTOR2I& aPos,
                             const COLOR4D& aDefaultColor,
                             int            aDefaultPensize ) const
{
    if( !m_image.IsOk() )
        return;

    // These 2 lines are useful only for plotters that cannot plot a bitmap
    // and plot a rectangle instead of.
    aPlotter->SetColor( aDefaultColor );
    aPlotter->SetCurrentLineWidth( aDefaultPensize );
    aPlotter->PlotImage( m_image, aPos, GetScalingFactor() );
}


void BITMAP_BASE::updateImageDataBuffer()
{
    if( m_image.IsOk() )
    {
        std::vector<unsigned char> data;
        RASTER_IMAGE::TYPE         type = m_imageType == RASTER_IMAGE::TYPE::JPEG
                                                  ? RASTER_IMAGE::TYPE::JPEG
                                                  : RASTER_IMAGE::TYPE::PNG;

        if( !m_image.Save( data, type ) )
            return;

        m_imageData.Clear();
        m_imageData.AppendData( data.data(), data.size() );
    }
}
