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
#ifndef _WIN32
#pragma GCC visibility push( default )
#endif

#include <wx/defs.h>

#include <cwctype>


/**
 * A Unicode character (a code point), as wxUniChar: converts from and to the C character
 * types; a char is taken as ISO-8859-1 (bytes above 127 never occur in KiCad's literals).
 */
class wxUniChar
{
public:
    typedef wxUint32 value_type;

    constexpr wxUniChar() : m_value( 0 ) {}
    constexpr wxUniChar( char c ) : m_value( static_cast<unsigned char>( c ) ) {}
    constexpr wxUniChar( unsigned char c ) : m_value( c ) {}
    constexpr wxUniChar( signed char c ) : m_value( static_cast<unsigned char>( c ) ) {}
    constexpr wxUniChar( wchar_t c ) : m_value( static_cast<value_type>( c ) ) {}
    constexpr wxUniChar( char16_t c ) : m_value( c ) {}
    constexpr wxUniChar( char32_t c ) : m_value( c ) {}
    constexpr wxUniChar( int c ) : m_value( static_cast<value_type>( c ) ) {}
    constexpr wxUniChar( unsigned int c ) : m_value( c ) {}
    constexpr wxUniChar( long c ) : m_value( static_cast<value_type>( c ) ) {}
    constexpr wxUniChar( unsigned long c ) : m_value( static_cast<value_type>( c ) ) {}
    constexpr wxUniChar( short c ) : m_value( static_cast<value_type>( c ) ) {}
    constexpr wxUniChar( unsigned short c ) : m_value( c ) {}
    constexpr wxUniChar( long long c ) : m_value( static_cast<value_type>( c ) ) {}
    constexpr wxUniChar( unsigned long long c ) : m_value( static_cast<value_type>( c ) ) {}

    constexpr value_type GetValue() const { return m_value; }

    constexpr bool IsAscii() const { return m_value < 0x80; }
    constexpr bool IsBMP() const { return m_value < 0x10000; }
    constexpr bool IsSupplementary() const { return m_value >= 0x10000 && m_value < 0x110000; }

    bool GetAsChar( char* c ) const
    {
        if( m_value > 0xFF )
            return false;

        *c = static_cast<char>( m_value );
        return true;
    }

    // The conversions of wxUniChar (wxWidgets has the same set).
    constexpr operator char() const { return static_cast<char>( m_value ); }
    constexpr operator unsigned char() const { return static_cast<unsigned char>( m_value ); }
    constexpr operator wchar_t() const { return static_cast<wchar_t>( m_value ); }
    constexpr operator int() const { return static_cast<int>( m_value ); }
    constexpr operator unsigned int() const { return m_value; }
    constexpr operator long() const { return static_cast<long>( m_value ); }
    constexpr operator unsigned long() const { return m_value; }
    constexpr operator short() const { return static_cast<short>( m_value ); }
    constexpr operator unsigned short() const { return static_cast<unsigned short>( m_value ); }
    constexpr operator long long() const { return m_value; }
    constexpr operator unsigned long long() const { return m_value; }

    constexpr bool operator!() const { return !m_value; }

    // Comparisons with the character types (otherwise ambiguous with the conversions).
#define KIWX_UNICHAR_CMP( T )                                                                  \
    constexpr bool operator==( T c ) const { return m_value == wxUniChar( c ).m_value; }       \
    constexpr bool operator!=( T c ) const { return m_value != wxUniChar( c ).m_value; }       \
    constexpr bool operator<( T c ) const { return m_value < wxUniChar( c ).m_value; }         \
    constexpr bool operator>( T c ) const { return m_value > wxUniChar( c ).m_value; }         \
    constexpr bool operator<=( T c ) const { return m_value <= wxUniChar( c ).m_value; }       \
    constexpr bool operator>=( T c ) const { return m_value >= wxUniChar( c ).m_value; }

