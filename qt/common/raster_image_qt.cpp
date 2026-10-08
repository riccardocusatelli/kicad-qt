/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <raster_image_qt.h>

#include <cmath>


/// Dots per inch to dots per meter (the resolution of QImage).
static int dotsPerMeter( double aDpi )
{
    return (int) std::lround( aDpi / 0.0254 );
}


QImage ToQImage( const RASTER_IMAGE& aImage )
{
    if( !aImage.IsOk() )
        return QImage();

    const int  width = aImage.GetWidth();
    const int  height = aImage.GetHeight();
    const bool alpha = aImage.HasAlpha();
    const bool mask = !alpha && aImage.HasMask();

    QImage image( width, height,
                  alpha || mask ? QImage::Format_RGBA8888 : QImage::Format_RGB888 );

    const unsigned char* rgb = aImage.GetData();
    const unsigned char* a = alpha ? aImage.GetAlpha() : nullptr;

    for( int y = 0; y < height; y++ )
    {
        unsigned char* dst = image.scanLine( y );

        for( int x = 0; x < width; x++ )
        {
            dst[0] = rgb[0];
            dst[1] = rgb[1];
            dst[2] = rgb[2];

            if( alpha )
            {
                dst[3] = *a++;
            }
            else if( mask )
            {
                bool masked = rgb[0] == aImage.GetMaskRed() && rgb[1] == aImage.GetMaskGreen()
                              && rgb[2] == aImage.GetMaskBlue();
                dst[3] = masked ? 0 : 255;
            }

            dst += alpha || mask ? 4 : 3;
            rgb += 3;
        }
    }

    if( aImage.GetResolutionX() > 0 && aImage.GetResolutionY() > 0 )
    {
        image.setDotsPerMeterX( dotsPerMeter( aImage.GetResolutionX() ) );
        image.setDotsPerMeterY( dotsPerMeter( aImage.GetResolutionY() ) );
    }

    return image;
}


RASTER_IMAGE FromQImage( const QImage& aImage )
{
    if( aImage.isNull() )
        return RASTER_IMAGE();

    const bool   alpha = aImage.hasAlphaChannel();
    const QImage source = aImage.convertToFormat( alpha ? QImage::Format_RGBA8888
                                                        : QImage::Format_RGB888 );

    RASTER_IMAGE image( source.width(), source.height() );

    if( alpha )
        image.InitAlpha();

    unsigned char* rgb = image.GetData();
    unsigned char* a = alpha ? image.GetAlpha() : nullptr;

    for( int y = 0; y < source.height(); y++ )
    {
        const unsigned char* src = source.constScanLine( y );

        for( int x = 0; x < source.width(); x++ )
        {
            rgb[0] = src[0];
            rgb[1] = src[1];
            rgb[2] = src[2];

            if( alpha )
                *a++ = src[3];

            src += alpha ? 4 : 3;
            rgb += 3;
        }
    }

    if( source.dotsPerMeterX() > 0 && source.dotsPerMeterY() > 0 )
        image.SetResolution( source.dotsPerMeterX() * 0.0254, source.dotsPerMeterY() * 0.0254 );

    return image;
}
