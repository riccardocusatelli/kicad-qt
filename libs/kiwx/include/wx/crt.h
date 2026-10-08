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

// exported from the kiwx library, as the wxWidgets API (WXDLLIMPEXP_BASE)
#pragma GCC visibility push( default )

#include <wx/string.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <cwctype>

// The C runtime functions of wxWidgets (wide character versions; KiCad's strings are wide).

inline size_t wxStrlen( const char* s ) { return s ? std::strlen( s ) : 0; }
inline size_t wxStrlen( const wchar_t* s ) { return s ? std::wcslen( s ) : 0; }
inline size_t wxStrlen( const wxCStrData& s ) { return std::wcslen( s.AsWChar() ); }

inline int wxStrcmp( const wchar_t* a, const wchar_t* b ) { return std::wcscmp( a, b ); }
inline int wxStrcmp( const char* a, const char* b ) { return std::strcmp( a, b ); }
inline int wxStrcmp( const wxString& a, const wxString& b ) { return a.Cmp( b ); }
inline int wxStrncmp( const wchar_t* a, const wchar_t* b, size_t n ) { return std::wcsncmp( a, b, n ); }
inline int wxStrncmp( const char* a, const char* b, size_t n ) { return std::strncmp( a, b, n ); }
inline int wxStrncmp( const wxString& a, const wxString& b, size_t n ) { return std::wcsncmp( a.wc_str(), b.wc_str(), n ); }
inline int wxStricmp( const wxString& a, const wxString& b ) { return a.CmpNoCase( b ); }
inline int wxStrnicmp( const wxString& a, const wxString& b, size_t n ) { return a.Left( n ).CmpNoCase( b.Left( n ) ); }

inline const wchar_t* wxStrchr( const wchar_t* s, wchar_t c ) { return std::wcschr( s, c ); }
inline const char*    wxStrchr( const char* s, char c ) { return std::strchr( s, c ); }
inline const wchar_t* wxStrrchr( const wchar_t* s, wchar_t c ) { return std::wcsrchr( s, c ); }
inline const wchar_t* wxStrstr( const wchar_t* s, const wchar_t* t ) { return std::wcsstr( s, t ); }
inline const char*    wxStrstr( const char* s, const char* t ) { return std::strstr( s, t ); }
inline wchar_t*       wxStrcpy( wchar_t* d, const wchar_t* s ) { return std::wcscpy( d, s ); }
inline char*          wxStrcpy( char* d, const char* s ) { return std::strcpy( d, s ); }
inline wchar_t*       wxStrncpy( wchar_t* d, const wchar_t* s, size_t n ) { return std::wcsncpy( d, s, n ); }
inline char*          wxStrncpy( char* d, const char* s, size_t n ) { return std::strncpy( d, s, n ); }
inline char*          wxStrdup( const char* s ) { return ::strdup( s ); }
inline wchar_t*       wxStrdup( const wchar_t* s ) { return ::wcsdup( s ); }

inline long          wxStrtol( const wchar_t* s, wchar_t** e, int b ) { return std::wcstol( s, e, b ); }
inline long          wxStrtol( const char* s, char** e, int b ) { return std::strtol( s, e, b ); }
inline unsigned long wxStrtoul( const wchar_t* s, wchar_t** e, int b ) { return std::wcstoul( s, e, b ); }
inline unsigned long wxStrtoul( const char* s, char** e, int b ) { return std::strtoul( s, e, b ); }
inline double        wxStrtod( const wchar_t* s, wchar_t** e ) { return std::wcstod( s, e ); }
inline double        wxStrtod( const char* s, char** e ) { return std::strtod( s, e ); }
inline long long     wxStrtoll( const wchar_t* s, wchar_t** e, int b ) { return std::wcstoll( s, e, b ); }

inline int    wxAtoi( const wxString& s ) { return static_cast<int>( std::wcstol( s.wc_str(), nullptr, 10 ) ); }
inline long   wxAtol( const wxString& s ) { return std::wcstol( s.wc_str(), nullptr, 10 ); }
inline double wxAtof( const wxString& s ) { return std::wcstod( s.wc_str(), nullptr ); }

