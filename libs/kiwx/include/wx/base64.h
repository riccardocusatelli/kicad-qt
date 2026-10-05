/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Derived from wxWidgets 3.2.4 (include/wx/base64.h, wxWindows Library Licence 3.1),
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

#pragma once

// exported from the kiwx library, as the wxWidgets API (WXDLLIMPEXP_BASE)
#pragma GCC visibility push( default )

#include <wx/buffer.h>
#include <wx/strconv.h>
#include <wx/string.h>

inline size_t wxBase64EncodedSize( size_t aLen )
{
    return 4 * ( ( aLen + 2 ) / 3 );
}

/// Encodes into a buffer, returns the encoded length or wxCONV_FAILED if too small (a null
/// buffer gives the needed size).
size_t wxBase64Encode( char* aDst, size_t aDstLen, const void* aSrc, size_t aSrcLen );

inline wxString wxBase64Encode( const void* aSrc, size_t aSrcLen )
{
    const size_t dstLen = wxBase64EncodedSize( aSrcLen );
    std::string  dst( dstLen, '\0' );
    wxBase64Encode( dst.data(), dstLen, aSrc, aSrcLen );
    return wxString::FromAscii( dst.c_str() );
}

inline wxString wxBase64Encode( const wxMemoryBuffer& aBuf )
{
    return wxBase64Encode( aBuf.GetData(), aBuf.GetDataLen() );
}


enum wxBase64DecodeMode
{
    wxBase64DecodeMode_Strict,  ///< stop at any invalid character
    wxBase64DecodeMode_SkipWS,  ///< skip white space
    wxBase64DecodeMode_Relaxed  ///< skip all invalid characters
};

inline size_t wxBase64DecodedSize( size_t aSrcLen )
{
    return 3 * aSrcLen / 4;
}

size_t wxBase64Decode( void* aDst, size_t aDstLen, const char* aSrc, size_t aSrcLen = wxNO_LEN,
                       wxBase64DecodeMode aMode = wxBase64DecodeMode_Strict,
                       size_t* aPosErr = nullptr );

inline size_t wxBase64Decode( void* aDst, size_t aDstLen, const wxString& aSrc,
                              wxBase64DecodeMode aMode = wxBase64DecodeMode_Strict,
                              size_t* aPosErr = nullptr )
{
    return wxBase64Decode( aDst, aDstLen, aSrc.ToAscii(), wxNO_LEN, aMode, aPosErr );
}

wxMemoryBuffer wxBase64Decode( const char* aSrc, size_t aSrcLen = wxNO_LEN,
                               wxBase64DecodeMode aMode = wxBase64DecodeMode_Strict,
                               size_t* aPosErr = nullptr );

inline wxMemoryBuffer wxBase64Decode( const wxString& aSrc,
                                      wxBase64DecodeMode aMode = wxBase64DecodeMode_Strict,
                                      size_t* aPosErr = nullptr )
{
    return wxBase64Decode( aSrc.ToAscii(), wxNO_LEN, aMode, aPosErr );
}

#pragma GCC visibility pop
