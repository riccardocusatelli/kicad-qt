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

/**
 * @file numformatter.h
 * The numbers in the format of the locale (wxNumberFormatter of wxWidgets): the separators of
 * the C locale of the program.
 */

#include <clocale>
#include <cstdlib>

#include <wx/string.h>


class wxNumberFormatter
{
public:
    enum Style
    {
        Style_None = 0x00,
        Style_WithThousandsSep = 0x01,
        Style_NoTrailingZeroes = 0x02
    };

    static wxChar GetDecimalSeparator()
    {
        const std::lconv* conv = std::localeconv();

        return conv && conv->decimal_point && conv->decimal_point[0]
                       ? wxChar( conv->decimal_point[0] )
                       : wxChar( '.' );
    }

    static bool GetThousandsSeparatorIfUsed( wxChar* aSep )
    {
        const std::lconv* conv = std::localeconv();

        if( !conv || !conv->thousands_sep || !conv->thousands_sep[0] )
            return false;

        if( aSep )
            *aSep = wxChar( conv->thousands_sep[0] );

        return true;
    }

    static wxString ToString( long aValue, int aStyle = Style_None )
    {
        return wxString::Format( wxS( "%ld" ), aValue );
    }

    static wxString ToString( double aValue, int aPrecision, int aStyle = Style_None )
    {
        wxString str = wxString::Format( wxS( "%.*f" ), aPrecision, aValue );

        if( ( aStyle & Style_NoTrailingZeroes ) && str.Contains( GetDecimalSeparator() ) )
        {
            while( str.EndsWith( wxS( "0" ) ) )
                str.RemoveLast();

            if( str.EndsWith( wxString( GetDecimalSeparator() ) ) )
                str.RemoveLast();
        }

        return str;
    }

    static bool FromString( const wxString& aText, long* aValue )
    {
        return aText.ToLong( aValue );
    }

    static bool FromString( const wxString& aText, double* aValue )
    {
        return aText.ToDouble( aValue );
    }
};
