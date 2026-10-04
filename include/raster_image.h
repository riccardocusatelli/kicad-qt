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

#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

#include <kicommon.h>
#include <wx/string.h>


/**
 * A raster image of the data model: reference images, bitmaps in schematics and drawing
 * sheets, and the images the plotters write.
 *
 * The pixels are 8 bit RGB, with an optional 8 bit alpha channel or a mask colour (the
 * layout of wxImage, so the GUI converts without copying pixel by pixel).  PNG and JPEG are
 * read and written without wxWidgets; the GUI can register a decoder for other formats.
 *
 * Copies share the pixels until one of them is changed.
 */
class KICOMMON_API RASTER_IMAGE
{
public:
    /// The encoding an image was read from (and is written to by default).
    enum class TYPE
    {
        INVALID,
        PNG,
        JPEG,
        OTHER       ///< Read by the fallback decoder; written as PNG
    };

    /// Decodes an image format without built-in support; @return false if it cannot.
    using DECODER = std::function<bool( const void* aData, size_t aSize, RASTER_IMAGE& aImage )>;

    RASTER_IMAGE();

    /// A black image of @a aWidth x @a aHeight pixels, without alpha.
    RASTER_IMAGE( int aWidth, int aHeight );

    bool IsOk() const;
    int  GetWidth() const;
    int  GetHeight() const;

    TYPE GetType() const;
    void SetType( TYPE aType );

    /// The RGB pixels, 3 bytes per pixel, row after row.
    unsigned char*       GetData();
    const unsigned char* GetData() const;

    unsigned char GetRed( int aX, int aY ) const;
    unsigned char GetGreen( int aX, int aY ) const;
    unsigned char GetBlue( int aX, int aY ) const;
    void          SetRGB( int aX, int aY, unsigned char aRed, unsigned char aGreen,
                          unsigned char aBlue );

    bool HasAlpha() const;

    /// Add an alpha channel, fully opaque.
    void InitAlpha();

    /// The alpha channel (1 byte per pixel), or nullptr.
    unsigned char*       GetAlpha();
    const unsigned char* GetAlpha() const;

    unsigned char GetAlpha( int aX, int aY ) const;
    void          SetAlpha( int aX, int aY, unsigned char aAlpha );

    /// The pixels of the mask colour are transparent (images without alpha channel).
    bool          HasMask() const;
    void          SetMaskColour( unsigned char aRed, unsigned char aGreen, unsigned char aBlue );
    void          RemoveMask();
    unsigned char GetMaskRed() const;
    unsigned char GetMaskGreen() const;
    unsigned char GetMaskBlue() const;

    /// Resolution in dots per inch; 0 if unknown.
    double GetResolutionX() const;
    double GetResolutionY() const;
    void   SetResolution( double aDpiX, double aDpiY );

    /// True if both images share the same pixels.
    bool IsSameAs( const RASTER_IMAGE& aOther ) const;

    /// The image mirrored left-right (@a aHorizontally) or top-bottom.
    RASTER_IMAGE Mirror( bool aHorizontally ) const;

    /// The image rotated by 90 degrees.  The resolution is kept.
    RASTER_IMAGE Rotate90( bool aClockwise ) const;

    /**
     * The image scaled to @a aWidth x @a aHeight, by nearest neighbour (as wxImage::Rescale()
     * with the default quality).
     */
    RASTER_IMAGE Rescale( int aWidth, int aHeight ) const;

    /// The image in grey levels (luma 0.299 R + 0.587 G + 0.114 B); the mask colour is kept.
    RASTER_IMAGE ConvertToGreyscale() const;

    /// Read an image from encoded data (PNG, JPEG, or what the fallback decoder reads).
    bool Load( const void* aData, size_t aSize );

    /// Read an image file.
    bool LoadFile( const wxString& aFileName );

    /**
     * Encode the image as PNG or JPEG (other types are encoded as PNG).
     * @param aQuality the JPEG quality, 0..100.
     */
    bool Save( std::vector<unsigned char>& aOutput, TYPE aType = TYPE::PNG,
               int aQuality = 75 ) const;

    /// Set the decoder of the formats without built-in support (the GUI uses wxImage).
    static void SetFallbackDecoder( DECODER aDecoder );

private:
    struct DATA;

    /// Make the pixels exclusive to this image before changing them.
    void unshare();

    bool loadPng( const unsigned char* aData, size_t aSize );
    bool loadJpeg( const unsigned char* aData, size_t aSize );
    bool savePng( std::vector<unsigned char>& aOutput ) const;
    bool saveJpeg( std::vector<unsigned char>& aOutput, int aQuality ) const;

    std::shared_ptr<DATA> m_data;
};
