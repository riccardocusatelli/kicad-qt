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
 * @file eda_text_print.cpp
 * @brief EDA_TEXT::Print(): text on a wxDC (printing), GUI only.
 */

#include <eda_text.h>

#include <gr_text.h>
#include <string_utils.h>
#include <render_settings.h>
#include <wx/dc.h>


void EDA_TEXT::Print( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset, const COLOR4D& aColor )
{
    if( IsMultilineAllowed() )
    {
        std::vector<VECTOR2I> positions;
        wxArrayString         strings;
        wxStringSplit( GetShownText( true ), strings, '\n' );

        positions.reserve( strings.Count() );

        GetLinePositions( aSettings, positions, (int) strings.Count() );

        for( unsigned ii = 0; ii < strings.Count(); ii++ )
            printOneLineOfText( aSettings, aOffset, aColor, strings[ii], positions[ii] );
    }
    else
    {
        printOneLineOfText( aSettings, aOffset, aColor, GetShownText( true ), GetDrawPos() );
    }
}


void EDA_TEXT::printOneLineOfText( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset, const COLOR4D& aColor,
                                   const wxString& aText, const VECTOR2I& aPos )
{
    wxDC* DC = aSettings->GetPrintDC();
    int   penWidth = GetEffectiveTextPenWidth( aSettings->GetDefaultPenWidth() );

    VECTOR2I size = GetTextSize();

    if( IsMirrored() )
        size.x = -size.x;

    KIFONT::FONT* font = GetDrawFont( aSettings );

    GRPrintText( DC, aOffset + aPos, aColor, aText, GetDrawRotation(), size, GetHorizJustify(), GetVertJustify(),
                 penWidth, IsItalic(), IsBold(), font, getFontMetrics() );
}
