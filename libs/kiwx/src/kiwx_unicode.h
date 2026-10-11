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
 * @file kiwx_unicode.h
 *
 * Code points in wide strings.  wchar_t holds a whole code point where it is 32 bit (Linux,
 * macOS) and a UTF-16 code unit on Windows, where the code points above U+FFFF take a
 * surrogate pair, as in wxWidgets (wxUSE_UNICODE_WCHAR).
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace kiwx
{

constexpr bool WCHAR_IS_UTF16 = sizeof( wchar_t ) == 2;

inline bool IsHighSurrogate( uint32_t aUnit )
{
    return aUnit >= 0xD800 && aUnit <= 0xDBFF;
}

inline bool IsLowSurrogate( uint32_t aUnit )
{
    return aUnit >= 0xDC00 && aUnit <= 0xDFFF;
}

/// Append the code point @a aCode (at most U+10FFFF) to @a aOut.
inline void AppendCodePoint( std::wstring& aOut, uint32_t aCode )
{
    if( WCHAR_IS_UTF16 && aCode >= 0x10000 )
    {
        aCode -= 0x10000;
        aOut += static_cast<wchar_t>( 0xD800 + ( aCode >> 10 ) );
        aOut += static_cast<wchar_t>( 0xDC00 + ( aCode & 0x3FF ) );
    }
    else
    {
        aOut += static_cast<wchar_t>( aCode );
    }
}

/**
 * The code point at @a aPos of @a aStr (@a aLen units long), advancing @a aPos past it.  With
 * UTF-16 a surrogate pair gives one code point; a lone surrogate is returned as it is.
 */
inline uint32_t NextCodePoint( const wchar_t* aStr, size_t aLen, size_t& aPos )
{
    uint32_t c = static_cast<uint32_t>( aStr[aPos++] );

    if( WCHAR_IS_UTF16 )
    {
        c &= 0xFFFF;

        if( IsHighSurrogate( c ) && aPos < aLen )
        {
            uint32_t low = static_cast<uint32_t>( aStr[aPos] ) & 0xFFFF;

            if( IsLowSurrogate( low ) )
            {
                ++aPos;
                c = 0x10000 + ( ( c - 0xD800 ) << 10 ) + ( low - 0xDC00 );
            }
        }
    }

    return c;
}

} // namespace kiwx
