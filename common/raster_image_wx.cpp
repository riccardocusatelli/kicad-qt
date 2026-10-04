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

#include <raster_image_wx.h>

#include <cstring>

#include <wx/image.h>
#include <wx/mstream.h>


wxImage ToWxImage( const RASTER_IMAGE& aImage )
{
    if( !aImage.IsOk() )
        return wxImage();

    const int w = aImage.GetWidth();
    const int h = aImage.GetHeight();
    wxImage   image( w, h, false );

    memcpy( image.GetData(), aImage.GetData(), (size_t) w * h * 3 );

    if( aImage.HasAlpha() )
    {
        image.InitAlpha();
        memcpy( image.GetAlpha(), aImage.GetAlpha(), (size_t) w * h );
    }
    else if( aImage.HasMask() )
    {
        image.SetMaskColour( aImage.GetMaskRed(), aImage.GetMaskGreen(), aImage.GetMaskBlue() );
    }

    if( aImage.GetResolutionX() > 0 )
    {
        image.SetOption( wxIMAGE_OPTION_RESOLUTIONUNIT, wxIMAGE_RESOLUTION_INCHES );
        image.SetOption( wxIMAGE_OPTION_RESOLUTIONX, (int) ( aImage.GetResolutionX() + 0.5 ) );
        image.SetOption( wxIMAGE_OPTION_RESOLUTIONY, (int) ( aImage.GetResolutionY() + 0.5 ) );
    }

    return image;
}


RASTER_IMAGE FromWxImage( const wxImage& aImage )
{
    if( !aImage.IsOk() )
        return RASTER_IMAGE();

    const int    w = aImage.GetWidth();
    const int    h = aImage.GetHeight();
    RASTER_IMAGE image( w, h );

    memcpy( image.GetData(), aImage.GetData(), (size_t) w * h * 3 );

    if( aImage.HasAlpha() )
    {
        image.InitAlpha();
        memcpy( image.GetAlpha(), aImage.GetAlpha(), (size_t) w * h );
    }
    else if( aImage.HasMask() )
    {
        image.SetMaskColour( aImage.GetMaskRed(), aImage.GetMaskGreen(), aImage.GetMaskBlue() );
    }

    // Resolution as BITMAP_BASE read it from wxImage: whole dots per unit
    int dpiX = aImage.GetOptionInt( wxIMAGE_OPTION_RESOLUTIONX );
    int dpiY = aImage.GetOptionInt( wxIMAGE_OPTION_RESOLUTIONY );

    if( dpiX > 0 )
    {
        double factor = aImage.GetOptionInt( wxIMAGE_OPTION_RESOLUTIONUNIT ) == wxIMAGE_RESOLUTION_CM
                                ? 2.54
                                : 1.0;

        image.SetResolution( dpiX * factor, ( dpiY > 0 ? dpiY : dpiX ) * factor );
    }

    switch( aImage.GetType() )
    {
    case wxBITMAP_TYPE_PNG:  image.SetType( RASTER_IMAGE::TYPE::PNG );   break;
    case wxBITMAP_TYPE_JPEG: image.SetType( RASTER_IMAGE::TYPE::JPEG );  break;
    default:                 image.SetType( RASTER_IMAGE::TYPE::OTHER ); break;
    }

    return image;
}


void RegisterWxImageDecoder()
{
    RASTER_IMAGE::SetFallbackDecoder(
            []( const void* aData, size_t aSize, RASTER_IMAGE& aImage ) -> bool
            {
                wxMemoryInputStream stream( aData, aSize );
                wxImage             image;

                if( !image.LoadFile( stream, wxBITMAP_TYPE_ANY ) )
                    return false;

                aImage = FromWxImage( image );
                return aImage.IsOk();
            } );
}