    KIWX_UNICHAR_CMP( const wxUniChar& )
    KIWX_UNICHAR_CMP( char )
    KIWX_UNICHAR_CMP( unsigned char )
    KIWX_UNICHAR_CMP( wchar_t )
    KIWX_UNICHAR_CMP( int )
    KIWX_UNICHAR_CMP( unsigned int )
    KIWX_UNICHAR_CMP( long )
    KIWX_UNICHAR_CMP( unsigned long )
    KIWX_UNICHAR_CMP( char16_t )
    KIWX_UNICHAR_CMP( char32_t )

#undef KIWX_UNICHAR_CMP

    constexpr int operator-( const wxUniChar& c ) const
    {
        return static_cast<int>( m_value ) - static_cast<int>( c.m_value );
    }

    constexpr int operator-( char c ) const { return *this - wxUniChar( c ); }
    constexpr int operator-( unsigned char c ) const { return *this - wxUniChar( c ); }
    constexpr int operator-( wchar_t c ) const { return *this - wxUniChar( c ); }

private:
    value_type m_value;
};


#define KIWX_UNICHAR_RCMP( T )                                                                 \
    constexpr bool operator==( T c, const wxUniChar& u ) { return u == c; }                    \
    constexpr bool operator!=( T c, const wxUniChar& u ) { return u != c; }                    \
    constexpr bool operator<( T c, const wxUniChar& u ) { return u > c; }                      \
    constexpr bool operator>( T c, const wxUniChar& u ) { return u < c; }                      \
    constexpr bool operator<=( T c, const wxUniChar& u ) { return u >= c; }                    \
    constexpr bool operator>=( T c, const wxUniChar& u ) { return u <= c; }

KIWX_UNICHAR_RCMP( char )
KIWX_UNICHAR_RCMP( unsigned char )
KIWX_UNICHAR_RCMP( wchar_t )
KIWX_UNICHAR_RCMP( int )
KIWX_UNICHAR_RCMP( unsigned int )
KIWX_UNICHAR_RCMP( long )
KIWX_UNICHAR_RCMP( unsigned long )
KIWX_UNICHAR_RCMP( char16_t )
KIWX_UNICHAR_RCMP( char32_t )

#undef KIWX_UNICHAR_RCMP


/**
 * A reference to a character of a wxString (the result of the non-const operator[]).
 */
class wxUniCharRef
{
public:
    explicit wxUniCharRef( wchar_t& aRef ) : m_ref( aRef ) {}

    wxUniCharRef& operator=( const wxUniChar& c )
    {
        m_ref = static_cast<wchar_t>( c.GetValue() );
        return *this;
    }

    wxUniCharRef& operator=( const wxUniCharRef& c )
    {
        m_ref = c.m_ref;
        return *this;
    }

    wxUniCharRef& operator=( char c ) { return *this = wxUniChar( c ); }
    wxUniCharRef& operator=( unsigned char c ) { return *this = wxUniChar( c ); }
    wxUniCharRef& operator=( wchar_t c ) { return *this = wxUniChar( c ); }
    wxUniCharRef& operator=( int c ) { return *this = wxUniChar( c ); }

    wxUniChar::value_type GetValue() const { return wxUniChar( m_ref ).GetValue(); }
    bool                  IsAscii() const { return wxUniChar( m_ref ).IsAscii(); }

    bool GetAsChar( char* c ) const { return wxUniChar( m_ref ).GetAsChar( c ); }

    operator wxUniChar() const { return wxUniChar( m_ref ); }

    operator char() const { return wxUniChar( m_ref ); }
    operator unsigned char() const { return wxUniChar( m_ref ); }
    operator wchar_t() const { return m_ref; }
    operator int() const { return wxUniChar( m_ref ); }
    operator unsigned int() const { return wxUniChar( m_ref ); }
    operator long() const { return wxUniChar( m_ref ); }
    operator unsigned long() const { return wxUniChar( m_ref ); }

