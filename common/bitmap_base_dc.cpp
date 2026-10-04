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

/**
 * @file bitmap_base_dc.cpp
 * @brief BITMAP_BASE::DrawBitmap(): drawing on a wxDC (printing), GUI only.
 */

#include "bitmap_base.h"

#include <gr_basic.h>
#include <math/util.h>
#include <raster_image_wx.h>
#include <wx/bitmap.h>
#include <wx/dc.h>


void BITMAP_BASE::DrawBitmap( wxDC* aDC, const VECTOR2I& aPos,
                              const KIGFX::COLOR4D& aBackgroundColor ) const
{
    if( !m_image.IsOk() )
        return;

    const wxBitmap bitmap( ToWxImage( m_image ) );

    VECTOR2I pos = aPos;
    VECTOR2I size = GetSize();

    // This fixes a bug in OSX that should be fixed in the 3.0.3 version or later.
    if( ( size.x == 0 ) || ( size.y == 0 ) )
        return;

    // To draw the bitmap, pos is the upper left corner position
    pos.x -= size.x / 2;
    pos.y -= size.y / 2;

    double scale;
    int    logicalOriginX, logicalOriginY;
    aDC->GetUserScale( &scale, &scale );
    aDC->GetLogicalOrigin( &logicalOriginX, &logicalOriginY );

    // We already have issues to draw a bitmap on the wxDC, depending on wxWidgets version.
    // Now we have an issue on wxWidgets 3.1.6 to fix the clip area
    // and the bitmap position when using TransformMatrix
    // So for version == 3.1.6  do not use it
    // Be careful before changing the code.
    bool useTransform = aDC->CanUseTransformMatrix();

    wxAffineMatrix2D init_matrix = aDC->GetTransformMatrix();

    // Note: clipping bitmap area was made to fix a minor issue in old versions of
    // KiCad/wxWidgets (5.1 / wx 3.0)
    // However SetClippingRegion creates a lot of issues (different ways to fix the
    // position and size of the area, depending on wxWidgets version)because it changes with
    // each versions of wxWidgets, so it is now disabled
    // However the code is still here, just in case
    // #define USE_CLIP_AREA

    wxPoint clipAreaPos;

    if( useTransform )
    {
        wxAffineMatrix2D matrix = aDC->GetTransformMatrix();
        matrix.Translate( pos.x, pos.y );
        matrix.Scale( GetScalingFactor(), GetScalingFactor() );
        aDC->SetTransformMatrix( matrix );

        // Needed on wx <= 3.1.5, and this is strange...
        // Nevertheless, this code has problem (the bitmap is not seen)
        // with wx version > 3.1.5
        clipAreaPos.x = pos.x;
        clipAreaPos.y = pos.y;

        pos.x = pos.y = 0;
    }
    else
    {
        aDC->SetUserScale( scale * GetScalingFactor(), scale * GetScalingFactor() );
        aDC->SetLogicalOrigin( logicalOriginX / GetScalingFactor(),
                               logicalOriginY / GetScalingFactor() );

        pos.x  = KiROUND( pos.x / GetScalingFactor() );
        pos.y  = KiROUND( pos.y / GetScalingFactor() );
        size.x = KiROUND( size.x / GetScalingFactor() );
        size.y = KiROUND( size.y / GetScalingFactor() );
        clipAreaPos.x = pos.x;
        clipAreaPos.y = pos.y;
    }

#ifdef USE_CLIP_AREA
    aDC->DestroyClippingRegion();
    aDC->SetClippingRegion( clipAreaPos, wxSize( size.x, size.y ) );
#endif

    if( aBackgroundColor != COLOR4D::UNSPECIFIED && bitmap.HasAlpha() )
    {
        // Most printers don't support transparent images properly,
        // so blend the image with background color.

        int w = bitmap.GetWidth();
        int h = bitmap.GetHeight();

        wxImage  image( w, h );
        wxColour bgColor = aBackgroundColor.ToColour();

        image.SetRGB( wxRect( 0, 0, w, h ), bgColor.Red(), bgColor.Green(), bgColor.Blue() );
        image.Paste( bitmap.ConvertToImage(), 0, 0, wxIMAGE_ALPHA_BLEND_COMPOSE );

        if( GetGRForceBlackPenState() )
            image = image.ConvertToGreyscale();

        aDC->DrawBitmap( wxBitmap( image ), pos.x, pos.y, true );
    }
    else if( GetGRForceBlackPenState() )
    {
        wxBitmap result( bitmap.ConvertToImage().ConvertToGreyscale() );
        aDC->DrawBitmap( result, pos.x, pos.y, true );
    }
    else
    {
        aDC->DrawBitmap( bitmap, pos.x, pos.y, true );
    }

    if( useTransform )
        aDC->SetTransformMatrix( init_matrix );
    else
    {
        aDC->SetUserScale( scale, scale );
        aDC->SetLogicalOrigin( logicalOriginX, logicalOriginY );
    }

#ifdef USE_CLIP_AREA
    aDC->DestroyClippingRegion();
#endif
}
