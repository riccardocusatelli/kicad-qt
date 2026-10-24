/*
 * Copyright (C) 2019 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Author: Maciej Suminski <maciej.suminski@cern.ch>
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

/**
 * @file cairo_print.cpp
 * The Cairo GAL of the printouts.  Its context (the Cairo surface of a printer or of an image)
 * depends on the GUI toolkit: see cairo_print_ctx_wx.cpp.
 */

#include <gal/cairo/cairo_print.h>

#include <cmath>

using namespace KIGFX;


CAIRO_PRINT_GAL::CAIRO_PRINT_GAL( GAL_DISPLAY_OPTIONS&             aDisplayOptions,
                                  std::unique_ptr<CAIRO_PRINT_CTX> aContext ) :
        CAIRO_GAL_BASE( aDisplayOptions )
{
    m_printCtx = std::move( aContext );
    m_context = m_currentContext = m_printCtx->GetContext();
    m_surface = m_printCtx->GetSurface();
    cairo_reference( m_context );
    cairo_surface_reference( m_surface );
    m_clearColor = COLOR4D( 1.0, 1.0, 1.0, 1.0 );
    m_hasNativeLandscapeRotation = false;
    resetContext();

    SetScreenDPI( m_printCtx->GetNativeDPI() );
}


void CAIRO_PRINT_GAL::ComputeWorldScreenMatrix()
{
    m_worldScale = m_screenDPI * m_worldUnitLength * m_zoomFactor;
    const VECTOR2D paperSizeIU = VECTOR2D( m_nativePaperSize.y, m_nativePaperSize.x ) /* inches */
                                 / m_worldUnitLength;                                 /* 1" in IU */
    const VECTOR2D paperSizeIUTransposed( paperSizeIU.y, paperSizeIU.x );

    MATRIX3x3D scale, translation, flip, rotate, lookat;

    scale.SetIdentity();
    translation.SetIdentity();
    flip.SetIdentity();
    rotate.SetIdentity();
    lookat.SetIdentity();

    if( m_hasNativeLandscapeRotation )
    {
        translation.SetTranslation( 0.5 / m_zoomFactor * paperSizeIUTransposed );
    }
    else
    {
        if( isLandscape() )
        {
            translation.SetTranslation( 0.5 / m_zoomFactor * paperSizeIU );
            rotate.SetRotation( 90.0 * M_PI / 180.0 );
        }
        else
        {
            translation.SetTranslation( 0.5 / m_zoomFactor * paperSizeIUTransposed );
        }
    }

    scale.SetScale( VECTOR2D( m_worldScale, m_worldScale ) );
    flip.SetScale( VECTOR2D( m_globalFlipX ? -1.0 : 1.0, m_globalFlipY ? -1.0 : 1.0 ) );
    lookat.SetTranslation( -m_lookAtPoint );

    m_worldScreenMatrix = scale * translation * flip * rotate * lookat;
    m_screenWorldMatrix = m_worldScreenMatrix.Inverse();
}


void CAIRO_PRINT_GAL::SetNativePaperSize( const VECTOR2D& aSize, bool aHasNativeLandscapeRotation )
{
    m_nativePaperSize = aSize;
    m_hasNativeLandscapeRotation = aHasNativeLandscapeRotation;
}


void CAIRO_PRINT_GAL::SetSheetSize( const VECTOR2D& aSize )
{
    // Convert aSize (inches) to pixels
    SetScreenSize( VECTOR2I( std::ceil( aSize.x * m_screenDPI ) * 2,
                             std::ceil( aSize.y * m_screenDPI ) * 2 ) );
}


std::unique_ptr<GAL_PRINT> GAL_PRINT::Create( GAL_DISPLAY_OPTIONS& aOptions, wxDC* aDC )
{
    auto printCtx = std::make_unique<CAIRO_PRINT_CTX>( aDC );
    return std::make_unique<CAIRO_PRINT_GAL>( aOptions, std::move( printCtx ) );
}


std::unique_ptr<CAIRO_PRINT_GAL> CAIRO_PRINT_GAL::Create( GAL_DISPLAY_OPTIONS& aOptions, wxImage* aImage, double aDPI )
{
    auto printCtx = std::make_unique<CAIRO_PRINT_CTX>( aImage, aDPI );
    return std::make_unique<CAIRO_PRINT_GAL>( aOptions, std::move( printCtx ) );
}