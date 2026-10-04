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

#include <plotters/plotter.h>
#include <trigo.h>
#include <math/util.h>          // for KiROUND
#include <font/font.h>
#include <text_eval/text_eval_wrapper.h>



int GetPenSizeForBold( int aTextSize )
{
    return KiROUND( aTextSize / 5.0 );
}


int GetPenSizeForDemiBold( int aTextSize )
{
    return KiROUND( aTextSize / 6.0 );
}


int GetPenSizeForBold( const VECTOR2I& aTextSize )
{
    return GetPenSizeForBold( std::min( aTextSize.x, aTextSize.y ) );
}


int GetPenSizeForDemiBold( const VECTOR2I& aTextSize )
{
    return GetPenSizeForDemiBold( std::min( aTextSize.x, aTextSize.y ) );
}


int GetPenSizeForNormal( int aTextSize )
{
    return KiROUND( aTextSize / 8.0 );
}


int GetPenSizeForNormal( const VECTOR2I& aTextSize )
{
    return GetPenSizeForNormal( std::min( aTextSize.x, aTextSize.y ) );
}


int ClampTextPenSize( int aPenSize, int aSize, bool aStrict )
{
    double scale    = aStrict ? 0.18 : 0.25;
    int    maxWidth = KiROUND( (double) aSize * scale );

    return std::min( aPenSize, maxWidth );
}


float ClampTextPenSize( float aPenSize, int aSize, bool aStrict )
{
    double scale    = aStrict ? 0.18 : 0.25;
    float maxWidth = (float) aSize * scale;

    return std::min( aPenSize, maxWidth );
}


int ClampTextPenSize( int aPenSize, const VECTOR2I& aSize, bool aStrict )
{
    int size = std::min( std::abs( aSize.x ), std::abs( aSize.y ) );

    return ClampTextPenSize( aPenSize, size, aStrict );
}


int GRTextWidth( const wxString& aText, KIFONT::FONT* aFont, const VECTOR2I& aSize,
                 int aThickness, bool aBold, bool aItalic, const KIFONT::METRICS& aFontMetrics )
{
    if( !aFont )
        aFont = KIFONT::FONT::GetFont();
    wxString evaluated( aText );

    if( evaluated.Contains( wxS( "@{" ) ) )
    {
        EXPRESSION_EVALUATOR evaluator;
        evaluated = evaluator.Evaluate( evaluated );
    }

    return KiROUND( aFont->StringBoundaryLimits( evaluated, aSize, aThickness, aBold, aItalic,
                                                 aFontMetrics ).x );
}
