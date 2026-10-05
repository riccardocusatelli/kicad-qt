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

/**
 * @file kiwx_diff.h
 *
 * Output helpers of the kiwx differential tests: every value is printed in a form that does
 * not depend on the implementation (strings as escaped UTF-8, arrays as lists).
 */

#pragma once

#include <wx/arrstr.h>
#include <wx/string.h>

#include <clocale>
#include <cstdio>
#include <string>
#include <type_traits>


inline std::string kiwxDiffEscape( const wxString& aStr )
{
    std::string out = "\"";

    for( wxUniChar c : aStr )
    {
        unsigned v = c.GetValue();

        if( v == '"' || v == '\\' )
        {
            out += '\\';
            out += static_cast<char>( v );
        }
        else if( v >= 0x20 && v < 0x7F )
        {
            out += static_cast<char>( v );
        }
        else
        {
            char buf[16];
            std::snprintf( buf, sizeof( buf ), "\\u{%x}", v );
            out += buf;
        }
    }

    return out + "\"";
}


inline std::string kiwxDiffValue( const wxString& aValue )
{
    return kiwxDiffEscape( aValue );
}

inline std::string kiwxDiffValue( const char* aValue )
{
    return aValue ? kiwxDiffEscape( wxString::FromUTF8( aValue ) ) : "(null)";
}

inline std::string kiwxDiffValue( const std::string& aValue )
{
    return kiwxDiffEscape( wxString::FromUTF8( aValue.c_str(), aValue.size() ) ) + "/"
           + std::to_string( aValue.size() );
}

inline std::string kiwxDiffValue( const wxArrayString& aValue )
{
    std::string out = "[";

    for( size_t i = 0; i < aValue.size(); ++i )
        out += ( i ? ", " : "" ) + kiwxDiffEscape( aValue[i] );

    return out + "]";
}

inline std::string kiwxDiffValue( const wxUniChar& aValue )
{
    return "char " + std::to_string( aValue.GetValue() );
}

inline std::string kiwxDiffValue( const wxUniCharRef& aValue )
{
    return kiwxDiffValue( wxUniChar( aValue ) );
}

inline std::string kiwxDiffValue( bool aValue )
{
    return aValue ? "true" : "false";
}

inline std::string kiwxDiffValue( double aValue )
{
    char buf[64];
    std::snprintf( buf, sizeof( buf ), "%.17g", aValue );
    return buf;
}

template <typename T, typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>>>
inline std::string kiwxDiffValue( T aValue )
{
    return std::to_string( aValue );
}


// As a wxWidgets application does at start-up (the char conversions depend on the locale).
inline const bool kiwxDiffLocale = std::setlocale( LC_ALL, "" ) != nullptr;


#define SECTION( name ) std::printf( "== %s\n", name )
#define OUT( expr )                                                                            \
    std::printf( "%d %s: %s\n", __LINE__, #expr, kiwxDiffValue( expr ).c_str() )