    bool operator!() const { return !m_ref; }

#define KIWX_UNICHARREF_CMP( T )                                                               \
    bool operator==( T c ) const { return wxUniChar( m_ref ) == c; }                           \
    bool operator!=( T c ) const { return wxUniChar( m_ref ) != c; }                           \
    bool operator<( T c ) const { return wxUniChar( m_ref ) < c; }                             \
    bool operator>( T c ) const { return wxUniChar( m_ref ) > c; }                             \
    bool operator<=( T c ) const { return wxUniChar( m_ref ) <= c; }                           \
    bool operator>=( T c ) const { return wxUniChar( m_ref ) >= c; }

    KIWX_UNICHARREF_CMP( const wxUniChar& )
    KIWX_UNICHARREF_CMP( char )
    KIWX_UNICHARREF_CMP( unsigned char )
    KIWX_UNICHARREF_CMP( wchar_t )
    KIWX_UNICHARREF_CMP( int )
    KIWX_UNICHARREF_CMP( unsigned int )
    KIWX_UNICHARREF_CMP( long )
    KIWX_UNICHARREF_CMP( unsigned long )

#undef KIWX_UNICHARREF_CMP

    bool operator==( const wxUniCharRef& c ) const { return m_ref == c.m_ref; }
    bool operator!=( const wxUniCharRef& c ) const { return m_ref != c.m_ref; }
    bool operator<( const wxUniCharRef& c ) const { return wxUniChar( m_ref ) < wxUniChar( c.m_ref ); }
    bool operator>( const wxUniCharRef& c ) const { return wxUniChar( m_ref ) > wxUniChar( c.m_ref ); }
    bool operator<=( const wxUniCharRef& c ) const { return wxUniChar( m_ref ) <= wxUniChar( c.m_ref ); }
    bool operator>=( const wxUniCharRef& c ) const { return wxUniChar( m_ref ) >= wxUniChar( c.m_ref ); }

    int operator-( const wxUniChar& c ) const { return wxUniChar( m_ref ) - c; }
    int operator-( const wxUniCharRef& c ) const { return wxUniChar( m_ref ) - wxUniChar( c.m_ref ); }
    int operator-( char c ) const { return wxUniChar( m_ref ) - c; }
    int operator-( unsigned char c ) const { return wxUniChar( m_ref ) - c; }
    int operator-( wchar_t c ) const { return wxUniChar( m_ref ) - c; }

    /// Swaps the referenced characters (for the algorithms on string iterators).
    friend void swap( wxUniCharRef a, wxUniCharRef b )
    {
        wchar_t tmp = a.m_ref;
        a.m_ref = b.m_ref;
        b.m_ref = tmp;
    }

private:
    wchar_t& m_ref;
};

inline int operator-( char c1, const wxUniCharRef& c2 ) { return -( c2 - c1 ); }
inline int operator-( const wxUniChar& c1, const wxUniCharRef& c2 ) { return -( c2 - c1 ); }
inline int operator-( wchar_t c1, const wxUniCharRef& c2 ) { return -( c2 - c1 ); }


#define KIWX_UNICHARREF_RCMP( T )                                                              \
    inline bool operator==( T c, const wxUniCharRef& u ) { return u == c; }                    \
    inline bool operator!=( T c, const wxUniCharRef& u ) { return u != c; }                    \
    inline bool operator<( T c, const wxUniCharRef& u ) { return u > c; }                      \
    inline bool operator>( T c, const wxUniCharRef& u ) { return u < c; }                      \
    inline bool operator<=( T c, const wxUniCharRef& u ) { return u >= c; }                    \
    inline bool operator>=( T c, const wxUniCharRef& u ) { return u <= c; }

KIWX_UNICHARREF_RCMP( char )
KIWX_UNICHARREF_RCMP( wchar_t )
KIWX_UNICHARREF_RCMP( int )
KIWX_UNICHARREF_RCMP( const wxUniChar& )

#undef KIWX_UNICHARREF_RCMP

#ifndef _WIN32
#pragma GCC visibility pop
#endif