inline bool wxIsdigit( wxUniChar c ) { return std::iswdigit( static_cast<wint_t>( c.GetValue() ) ); }
inline bool wxIsalpha( wxUniChar c ) { return std::iswalpha( static_cast<wint_t>( c.GetValue() ) ); }
inline bool wxIsalnum( wxUniChar c ) { return std::iswalnum( static_cast<wint_t>( c.GetValue() ) ); }
inline bool wxIsspace( wxUniChar c ) { return std::iswspace( static_cast<wint_t>( c.GetValue() ) ); }
inline bool wxIsupper( wxUniChar c ) { return std::iswupper( static_cast<wint_t>( c.GetValue() ) ); }
inline bool wxIslower( wxUniChar c ) { return std::iswlower( static_cast<wint_t>( c.GetValue() ) ); }
inline bool wxIspunct( wxUniChar c ) { return std::iswpunct( static_cast<wint_t>( c.GetValue() ) ); }
inline bool wxIsprint( wxUniChar c ) { return std::iswprint( static_cast<wint_t>( c.GetValue() ) ); }
inline bool wxIsxdigit( wxUniChar c ) { return std::iswxdigit( static_cast<wint_t>( c.GetValue() ) ); }
inline bool wxIscntrl( wxUniChar c ) { return std::iswcntrl( static_cast<wint_t>( c.GetValue() ) ); }
inline wxChar wxToupper( wxUniChar c ) { return static_cast<wxChar>( std::towupper( static_cast<wint_t>( c.GetValue() ) ) ); }
inline wxChar wxTolower( wxUniChar c ) { return static_cast<wxChar>( std::towlower( static_cast<wint_t>( c.GetValue() ) ) ); }

inline FILE* wxFopen( const wxString& aPath, const wxString& aMode )
{
    return std::fopen( aPath.utf8_string().c_str(), aMode.utf8_string().c_str() );
}

inline int wxRemove( const wxString& aPath )
{
    return std::remove( aPath.utf8_string().c_str() );
}

inline int wxRename( const wxString& aFrom, const wxString& aTo )
{
    return std::rename( aFrom.utf8_string().c_str(), aTo.utf8_string().c_str() );
}

inline wxString wxGetenv( const wxString& aVar )
{
    const char* v = std::getenv( aVar.utf8_string().c_str() );
    return v ? wxString( v ) : wxString();
}

/// @return true if a string is null or empty.
inline bool wxIsEmpty( const char* aStr ) { return !aStr || !*aStr; }
inline bool wxIsEmpty( const wchar_t* aStr ) { return !aStr || !*aStr; }
inline bool wxIsEmpty( const wxString& aStr ) { return aStr.IsEmpty(); }

inline int wxSystem( const wxString& aCmd )
{
    return std::system( aCmd.utf8_string().c_str() );
}

/// printf-like output (the text is written as UTF-8).
template <typename... ARGS>
int wxFprintf( FILE* aFile, const wxString& aFormat, const ARGS&... aArgs )
{
    std::string text = wxString::Format( aFormat, aArgs... ).utf8_string();
    return std::fputs( text.c_str(), aFile ) < 0 ? -1 : static_cast<int>( text.size() );
}

template <typename... ARGS>
int wxPrintf( const wxString& aFormat, const ARGS&... aArgs )
{
    return wxFprintf( stdout, aFormat, aArgs... );
}

template <typename... ARGS>
int wxSnprintf( wchar_t* aBuf, size_t aLen, const wxString& aFormat, const ARGS&... aArgs )
{
    wxString text = wxString::Format( aFormat, aArgs... );
    size_t   n = std::min( text.length(), aLen ? aLen - 1 : 0 );

    if( aLen )
    {
        std::wmemcpy( aBuf, text.wc_str(), n );
        aBuf[n] = 0;
    }

    return static_cast<int>( text.length() );
}

template <typename... ARGS>
int wxSnprintf( char* aBuf, size_t aLen, const wxString& aFormat, const ARGS&... aArgs )
{
    std::string text = wxString::Format( aFormat, aArgs... ).utf8_string();
    size_t      n = std::min( text.size(), aLen ? aLen - 1 : 0 );

    if( aLen )
    {
        std::memcpy( aBuf, text.data(), n );
        aBuf[n] = 0;
    }

    return static_cast<int>( text.size() );
}

inline int wxPuts( const wxString& aStr )
{
    return std::puts( aStr.utf8_string().c_str() );
}

template <typename... ARGS>
int wxSscanf( const wxString& aStr, const wchar_t* aFormat, ARGS... aArgs )
{
    return std::swscanf( aStr.wc_str(), aFormat, aArgs... );
}

template <typename... ARGS>
int wxSscanf( const char* aStr, const char* aFormat, ARGS... aArgs )
{
    return std::sscanf( aStr, aFormat, aArgs... );
}

#pragma GCC visibility pop
