/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@gmail.com>
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
 * @file gr_text_print.cpp
 * @brief GRPrintText(): text on a wxDC (printing), GUI only.
 */

#include <gr_text.h>

#include <callback_gal.h>
#include <font/font.h>
#include <gal/gal_display_options.h>
#include <gr_basic.h>
#include <text_eval/text_eval_wrapper.h>


void GRPrintText( wxDC* aDC, const VECTOR2I& aPos, const COLOR4D& aColor, const wxString& aText,
                  const EDA_ANGLE& aOrient, const VECTOR2I& aSize,
                  enum GR_TEXT_H_ALIGN_T aH_justify, enum GR_TEXT_V_ALIGN_T aV_justify,
                  int aWidth, bool aItalic, bool aBold, KIFONT::FONT* aFont,
                  const KIFONT::METRICS& aFontMetrics )
{
    KIGFX::GAL_DISPLAY_OPTIONS empty_opts;
    bool                       fill_mode = true;
    wxString                   evaluatedText( aText );

    if( evaluatedText.Contains( wxS( "@{" ) ) )
    {
        EXPRESSION_EVALUATOR evaluator;
        evaluatedText = evaluator.Evaluate( evaluatedText );
    }

    if( !aFont )
        aFont = KIFONT::FONT::GetFont();

    if( aWidth == 0 ) // Use default values if aWidth == 0
    {
        if( aBold )
            aWidth = GetPenSizeForBold( std::min( aSize.x, aSize.y ) );
        else
            aWidth = GetPenSizeForNormal( std::min( aSize.x, aSize.y ) );
    }

    if( aWidth < 0 )
    {
        aWidth = -aWidth;
        fill_mode = false;
    }

    CALLBACK_GAL callback_gal( empty_opts,
            // Stroke callback
            [&]( const VECTOR2I& aPt1, const VECTOR2I& aPt2 )
            {
                if( fill_mode )
                    GRLine( aDC, aPt1, aPt2, aWidth, aColor );
                else
                    GRCSegm( aDC, aPt1, aPt2, aWidth, aColor );
            },
            // Polygon callback
            [&]( const SHAPE_LINE_CHAIN& aPoly )
            {
                GRClosedPoly( aDC, aPoly.PointCount(), aPoly.CPoints().data(), true, aColor );
            } );

    TEXT_ATTRIBUTES attributes;
    attributes.m_Angle = aOrient;
    attributes.m_StrokeWidth = aWidth;
    attributes.m_Italic = aItalic;
    attributes.m_Bold = aBold;
    attributes.m_Halign = aH_justify;
    attributes.m_Valign = aV_justify;
    attributes.m_Size = aSize;

    aFont->Draw( &callback_gal, evaluatedText, aPos, attributes, aFontMetrics );
}
