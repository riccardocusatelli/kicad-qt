/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Mario Luzeiro <mrluzeiro@ua.pt>
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

 /**
 * @file buffers_debug.cpp
 * @brief
 */

#include "buffers_debug.h"
#include <raster_image.h>
#include <wx/ffile.h>

#include <cstring>
#include <vector>

/// Save a buffer as a PNG file (mirrored top-bottom), and free it (allocated with malloc()).
static void dbg_save_image( const wxString& aFileName, unsigned char* aRGBpixelBuffer,
                            unsigned char* aAlphaBuffer, unsigned int aXSize, unsigned int aYSize )
{
    RASTER_IMAGE image( aXSize, aYSize );
    memcpy( image.GetData(), aRGBpixelBuffer, (size_t) aXSize * aYSize * 3 );

    if( aAlphaBuffer )
    {
        image.InitAlpha();
        memcpy( image.GetAlpha(), aAlphaBuffer, (size_t) aXSize * aYSize );
    }

    free( aRGBpixelBuffer );
    free( aAlphaBuffer );

    std::vector<unsigned char> png;

    if( image.Mirror( false ).Save( png, RASTER_IMAGE::TYPE::PNG ) )
    {
        wxFFile file( aFileName + wxT( ".png" ), wxT( "wb" ) );

        if( file.IsOpened() )
            file.Write( png.data(), png.size() );
    }
}


static void dbg_save_rgb_buffer( const wxString& aFileName, unsigned char *aRGBpixelBuffer,
                                 unsigned int aXSize, unsigned int aYSize )
{
    dbg_save_image( aFileName, aRGBpixelBuffer, nullptr, aXSize, aYSize );
}


static void dbg_save_rgb_a_buffer( const wxString& aFileName, unsigned char* aRGBpixelBuffer,
                                   unsigned char* aAlphaBuffer, unsigned int aXSize,
                                   unsigned int aYSize )
{
    dbg_save_image( aFileName, aRGBpixelBuffer, aAlphaBuffer, aXSize, aYSize );
}


void DBG_SaveBuffer( const wxString& aFileName, const unsigned char *aInBuffer,
                     unsigned int aXSize, unsigned int aYSize )
{
    const unsigned int wxh = aXSize * aYSize;

    unsigned char *pixelbuffer = (unsigned char*) malloc( wxh * 3 );

    for( unsigned int i = 0; i < wxh; ++i )
    {
        unsigned char v = aInBuffer[i];

        // Set RGB value with all same values intensities
        pixelbuffer[i * 3 + 0] = v;
        pixelbuffer[i * 3 + 1] = v;
        pixelbuffer[i * 3 + 2] = v;
    }

    dbg_save_rgb_buffer( aFileName, pixelbuffer, aXSize, aYSize );
}


void DBG_SaveBuffer( const wxString& aFileName, const float *aInBuffer,
                     unsigned int aXSize, unsigned int aYSize )
{
    const unsigned int wxh = aXSize * aYSize;

    unsigned char *pixelbuffer = (unsigned char*) malloc( wxh * 3 );

    for( unsigned int i = 0; i < wxh; ++i )
    {
        const unsigned char v = (unsigned char)glm::min( (int)(aInBuffer[i] * 255.0f), 255 );

        // Set RGB value with all same values intensities
        pixelbuffer[i * 3 + 0] = v;
        pixelbuffer[i * 3 + 1] = v;
        pixelbuffer[i * 3 + 2] = v;
    }

    dbg_save_rgb_buffer( aFileName, pixelbuffer, aXSize, aYSize );
}


void DBG_SaveBuffer( const wxString& aFileName, const SFVEC3F *aInBuffer,
                     unsigned int aXSize, unsigned int aYSize )
{
    const unsigned int wxh = aXSize * aYSize;

    unsigned char *pixelbuffer = (unsigned char*) malloc( wxh * 3 );

    for( unsigned int i = 0; i < wxh; ++i )
    {
        const SFVEC3F &v = aInBuffer[i];
        const unsigned int ix3 = i * 3;

        // Set RGB value with all same values intensities
        pixelbuffer[ix3 + 0] = (unsigned char) glm::min( (int) ( v.r * 255.0f ), 255 );
        pixelbuffer[ix3 + 1] = (unsigned char) glm::min( (int) ( v.g * 255.0f ), 255 );
        pixelbuffer[ix3 + 2] = (unsigned char) glm::min( (int) ( v.b * 255.0f ), 255 );
    }

    dbg_save_rgb_buffer( aFileName, pixelbuffer, aXSize, aYSize );
}


void DBG_SaveBuffer( const wxString& aFileName, const SFVEC4F *aInBuffer,
                     unsigned int aXSize, unsigned int aYSize )
{
    const unsigned int wxh = aXSize * aYSize;

    unsigned char *pixelbuffer = (unsigned char*) malloc( wxh * 4 );
    unsigned char *alphabuffer = (unsigned char*) malloc( wxh );

    for( unsigned int i = 0; i < wxh; ++i )
    {
        const SFVEC4F &v = aInBuffer[i];
        const unsigned int ix3 = i * 3;

        // Set RGB value with all same values intensities
        pixelbuffer[ix3 + 0] = (unsigned char) glm::min( (int) ( v.r * 255.0f ), 255 );
        pixelbuffer[ix3 + 1] = (unsigned char) glm::min( (int) ( v.g * 255.0f ), 255 );
        pixelbuffer[ix3 + 2] = (unsigned char) glm::min( (int) ( v.b * 255.0f ), 255 );

        alphabuffer[i] = (unsigned char) glm::min( (int) ( v.a * 255.0f ), 255 );
    }

    dbg_save_rgb_a_buffer( aFileName, pixelbuffer, alphabuffer, aXSize, aYSize );
}


void DBG_SaveNormalsBuffer( const wxString& aFileName, const SFVEC3F *aInNormalsBuffer,
                            unsigned int aXSize, unsigned int aYSize )
{
    const unsigned int wxh = aXSize * aYSize;

    unsigned char *pixelbuffer = (unsigned char*) malloc( wxh * 3 );

    for( unsigned int i = 0; i < wxh; ++i )
    {
        const SFVEC3F &v = aInNormalsBuffer[i];
        const unsigned int ix3 = i * 3;

        // Set RGB value with all same values intensities
        pixelbuffer[ix3 + 0] = (unsigned char) glm::min( (int) ( ( v.r + 1.0f ) * 127.0f ), 255 );
        pixelbuffer[ix3 + 1] = (unsigned char) glm::min( (int) ( ( v.g + 1.0f ) * 127.0f ), 255 );
        pixelbuffer[ix3 + 2] = (unsigned char) glm::min( (int) ( ( v.b + 1.0f ) * 127.0f ), 255 );
    }

    dbg_save_rgb_buffer( aFileName, pixelbuffer, aXSize, aYSize );
}
