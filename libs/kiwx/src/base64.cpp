/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Derived from wxWidgets 3.2.4 (src/common/base64.cpp, wxWindows Library Licence 3.1),
 * Copyright (C) the wxWidgets authors.
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

#include <wx/base64.h>

#include <cstring>


size_t wxBase64Encode( char* aDst, size_t aDstLen, const void* aSrc, size_t aSrcLen )
{
    if( !aSrc )
        return wxCONV_FAILED;

    const unsigned char* src = static_cast<const unsigned char*>( aSrc );

    static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    size_t encLen = 0;

    for( ; aSrcLen >= 3; aSrcLen -= 3, src += 3 )
    {
        encLen += 4;

        if( aDst )
        {
            if( encLen > aDstLen )
                return wxCONV_FAILED;

            *aDst++ = b64[src[0] >> 2];
            *aDst++ = b64[( ( src[0] & 0x03 ) << 4 ) | ( ( src[1] & 0xf0 ) >> 4 )];
            *aDst++ = b64[( ( src[1] & 0x0f ) << 2 ) | ( ( src[2] & 0xc0 ) >> 6 )];
            *aDst++ = b64[src[2] & 0x3f];
        }
    }

    if( aSrcLen )
    {
        encLen += 4;

        if( aDst )
        {
            if( encLen > aDstLen )
                return wxCONV_FAILED;

            unsigned char next = aSrcLen == 2 ? src[1] : 0;
            *aDst++ = b64[src[0] >> 2];
            *aDst++ = b64[( ( src[0] & 0x03 ) << 4 ) | ( ( next & 0xf0 ) >> 4 )];
            *aDst++ = aSrcLen == 2 ? b64[( ( next & 0x0f ) << 2 )] : '=';
            *aDst = '=';
        }
    }

    return encLen;
}


size_t wxBase64Decode( void* aDst, size_t aDstLen, const char* aSrc, size_t aSrcLen,
                       wxBase64DecodeMode aMode, size_t* aPosErr )
{
    if( !aSrc )
        return wxCONV_FAILED;

    unsigned char* dst = static_cast<unsigned char*>( aDst );
    size_t         decLen = 0;

    if( aSrcLen == wxNO_LEN )
        aSrcLen = std::strlen( aSrc );

    // values of the characters, or white space, invalid and padding markers
    enum
    {
        WSP = 200,
        INV,
        PAD
    };

    static const unsigned char decode[256] = {
        WSP, INV, INV, INV, INV, INV, INV, INV, INV, WSP, WSP, INV, WSP, WSP, INV, INV,
        INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV,
        WSP, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, 076, INV, INV, INV, 077,
        064, 065, 066, 067, 070, 071, 072, 073, 074, 075, INV, INV, INV, PAD, INV, INV,
        INV, 000, 001, 002, 003, 004, 005, 006, 007, 010, 011, 012, 013, 014, 015, 016,
        017, 020, 021, 022, 023, 024, 025, 026, 027, 030, 031, INV, INV, INV, INV, INV,
        INV, 032, 033, 034, 035, 036, 037, 040, 041, 042, 043, 044, 045, 046, 047, 050,
        051, 052, 053, 054, 055, 056, 057, 060, 061, 062, 063, INV, INV, INV, INV, INV,
        INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV,
        INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV,
        INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV,
        INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV,
        INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV,
        INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV,
        INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV,
        INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV, INV,
    };

    int           n = 0;          // position in the current quartet
    unsigned char in[4];
    bool          end = false;    // padding seen
    size_t        padLen = 0;
    const char*   p;

    for( p = aSrc; aSrcLen; p++, aSrcLen-- )
    {
        const unsigned char c = decode[static_cast<unsigned char>( *p )];

        switch( c )
        {
        case WSP:
            if( aMode == wxBase64DecodeMode_SkipWS )
                continue;

            [[fallthrough]];

        case INV:
            if( aMode == wxBase64DecodeMode_Relaxed )
                continue;

            n = -1;
            aSrcLen = 1;
            break;

        case PAD:
            end = true;

            if( n == 3 )
            {
                padLen = 1;
                in[n++] = '\0';
            }
            else if( ( n == 2 ) && ( --aSrcLen && *++p == '=' ) )
            {
                padLen = 2;
                in[n++] = '\0';
                in[n++] = '\0';
            }
            else
            {
                n = -1;
                aSrcLen = 1;
            }

            break;

        default:
            if( end )
            {
                n = -1;
                aSrcLen = 1;
                break;
            }

            in[n++] = c;
        }

        if( n == 4 )
        {
            decLen += 3 - padLen;

            if( dst )
            {
                if( decLen > aDstLen )
                    return wxCONV_FAILED;

                *dst++ = in[0] << 2 | in[1] >> 4;

                if( padLen != 2 )
                {
                    *dst++ = in[1] << 4 | in[2] >> 2;

                    if( !padLen )
                        *dst++ = in[2] << 6 | in[3];
                }
            }

            n = 0;
        }
    }

    if( n )
    {
        if( aPosErr )
            *aPosErr = p - aSrc - 1;

        return wxCONV_FAILED;
    }

    return decLen;
}


wxMemoryBuffer wxBase64Decode( const char* aSrc, size_t aSrcLen, wxBase64DecodeMode aMode,
                               size_t* aPosErr )
{
    wxMemoryBuffer buf;

    if( !aSrc )
        return buf;

    if( aSrcLen == wxNO_LEN )
        aSrcLen = std::strlen( aSrc );

    size_t len = wxBase64DecodedSize( aSrcLen );
    len = wxBase64Decode( buf.GetWriteBuf( len ), len, aSrc, aSrcLen, aMode, aPosErr );

    if( len == wxCONV_FAILED )
        len = 0;

    buf.SetDataLen( len );
    return buf;
}
