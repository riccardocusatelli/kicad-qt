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
#include <wx/buffer.h>
#include <wx/chartype.h>
#include <wx/strconv.h>
#include <wx/unichar.h>

#include <cstring>
#include <ctype.h>
#include <functional>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cwchar>
#include <iosfwd>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

typedef std::wstring wxStdWideString;

class wxString;
class wxFormatArg;

namespace kiwx
{
wxString Format( const wxString& aFormat, const wxFormatArg* aArgs, size_t aCount );
}


/**
 * The result of wxString::c_str(): converts to const wchar_t* and to const char* (UTF-8, kept
 * in the string until it changes).
 */
class wxCStrData
{
public:
    wxCStrData( const wxString* aStr, size_t aOffset = 0 ) : m_str( aStr ), m_offset( aOffset ) {}

    const wchar_t* AsWChar() const;
    const char*    AsChar() const;
    wxString       AsString() const;

    const wchar_t* AsInternal() const { return AsWChar(); }

    operator const wchar_t*() const { return AsWChar(); }
    operator const char*() const { return AsChar(); }
    operator const void*() const { return AsWChar(); }
    operator const unsigned char*() const { return reinterpret_cast<const unsigned char*>( AsChar() ); }

    wxScopedCharBuffer  AsCharBuf() const;
    wxScopedWCharBuffer AsWCharBuf() const;

    wxUniChar operator[]( size_t aIndex ) const;
    wxUniChar operator*() const { return ( *this )[0]; }

    wxCStrData operator+( size_t aOffset ) const { return wxCStrData( m_str, m_offset + aOffset ); }
    wxCStrData operator+( int aOffset ) const { return wxCStrData( m_str, m_offset + aOffset ); }

    const wxString* GetString() const { return m_str; }
    size_t          GetOffset() const { return m_offset; }

private:
    const wxString* m_str;
    size_t          m_offset;
};



/**
 * The iterators of wxString: as in wxWidgets, they give characters as wxUniChar (constant
 * iterator) or wxUniCharRef (a reference proxy), over the wide characters of the string.
 */
class kiwxStringConstIterator
{
public:
    typedef std::random_access_iterator_tag iterator_category;
    typedef wxUniChar                       value_type;
    typedef std::ptrdiff_t                  difference_type;
    typedef wxUniChar                       reference;
    typedef void                            pointer;

    kiwxStringConstIterator() = default;
    explicit kiwxStringConstIterator( std::wstring::const_iterator aIt ) : m_it( aIt ) {}

    wxUniChar operator*() const { return *m_it; }
    wxUniChar operator[]( difference_type n ) const { return m_it[n]; }

    kiwxStringConstIterator& operator++() { ++m_it; return *this; }
    kiwxStringConstIterator  operator++( int ) { return kiwxStringConstIterator( m_it++ ); }
    kiwxStringConstIterator& operator--() { --m_it; return *this; }
    kiwxStringConstIterator  operator--( int ) { return kiwxStringConstIterator( m_it-- ); }
    kiwxStringConstIterator& operator+=( difference_type n ) { m_it += n; return *this; }
    kiwxStringConstIterator& operator-=( difference_type n ) { m_it -= n; return *this; }
    kiwxStringConstIterator  operator+( difference_type n ) const { return kiwxStringConstIterator( m_it + n ); }
    kiwxStringConstIterator  operator-( difference_type n ) const { return kiwxStringConstIterator( m_it - n ); }
    friend kiwxStringConstIterator operator+( difference_type n, const kiwxStringConstIterator& i ) { return i + n; }
    difference_type operator-( const kiwxStringConstIterator& o ) const { return m_it - o.m_it; }

    bool operator==( const kiwxStringConstIterator& o ) const { return m_it == o.m_it; }
    bool operator!=( const kiwxStringConstIterator& o ) const { return m_it != o.m_it; }
    bool operator<( const kiwxStringConstIterator& o ) const { return m_it < o.m_it; }
    bool operator>( const kiwxStringConstIterator& o ) const { return m_it > o.m_it; }
    bool operator<=( const kiwxStringConstIterator& o ) const { return m_it <= o.m_it; }
    bool operator>=( const kiwxStringConstIterator& o ) const { return m_it >= o.m_it; }

    /// The underlying std::wstring iterator.
    std::wstring::const_iterator impl() const { return m_it; }

private:
    std::wstring::const_iterator m_it;
};


class kiwxStringIterator
{
public:
    typedef std::random_access_iterator_tag iterator_category;
    typedef wxUniChar                       value_type;
    typedef std::ptrdiff_t                  difference_type;
    typedef wxUniCharRef                    reference;
    typedef void                            pointer;

    kiwxStringIterator() = default;
    explicit kiwxStringIterator( std::wstring::iterator aIt ) : m_it( aIt ) {}

    wxUniCharRef operator*() const { return wxUniCharRef( *m_it ); }
    wxUniCharRef operator[]( difference_type n ) const { return wxUniCharRef( m_it[n] ); }

    kiwxStringIterator& operator++() { ++m_it; return *this; }
    kiwxStringIterator  operator++( int ) { return kiwxStringIterator( m_it++ ); }
    kiwxStringIterator& operator--() { --m_it; return *this; }
    kiwxStringIterator  operator--( int ) { return kiwxStringIterator( m_it-- ); }
    kiwxStringIterator& operator+=( difference_type n ) { m_it += n; return *this; }
    kiwxStringIterator& operator-=( difference_type n ) { m_it -= n; return *this; }
    kiwxStringIterator  operator+( difference_type n ) const { return kiwxStringIterator( m_it + n ); }
    kiwxStringIterator  operator-( difference_type n ) const { return kiwxStringIterator( m_it - n ); }
    friend kiwxStringIterator operator+( difference_type n, const kiwxStringIterator& i ) { return i + n; }
    difference_type operator-( const kiwxStringIterator& o ) const { return m_it - o.m_it; }

    bool operator==( const kiwxStringIterator& o ) const { return m_it == o.m_it; }
    bool operator!=( const kiwxStringIterator& o ) const { return m_it != o.m_it; }
    bool operator<( const kiwxStringIterator& o ) const { return m_it < o.m_it; }
    bool operator>( const kiwxStringIterator& o ) const { return m_it > o.m_it; }
    bool operator<=( const kiwxStringIterator& o ) const { return m_it <= o.m_it; }
    bool operator>=( const kiwxStringIterator& o ) const { return m_it >= o.m_it; }

    operator kiwxStringConstIterator() const { return kiwxStringConstIterator( m_it ); }

    // between constant and mutable iterators
    friend bool operator==( const kiwxStringIterator& a, const kiwxStringConstIterator& b ) { return kiwxStringConstIterator( a ) == b; }
    friend bool operator!=( const kiwxStringIterator& a, const kiwxStringConstIterator& b ) { return kiwxStringConstIterator( a ) != b; }
    friend bool operator==( const kiwxStringConstIterator& a, const kiwxStringIterator& b ) { return a == kiwxStringConstIterator( b ); }
    friend bool operator!=( const kiwxStringConstIterator& a, const kiwxStringIterator& b ) { return a != kiwxStringConstIterator( b ); }
    friend difference_type operator-( const kiwxStringIterator& a, const kiwxStringConstIterator& b ) { return kiwxStringConstIterator( a ) - b; }
    friend difference_type operator-( const kiwxStringConstIterator& a, const kiwxStringIterator& b ) { return a - kiwxStringConstIterator( b ); }

    std::wstring::iterator impl() const { return m_it; }

private:
    std::wstring::iterator m_it;
};


/**
 * An argument of the printf-like functions (wxString::Format(), wxPrintf()...), normalised as
 * wxWidgets does: integers keep their size and signedness, strings of every kind become text.
 */
class wxFormatArg
{
public:
    enum TYPE
    {
        INT,
        UINT,
        DOUBLE,
        LONG_DOUBLE,
        STRING,
        POINTER,
        CHAR
    };

    TYPE               m_type = INT;
    int                m_size = sizeof( int );   ///< byte size of an integer argument
    long long          m_int = 0;
    unsigned long long m_uint = 0;
    double             m_double = 0.0;
    long double        m_longDouble = 0.0;
    const void*        m_pointer = nullptr;
    std::wstring       m_string;
    bool               m_isNull = false;          ///< a null char* or wchar_t* string

    template <typename T>
    static wxFormatArg Make( const T& aValue );
};


/**
 * The string class of the data model: a wide (UTF-32) string with the wxString API of
 * wxWidgets 3.2.
 */
class wxString
{
public:
    typedef wxUniChar                                 value_type;
    typedef wxUniChar                                 char_type;
    typedef std::wstring::size_type                   size_type;
    typedef std::wstring::difference_type             difference_type;
    typedef wxUniCharRef                              reference;
    typedef wxUniChar                                 const_reference;
    typedef wchar_t*                                  pointer;
    typedef const wchar_t*                            const_pointer;
    typedef kiwxStringIterator                        iterator;
    typedef kiwxStringConstIterator                   const_iterator;
    typedef std::reverse_iterator<iterator>           reverse_iterator;
    typedef std::reverse_iterator<const_iterator>     const_reverse_iterator;

    static constexpr size_t npos = size_t( -1 );

    enum stripType
    {
        leading = 0x1,
        trailing = 0x2,
        both = 0x3
    };

    enum caseCompare
    {
        exact,
        ignoreCase
    };

    // Construction
    wxString() = default;
    wxString( const wxString& aOther ) : m_impl( aOther.m_impl ) {}
    wxString( wxString&& aOther ) noexcept : m_impl( std::move( aOther.m_impl ) ) {}

    wxString( const char* aStr ) { assignUtf8( aStr, aStr ? std::strlen( aStr ) : 0 ); }
    wxString( const char* aStr, size_t aLen ) { assignUtf8( aStr, aLen == npos && aStr ? std::strlen( aStr ) : aLen ); }
    wxString( const char* aStr, const wxMBConv& aConv, size_t aLen = npos );
    wxString( const unsigned char* aStr ) : wxString( reinterpret_cast<const char*>( aStr ) ) {}
    wxString( const unsigned char* aStr, size_t aLen ) :
            wxString( reinterpret_cast<const char*>( aStr ), aLen )
    {
    }

    wxString( const wchar_t* aStr ) : m_impl( aStr ? aStr : L"" ) {}
    wxString( const wchar_t* aStr, size_t aLen ) :
            m_impl( aStr ? std::wstring( aStr, aLen == npos ? std::wcslen( aStr ) : aLen ) : std::wstring() )
    {
    }
    wxString( const wchar_t* aStr, const wxMBConv&, size_t aLen = npos ) : wxString( aStr, aLen ) {}

    wxString( const std::string& aStr ) { assignUtf8( aStr.data(), aStr.size() ); }
    wxString( const std::wstring& aStr ) : m_impl( aStr ) {}
    wxString( std::wstring&& aStr ) : m_impl( std::move( aStr ) ) {}
    explicit wxString( std::string_view aStr ) { assignUtf8( aStr.data(), aStr.size() ); }
    explicit wxString( std::wstring_view aStr ) : m_impl( aStr ) {}

    wxString( const wxCStrData& aData );
    wxString( const wxCStrData& aData, size_t aLen ) : wxString( wxString( aData ).Mid( 0, aLen ) ) {}
    wxString( const wxScopedCharBuffer& aBuf ) : wxString( aBuf.data(), aBuf.length() ) {}
    wxString( const wxScopedWCharBuffer& aBuf ) : wxString( aBuf.data(), aBuf.length() ) {}

    wxString( wxUniChar aCh, size_t aRepeat = 1 ) : m_impl( aRepeat, static_cast<wchar_t>( aCh.GetValue() ) ) {}
    wxString( wxUniCharRef aCh, size_t aRepeat = 1 ) : wxString( wxUniChar( aCh ), aRepeat ) {}
    wxString( char aCh, size_t aRepeat = 1 ) : wxString( wxUniChar( aCh ), aRepeat ) {}
    wxString( wchar_t aCh, size_t aRepeat = 1 ) : wxString( wxUniChar( aCh ), aRepeat ) {}
    wxString( size_t aRepeat, wxUniChar aCh ) : wxString( aCh, aRepeat ) {}
    wxString( size_t aRepeat, wxUniCharRef aCh ) : wxString( wxUniChar( aCh ), aRepeat ) {}
    wxString( size_t aRepeat, char aCh ) : wxString( wxUniChar( aCh ), aRepeat ) {}
    wxString( size_t aRepeat, wchar_t aCh ) : wxString( wxUniChar( aCh ), aRepeat ) {}

    template <typename IT,
              typename = typename std::iterator_traits<IT>::iterator_category>
    wxString( IT aFirst, IT aLast )
    {
        for( ; aFirst != aLast; ++aFirst )
            m_impl += static_cast<wchar_t>( wxUniChar( *aFirst ).GetValue() );
    }

    wxString( const wxString& aStr, size_t aPos, size_t aLen = npos ) :
            m_impl( aStr.m_impl, std::min( aPos, aStr.m_impl.size() ), aLen )
    {
    }

    ~wxString() = default;

    // Assignment
    wxString& operator=( const wxString& aOther )
    {
        if( this != &aOther )
        {
            m_impl = aOther.m_impl;
            m_utf8.clear();
        }

        return *this;
    }

    wxString& operator=( wxString&& aOther ) noexcept
    {
        m_impl = std::move( aOther.m_impl );
        m_utf8.clear();
        return *this;
    }

    wxString& operator=( const char* aStr ) { return *this = wxString( aStr ); }
    wxString& operator=( const wchar_t* aStr ) { return *this = wxString( aStr ); }
    wxString& operator=( const std::string& aStr ) { return *this = wxString( aStr ); }
    wxString& operator=( const std::wstring& aStr ) { return *this = wxString( aStr ); }
    wxString& operator=( const wxCStrData& aData ) { return *this = wxString( aData ); }
    wxString& operator=( const wxScopedCharBuffer& aBuf ) { return *this = wxString( aBuf ); }
    wxString& operator=( const wxScopedWCharBuffer& aBuf ) { return *this = wxString( aBuf ); }
    wxString& operator=( wxUniChar aCh ) { return *this = wxString( aCh ); }
    wxString& operator=( wxUniCharRef aCh ) { return *this = wxString( aCh ); }
    wxString& operator=( char aCh ) { return *this = wxString( aCh ); }
    wxString& operator=( wchar_t aCh ) { return *this = wxString( aCh ); }

    wxString& assign( const wxString& aStr ) { return *this = aStr; }
    wxString& assign( const wxString& aStr, size_t aPos, size_t aLen ) { return *this = wxString( aStr, aPos, aLen ); }
    wxString& assign( const char* aStr ) { return *this = wxString( aStr ); }
    wxString& assign( const char* aStr, size_t aLen ) { return *this = wxString( aStr, aLen ); }
    wxString& assign( const wchar_t* aStr ) { return *this = wxString( aStr ); }
    wxString& assign( const wchar_t* aStr, size_t aLen ) { return *this = wxString( aStr, aLen ); }
    wxString& assign( size_t aCount, wxUniChar aCh ) { return *this = wxString( aCh, aCount ); }

    template <typename IT,
              typename = typename std::iterator_traits<IT>::iterator_category>
    wxString& assign( IT aFirst, IT aLast )
    {
        return *this = wxString( aFirst, aLast );
    }

    // Size and access
    size_t length() const { return m_impl.size(); }
    size_t size() const { return m_impl.size(); }
    size_t Len() const { return m_impl.size(); }
    size_t Length() const { return m_impl.size(); }
    size_t max_size() const { return m_impl.max_size(); }
    size_t capacity() const { return m_impl.capacity(); }
    bool   empty() const { return m_impl.empty(); }
    bool   IsEmpty() const { return m_impl.empty(); }
    bool   IsNull() const { return m_impl.empty(); }
    bool   operator!() const { return m_impl.empty(); }

    void   clear() { m_impl.clear(); invalidate(); }
    void   Clear() { clear(); }
    void   Empty() { clear(); }
    void   reserve( size_t aSize ) { m_impl.reserve( aSize ); }
    bool   Alloc( size_t aSize ) { m_impl.reserve( aSize ); return true; }
    void   resize( size_t aSize, wchar_t aCh = 0 ) { m_impl.resize( aSize, aCh ); invalidate(); }
    void   shrink_to_fit() { m_impl.shrink_to_fit(); }
    wxString& Shrink() { m_impl.shrink_to_fit(); return *this; }

    wxUniChar    GetChar( size_t aIndex ) const { return m_impl[aIndex]; }
    wxUniCharRef GetWritableChar( size_t aIndex ) { invalidate(); return wxUniCharRef( m_impl[aIndex] ); }
    void         SetChar( size_t aIndex, wxUniChar aCh ) { m_impl[aIndex] = static_cast<wchar_t>( aCh.GetValue() ); invalidate(); }

    wxUniChar    Last() const { return m_impl.back(); }
    wxUniCharRef Last() { invalidate(); return wxUniCharRef( m_impl.back() ); }

    wxUniChar operator[]( int aIndex ) const { return m_impl[aIndex]; }
    wxUniChar operator[]( long aIndex ) const { return m_impl[aIndex]; }
    wxUniChar operator[]( size_t aIndex ) const { return m_impl[aIndex]; }
    wxUniChar operator[]( unsigned int aIndex ) const { return m_impl[aIndex]; }
    wxUniChar operator[]( long long aIndex ) const { return m_impl[aIndex]; }

    wxUniCharRef operator[]( int aIndex ) { invalidate(); return wxUniCharRef( m_impl[aIndex] ); }
    wxUniCharRef operator[]( long aIndex ) { invalidate(); return wxUniCharRef( m_impl[aIndex] ); }
    wxUniCharRef operator[]( size_t aIndex ) { invalidate(); return wxUniCharRef( m_impl[aIndex] ); }
    wxUniCharRef operator[]( unsigned int aIndex ) { invalidate(); return wxUniCharRef( m_impl[aIndex] ); }
    wxUniCharRef operator[]( long long aIndex ) { invalidate(); return wxUniCharRef( m_impl[aIndex] ); }

    wxUniChar    at( size_t aIndex ) const { return m_impl.at( aIndex ); }
    wxUniCharRef at( size_t aIndex ) { invalidate(); return wxUniCharRef( m_impl.at( aIndex ) ); }

    wxUniChar front() const { return m_impl.front(); }
    wxUniChar back() const { return m_impl.back(); }

    iterator               begin() { invalidate(); return iterator( m_impl.begin() ); }
    iterator               end() { invalidate(); return iterator( m_impl.end() ); }
    const_iterator         begin() const { return const_iterator( m_impl.begin() ); }
    const_iterator         end() const { return const_iterator( m_impl.end() ); }
    const_iterator         cbegin() const { return begin(); }
    const_iterator         cend() const { return end(); }
    reverse_iterator       rbegin() { return reverse_iterator( end() ); }
    reverse_iterator       rend() { return reverse_iterator( begin() ); }
    const_reverse_iterator rbegin() const { return const_reverse_iterator( end() ); }
    const_reverse_iterator rend() const { return const_reverse_iterator( begin() ); }
    const_reverse_iterator crbegin() const { return rbegin(); }
    const_reverse_iterator crend() const { return rend(); }

    // Conversions
    wxCStrData           c_str() const { return wxCStrData( this ); }
    const wchar_t*       wc_str() const { return m_impl.c_str(); }
    const wchar_t*       wc_str( const wxMBConv& ) const { return m_impl.c_str(); }
    const wchar_t*       wx_str() const { return m_impl.c_str(); }
    const wchar_t*       t_str() const { return m_impl.c_str(); }
    const wchar_t*       data() const { return m_impl.c_str(); }
    wxCStrData           GetData() const { return c_str(); }
    wxWritableCharBuffer char_str( const wxMBConv& aConv = wxConvLibc ) const { return mb_str( aConv ); }
    wxWritableWCharBuffer wchar_str() const { return wxWritableWCharBuffer( m_impl ); }
    wxScopedCharBuffer   fn_str() const { return utf8_str(); }
    wxCharBuffer         mb_str( const wxMBConv& aConv = wxConvLibc ) const;
    wxCharBuffer         mbc_str() const { return mb_str( *wxConvCurrent ); }
    wxScopedCharBuffer   utf8_str() const;
    wxScopedCharBuffer   ToUTF8() const { return utf8_str(); }
    wxCharBuffer         ToAscii( char aReplacement = '_' ) const;
    wxCharBuffer         To8BitData() const;
    std::string          ToStdString( const wxMBConv& aConv = wxConvLibc ) const;
    std::wstring         ToStdWstring() const { return m_impl; }
    std::string          utf8_string() const;
    const std::wstring&  ToStdWstringRef() const { return m_impl; }

    // the implicit conversions of wxWidgets (with wxUSE_UNSAFE_WXSTRING_CONV, the default)
    operator wxCStrData() const { return c_str(); }
    operator const wchar_t*() const { return m_impl.c_str(); }
    operator const char*() const { return c_str().AsChar(); }
    operator const void*() const { return m_impl.c_str(); }

    static wxString FromUTF8( const char* aStr, size_t aLen = npos );
    static wxString FromUTF8( const std::string& aStr ) { return FromUTF8( aStr.data(), aStr.size() ); }
    static wxString FromUTF8Unchecked( const char* aStr, size_t aLen = npos ) { return FromUTF8( aStr, aLen ); }
    static wxString FromUTF8Unchecked( const std::string& aStr ) { return FromUTF8( aStr ); }
    static wxString FromAscii( const char* aStr, size_t aLen = npos );
    static wxString FromAscii( char aCh ) { return wxString( aCh ); }
    static wxString From8BitData( const char* aStr, size_t aLen = npos );

    // Append
    wxString& append( const wxString& aStr ) { m_impl += aStr.m_impl; invalidate(); return *this; }
    wxString& append( const wxString& aStr, size_t aPos, size_t aLen ) { m_impl.append( aStr.m_impl, aPos, aLen ); invalidate(); return *this; }
    wxString& append( const char* aStr ) { return append( wxString( aStr ) ); }
    wxString& append( const char* aStr, size_t aLen ) { return append( wxString( aStr, aLen ) ); }
    wxString& append( const wchar_t* aStr ) { return append( wxString( aStr ) ); }
    wxString& append( const wchar_t* aStr, size_t aLen ) { return append( wxString( aStr, aLen ) ); }
    wxString& append( size_t aCount, wxUniChar aCh ) { return append( wxString( aCh, aCount ) ); }

    template <typename IT,
              typename = typename std::iterator_traits<IT>::iterator_category>
    wxString& append( IT aFirst, IT aLast )
    {
        return append( wxString( aFirst, aLast ) );
    }

    wxString& Append( const wxString& aStr ) { return append( aStr ); }
    wxString& Append( const char* aStr ) { return append( aStr ); }
    wxString& Append( const char* aStr, size_t aLen ) { return append( aStr, aLen ); }
    wxString& Append( const wchar_t* aStr ) { return append( aStr ); }
    wxString& Append( const wchar_t* aStr, size_t aLen ) { return append( aStr, aLen ); }
    wxString& Append( wxUniChar aCh, size_t aCount = 1 ) { return append( aCount, aCh ); }
    wxString& Append( char aCh, size_t aCount = 1 ) { return append( aCount, aCh ); }
    wxString& Append( wchar_t aCh, size_t aCount = 1 ) { return append( aCount, aCh ); }
    wxString& Append( wxUniCharRef aCh, size_t aCount = 1 ) { return append( aCount, wxUniChar( aCh ) ); }
    wxString& Append( unsigned char aCh, size_t aCount = 1 ) { return append( aCount, wxUniChar( aCh ) ); }
    wxString& Append( const wxCStrData& aData ) { return append( wxString( aData ) ); }

    void push_back( wxUniChar aCh ) { m_impl += static_cast<wchar_t>( aCh.GetValue() ); invalidate(); }
    void pop_back() { m_impl.pop_back(); invalidate(); }

    wxString& Prepend( const wxString& aStr ) { m_impl.insert( 0, aStr.m_impl ); invalidate(); return *this; }

    wxString& operator+=( const wxString& aStr ) { return append( aStr ); }
    wxString& operator+=( const char* aStr ) { return append( aStr ); }
    wxString& operator+=( const wchar_t* aStr ) { return append( aStr ); }
    wxString& operator+=( const wxCStrData& aData ) { return append( wxString( aData ) ); }
    wxString& operator+=( const wxScopedCharBuffer& aBuf ) { return append( wxString( aBuf ) ); }
    wxString& operator+=( const wxScopedWCharBuffer& aBuf ) { return append( wxString( aBuf ) ); }
    wxString& operator+=( wxUniChar aCh ) { push_back( aCh ); return *this; }
    wxString& operator+=( wxUniCharRef aCh ) { push_back( aCh ); return *this; }
    wxString& operator+=( char aCh ) { push_back( aCh ); return *this; }
    wxString& operator+=( unsigned char aCh ) { push_back( aCh ); return *this; }
    wxString& operator+=( wchar_t aCh ) { push_back( aCh ); return *this; }
    wxString& operator+=( int aCh ) { push_back( wxUniChar( aCh ) ); return *this; }

    wxString& operator<<( const wxString& aStr ) { return append( aStr ); }
    wxString& operator<<( const char* aStr ) { return append( aStr ); }
    wxString& operator<<( const wchar_t* aStr ) { return append( aStr ); }
    wxString& operator<<( const wxCStrData& aData ) { return append( wxString( aData ) ); }
    wxString& operator<<( const wxScopedCharBuffer& aBuf ) { return append( wxString( aBuf ) ); }
    wxString& operator<<( const wxScopedWCharBuffer& aBuf ) { return append( wxString( aBuf ) ); }
    wxString& operator<<( wxUniChar aCh ) { push_back( aCh ); return *this; }
    wxString& operator<<( wxUniCharRef aCh ) { push_back( aCh ); return *this; }
    wxString& operator<<( char aCh ) { push_back( aCh ); return *this; }
    wxString& operator<<( unsigned char aCh ) { push_back( aCh ); return *this; }
    wxString& operator<<( wchar_t aCh ) { push_back( aCh ); return *this; }
    wxString& operator<<( int aValue );
    wxString& operator<<( unsigned int aValue );
    wxString& operator<<( long aValue );
    wxString& operator<<( unsigned long aValue );
    wxString& operator<<( long long aValue );
    wxString& operator<<( unsigned long long aValue );
    wxString& operator<<( float aValue );
    wxString& operator<<( double aValue );

    // Insertion and removal
    wxString& insert( size_t aPos, const wxString& aStr ) { m_impl.insert( aPos, aStr.m_impl ); invalidate(); return *this; }
    wxString& insert( size_t aPos, const char* aStr ) { return insert( aPos, wxString( aStr ) ); }
    wxString& insert( size_t aPos, const wchar_t* aStr ) { return insert( aPos, wxString( aStr ) ); }
    wxString& insert( size_t aPos, const char* aStr, size_t aLen ) { return insert( aPos, wxString( aStr, aLen ) ); }
    wxString& insert( size_t aPos, const wchar_t* aStr, size_t aLen ) { return insert( aPos, wxString( aStr, aLen ) ); }
    wxString& insert( size_t aPos, const wxString& aStr, size_t aPos2, size_t aLen ) { return insert( aPos, aStr.substr( aPos2, aLen ) ); }
    wxString& insert( size_t aPos, size_t aCount, wxUniChar aCh ) { return insert( aPos, wxString( aCh, aCount ) ); }
    iterator  insert( iterator aIt, wxUniChar aCh ) { invalidate(); return iterator( m_impl.insert( aIt.impl(), static_cast<wchar_t>( aCh.GetValue() ) ) ); }
    void      insert( iterator aIt, size_t aCount, wxUniChar aCh ) { invalidate(); m_impl.insert( aIt.impl(), aCount, static_cast<wchar_t>( aCh.GetValue() ) ); }

    template <typename IT,
              typename = typename std::iterator_traits<IT>::iterator_category>
    void insert( iterator aIt, IT aFirst, IT aLast )
    {
        size_t pos = aIt.impl() - m_impl.begin();
        insert( pos, wxString( aFirst, aLast ) );
    }

    wxString& erase( size_t aPos = 0, size_t aLen = npos ) { m_impl.erase( aPos, aLen ); invalidate(); return *this; }
    iterator  erase( iterator aIt ) { invalidate(); return iterator( m_impl.erase( aIt.impl() ) ); }
    iterator  erase( iterator aFirst, iterator aLast ) { invalidate(); return iterator( m_impl.erase( aFirst.impl(), aLast.impl() ) ); }

    wxString& Remove( size_t aPos ) { return Truncate( aPos ); }
    wxString& Remove( size_t aPos, size_t aLen ) { return erase( aPos, aLen ); }
    wxString& RemoveLast( size_t aCount = 1 );
    wxString& Truncate( size_t aLen );

    wxString& replace( size_t aPos, size_t aLen, const wxString& aStr ) { m_impl.replace( aPos, aLen, aStr.m_impl ); invalidate(); return *this; }
    wxString& replace( size_t aPos, size_t aLen, const char* aStr ) { return replace( aPos, aLen, wxString( aStr ) ); }
    wxString& replace( size_t aPos, size_t aLen, const wchar_t* aStr ) { return replace( aPos, aLen, wxString( aStr ) ); }
    wxString& replace( size_t aPos, size_t aLen, size_t aCount, wxUniChar aCh ) { return replace( aPos, aLen, wxString( aCh, aCount ) ); }
    wxString& replace( size_t aPos, size_t aLen, const wxString& aStr, size_t aPos2, size_t aLen2 )
    {
        return replace( aPos, aLen, aStr.substr( aPos2, aLen2 ) );
    }
    wxString& replace( iterator aFirst, iterator aLast, const wxString& aStr )
    {
        m_impl.replace( aFirst.impl(), aLast.impl(), aStr.m_impl );
        invalidate();
        return *this;
    }

    size_t Replace( const wxString& aOld, const wxString& aNew, bool aReplaceAll = true );

    void swap( wxString& aOther ) { m_impl.swap( aOther.m_impl ); invalidate(); aOther.invalidate(); }

    // Comparison
    int  compare( const wxString& aStr ) const { return m_impl.compare( aStr.m_impl ); }
    int  compare( size_t aPos, size_t aLen, const wxString& aStr ) const { return m_impl.compare( aPos, aLen, aStr.m_impl ); }
    int  compare( size_t aPos, size_t aLen, const wxString& aStr, size_t aPos2, size_t aLen2 ) const
    {
        return m_impl.compare( aPos, aLen, aStr.m_impl, aPos2, aLen2 );
    }
    int  compare( const char* aStr ) const { return compare( wxString( aStr ) ); }
    int  compare( const wchar_t* aStr ) const { return m_impl.compare( aStr ); }

    int  Cmp( const wxString& aStr ) const { return compare( aStr ); }
    int  CmpNoCase( const wxString& aStr ) const;
    int  CompareTo( const wxString& aStr, caseCompare aCmp = exact ) const
    {
        return aCmp == exact ? Cmp( aStr ) : CmpNoCase( aStr );
    }

    bool IsSameAs( const wxString& aStr, bool aCaseSensitive = true ) const;
    bool IsSameAs( const char* aStr, bool aCaseSensitive = true ) const { return IsSameAs( wxString( aStr ), aCaseSensitive ); }
    bool IsSameAs( const wchar_t* aStr, bool aCaseSensitive = true ) const { return IsSameAs( wxString( aStr ), aCaseSensitive ); }
    bool IsSameAs( const wxCStrData& aStr, bool aCaseSensitive = true ) const { return IsSameAs( wxString( aStr ), aCaseSensitive ); }
    bool IsSameAs( wxUniChar aCh, bool aCaseSensitive = true ) const;
    bool IsSameAs( char aCh, bool aCaseSensitive = true ) const { return IsSameAs( wxUniChar( aCh ), aCaseSensitive ); }
    bool IsSameAs( wchar_t aCh, bool aCaseSensitive = true ) const { return IsSameAs( wxUniChar( aCh ), aCaseSensitive ); }

    bool Matches( const wxString& aMask ) const;

    bool StartsWith( const wxString& aPrefix, wxString* aRest = nullptr ) const;
    bool EndsWith( const wxString& aSuffix, wxString* aRest = nullptr ) const;
    bool starts_with( const wxString& aPrefix ) const { return StartsWith( aPrefix ); }
    bool ends_with( const wxString& aSuffix ) const { return EndsWith( aSuffix ); }
    bool starts_with( wxUniChar aCh ) const { return !empty() && front() == aCh; }
    bool ends_with( wxUniChar aCh ) const { return !empty() && back() == aCh; }
    bool Contains( const wxString& aStr ) const { return m_impl.find( aStr.m_impl ) != std::wstring::npos; }

    // Searching
    int  Find( wxUniChar aCh, bool aFromEnd = false ) const;
    int  Find( char aCh, bool aFromEnd = false ) const { return Find( wxUniChar( aCh ), aFromEnd ); }
    int  Find( wchar_t aCh, bool aFromEnd = false ) const { return Find( wxUniChar( aCh ), aFromEnd ); }
    int  Find( unsigned char aCh, bool aFromEnd = false ) const { return Find( wxUniChar( aCh ), aFromEnd ); }
    int  Find( wxUniCharRef aCh, bool aFromEnd = false ) const { return Find( wxUniChar( aCh ), aFromEnd ); }
    int  Find( const wxString& aStr ) const;
    int  Find( const char* aStr ) const { return Find( wxString( aStr ) ); }
    int  Find( const wchar_t* aStr ) const { return Find( wxString( aStr ) ); }
    int  Find( const wxCStrData& aStr ) const { return Find( wxString( aStr ) ); }
    int  First( wxUniChar aCh ) const { return Find( aCh ); }
    int  First( const wxString& aStr ) const { return Find( aStr ); }
    int  Last( wxUniChar aCh ) const { return Find( aCh, true ); }
    size_t Index( wxUniChar aCh, bool aFromEnd = false ) const { return Find( aCh, aFromEnd ); }
    size_t Freq( wxUniChar aCh ) const;

    size_t find( const wxString& aStr, size_t aPos = 0 ) const { return m_impl.find( aStr.m_impl, aPos ); }
    size_t find( const char* aStr, size_t aPos = 0 ) const { return find( wxString( aStr ), aPos ); }
    size_t find( const wchar_t* aStr, size_t aPos = 0 ) const { return m_impl.find( aStr, aPos ); }
    size_t find( const wchar_t* aStr, size_t aPos, size_t aLen ) const { return m_impl.find( aStr, aPos, aLen ); }
    size_t find( wxUniChar aCh, size_t aPos = 0 ) const { return m_impl.find( static_cast<wchar_t>( aCh.GetValue() ), aPos ); }
    size_t find( char aCh, size_t aPos = 0 ) const { return find( wxUniChar( aCh ), aPos ); }
    size_t find( wchar_t aCh, size_t aPos = 0 ) const { return find( wxUniChar( aCh ), aPos ); }
    size_t find( wxUniCharRef aCh, size_t aPos = 0 ) const { return find( wxUniChar( aCh ), aPos ); }
    size_t rfind( const wxString& aStr, size_t aPos = npos ) const { return m_impl.rfind( aStr.m_impl, aPos ); }
    size_t rfind( const char* aStr, size_t aPos = npos ) const { return rfind( wxString( aStr ), aPos ); }
    size_t rfind( const wchar_t* aStr, size_t aPos = npos ) const { return m_impl.rfind( aStr, aPos ); }
    size_t rfind( wxUniChar aCh, size_t aPos = npos ) const { return m_impl.rfind( static_cast<wchar_t>( aCh.GetValue() ), aPos ); }
    size_t rfind( char aCh, size_t aPos = npos ) const { return rfind( wxUniChar( aCh ), aPos ); }
    size_t rfind( wchar_t aCh, size_t aPos = npos ) const { return rfind( wxUniChar( aCh ), aPos ); }

    size_t find_first_of( const wxString& aSet, size_t aPos = 0 ) const { return m_impl.find_first_of( aSet.m_impl, aPos ); }
    size_t find_first_of( const char* aSet, size_t aPos = 0 ) const { return find_first_of( wxString( aSet ), aPos ); }
    size_t find_first_of( const wchar_t* aSet, size_t aPos = 0 ) const { return m_impl.find_first_of( aSet, aPos ); }
    size_t find_first_of( wxUniChar aCh, size_t aPos = 0 ) const { return find( aCh, aPos ); }
    size_t find_first_of( char aCh, size_t aPos = 0 ) const { return find( wxUniChar( aCh ), aPos ); }
    size_t find_first_of( wchar_t aCh, size_t aPos = 0 ) const { return find( wxUniChar( aCh ), aPos ); }
    size_t find_last_of( const wxString& aSet, size_t aPos = npos ) const { return m_impl.find_last_of( aSet.m_impl, aPos ); }
    size_t find_last_of( const char* aSet, size_t aPos = npos ) const { return find_last_of( wxString( aSet ), aPos ); }
    size_t find_last_of( const wchar_t* aSet, size_t aPos = npos ) const { return m_impl.find_last_of( aSet, aPos ); }
    size_t find_last_of( wxUniChar aCh, size_t aPos = npos ) const { return rfind( aCh, aPos ); }
    size_t find_last_of( char aCh, size_t aPos = npos ) const { return rfind( wxUniChar( aCh ), aPos ); }
    size_t find_last_of( wchar_t aCh, size_t aPos = npos ) const { return rfind( wxUniChar( aCh ), aPos ); }
    size_t find_first_not_of( const wxString& aSet, size_t aPos = 0 ) const { return m_impl.find_first_not_of( aSet.m_impl, aPos ); }
    size_t find_first_not_of( const char* aSet, size_t aPos = 0 ) const { return find_first_not_of( wxString( aSet ), aPos ); }
    size_t find_first_not_of( const wchar_t* aSet, size_t aPos = 0 ) const { return m_impl.find_first_not_of( aSet, aPos ); }
    size_t find_first_not_of( wxUniChar aCh, size_t aPos = 0 ) const { return m_impl.find_first_not_of( static_cast<wchar_t>( aCh.GetValue() ), aPos ); }
    size_t find_first_not_of( char aCh, size_t aPos = 0 ) const { return find_first_not_of( wxUniChar( aCh ), aPos ); }
    size_t find_first_not_of( wchar_t aCh, size_t aPos = 0 ) const { return find_first_not_of( wxUniChar( aCh ), aPos ); }
    size_t find_last_not_of( const wxString& aSet, size_t aPos = npos ) const { return m_impl.find_last_not_of( aSet.m_impl, aPos ); }
    size_t find_last_not_of( const char* aSet, size_t aPos = npos ) const { return find_last_not_of( wxString( aSet ), aPos ); }
    size_t find_last_not_of( const wchar_t* aSet, size_t aPos = npos ) const { return m_impl.find_last_not_of( aSet, aPos ); }
    size_t find_last_not_of( wxUniChar aCh, size_t aPos = npos ) const { return m_impl.find_last_not_of( static_cast<wchar_t>( aCh.GetValue() ), aPos ); }
    size_t find_last_not_of( char aCh, size_t aPos = npos ) const { return find_last_not_of( wxUniChar( aCh ), aPos ); }
    size_t find_last_not_of( wchar_t aCh, size_t aPos = npos ) const { return find_last_not_of( wxUniChar( aCh ), aPos ); }

    // Substrings
    wxString substr( size_t aPos = 0, size_t aLen = npos ) const { return wxString( m_impl.substr( aPos, aLen ) ); }
    wxString Mid( size_t aFirst, size_t aCount = npos ) const;
    wxString operator()( size_t aFirst, size_t aCount ) const { return Mid( aFirst, aCount ); }
    wxString SubString( size_t aFrom, size_t aTo ) const { return Mid( aFrom, aTo - aFrom + 1 ); }
    wxString Left( size_t aCount ) const;
    wxString Right( size_t aCount ) const;
    wxString BeforeFirst( wxUniChar aCh, wxString* aRest = nullptr ) const;
    wxString BeforeLast( wxUniChar aCh, wxString* aRest = nullptr ) const;
    wxString AfterFirst( wxUniChar aCh ) const;
    wxString AfterLast( wxUniChar aCh ) const;
    wxString Before( wxUniChar aCh ) const { return BeforeLast( aCh ); }
    wxString After( wxUniChar aCh ) const { return AfterFirst( aCh ); }

    // Case and whitespace
    wxString& MakeUpper();
    wxString& MakeLower();
    wxString& MakeCapitalized();
    wxString  Upper() const { return wxString( *this ).MakeUpper(); }
    wxString  Lower() const { return wxString( *this ).MakeLower(); }
    wxString  Capitalize() const { return wxString( *this ).MakeCapitalized(); }
    wxString& UpperCase() { return MakeUpper(); }
    wxString& LowerCase() { return MakeLower(); }

    wxString& Trim( bool aFromRight = true );
    wxString  Strip( stripType aWhat = trailing ) const;
    wxString& Pad( size_t aCount, wxUniChar aCh = ' ', bool aFromRight = true );

    // Classification
    bool IsAscii() const;
    bool IsNumber() const;
    bool IsWord() const;

    // Numbers
    bool ToLong( long* aValue, int aBase = 10 ) const;
    bool ToULong( unsigned long* aValue, int aBase = 10 ) const;
    bool ToLongLong( long long* aValue, int aBase = 10 ) const;
    bool ToULongLong( unsigned long long* aValue, int aBase = 10 ) const;
    bool ToInt( int* aValue, int aBase = 10 ) const;
    bool ToUInt( unsigned int* aValue, int aBase = 10 ) const;
    bool ToDouble( double* aValue ) const;
    bool ToCLong( long* aValue, int aBase = 10 ) const { return ToLong( aValue, aBase ); }
    bool ToCULong( unsigned long* aValue, int aBase = 10 ) const { return ToULong( aValue, aBase ); }
    bool ToCDouble( double* aValue ) const;

    static wxString FromDouble( double aValue, int aPrecision = -1 );
    static wxString FromCDouble( double aValue, int aPrecision = -1 );

    // Formatting
    template <typename... ARGS>
    static wxString Format( const wxString& aFormat, const ARGS&... aArgs )
    {
        if constexpr( sizeof...( ARGS ) == 0 )
        {
            return kiwx::Format( aFormat, nullptr, 0 );
        }
        else
        {
            const wxFormatArg args[] = { wxFormatArg::Make( aArgs )... };
            return kiwx::Format( aFormat, args, sizeof...( ARGS ) );
        }
    }

    template <typename... ARGS>
    int Printf( const wxString& aFormat, const ARGS&... aArgs )
    {
        *this = Format( aFormat, aArgs... );
        return static_cast<int>( length() );
    }

    template <typename... ARGS>
    int sprintf( const wxString& aFormat, const ARGS&... aArgs )
    {
        return Printf( aFormat, aArgs... );
    }

    // Internal
    const std::wstring& impl() const { return m_impl; }
    std::wstring&       impl() { invalidate(); return m_impl; }

private:
    friend class wxCStrData;

    void assignUtf8( const char* aStr, size_t aLen );
    void invalidate() { m_utf8.clear(); }

    const char* utf8Cache() const;

    std::wstring        m_impl;
    mutable std::string m_utf8;   ///< UTF-8 copy for c_str() conversions to const char*
};


// The empty string (a function-local static: usable during static initialisation).
const wxString& kiwxEmptyString();
#define wxEmptyString kiwxEmptyString()


// Concatenation
wxString operator+( const wxString& aLeft, const wxString& aRight );
wxString operator+( const wxString& aLeft, const char* aRight );
wxString operator+( const char* aLeft, const wxString& aRight );
wxString operator+( const wxString& aLeft, const wchar_t* aRight );
wxString operator+( const wchar_t* aLeft, const wxString& aRight );
wxString operator+( const wxString& aLeft, wxUniChar aRight );
wxString operator+( wxUniChar aLeft, const wxString& aRight );
wxString operator+( const wxString& aLeft, char aRight );
wxString operator+( char aLeft, const wxString& aRight );
wxString operator+( const wxString& aLeft, wchar_t aRight );
wxString operator+( wchar_t aLeft, const wxString& aRight );
wxString operator+( const wxString& aLeft, wxUniCharRef aRight );
wxString operator+( const wxString& aLeft, const wxCStrData& aRight );
wxString operator+( const wxCStrData& aLeft, const wxString& aRight );


// Comparison: the overloads of wxWidgets (no std::string ones: those go through wxString)
#define KIWX_STRING_CMP_OP( OP )                                                               \
    inline bool operator OP( const wxString& a, const wxString& b ) { return a.impl() OP b.impl(); } \
    inline bool operator OP( const wxString& a, const char* b ) { return a.impl() OP wxString( b ).impl(); } \
    inline bool operator OP( const char* a, const wxString& b ) { return wxString( a ).impl() OP b.impl(); } \
    inline bool operator OP( const wxString& a, const wchar_t* b ) { return a.impl() OP std::wstring_view( b ? b : L"" ); } \
    inline bool operator OP( const wchar_t* a, const wxString& b ) { return std::wstring_view( a ? a : L"" ) OP b.impl(); }

KIWX_STRING_CMP_OP( == )
KIWX_STRING_CMP_OP( != )
KIWX_STRING_CMP_OP( < )
KIWX_STRING_CMP_OP( > )
KIWX_STRING_CMP_OP( <= )
KIWX_STRING_CMP_OP( >= )

#undef KIWX_STRING_CMP_OP

#define KIWX_STRING_EQ_OP( T )                                                                 \
    inline bool operator==( const wxString& a, T b ) { return a.impl() == wxString( b ).impl(); } \
    inline bool operator==( T a, const wxString& b ) { return wxString( a ).impl() == b.impl(); } \
    inline bool operator!=( const wxString& a, T b ) { return a.impl() != wxString( b ).impl(); } \
    inline bool operator!=( T a, const wxString& b ) { return wxString( a ).impl() != b.impl(); }

KIWX_STRING_EQ_OP( const wxCStrData& )
KIWX_STRING_EQ_OP( const wxScopedWCharBuffer& )
KIWX_STRING_EQ_OP( const wxScopedCharBuffer& )

#undef KIWX_STRING_EQ_OP

#define KIWX_STRING_CHAR_OP( T )                                                               \
    inline bool operator==( T c, const wxString& s ) { return s.IsSameAs( wxUniChar( c ) ); }  \
    inline bool operator==( const wxString& s, T c ) { return s.IsSameAs( wxUniChar( c ) ); }  \
    inline bool operator!=( T c, const wxString& s ) { return !s.IsSameAs( wxUniChar( c ) ); } \
    inline bool operator!=( const wxString& s, T c ) { return !s.IsSameAs( wxUniChar( c ) ); }

KIWX_STRING_CHAR_OP( const wxUniChar& )
KIWX_STRING_CHAR_OP( const wxUniCharRef& )
KIWX_STRING_CHAR_OP( char )
KIWX_STRING_CHAR_OP( wchar_t )

#undef KIWX_STRING_CHAR_OP

inline bool operator==( int c, const wxString& s ) { return s.IsSameAs( wxUniChar( c ) ); }
inline bool operator!=( int c, const wxString& s ) { return !s.IsSameAs( wxUniChar( c ) ); }

std::ostream&  operator<<( std::ostream& aStream, const wxString& aStr );
std::wostream& operator<<( std::wostream& aStream, const wxString& aStr );
std::ostream&  operator<<( std::ostream& aStream, const wxCStrData& aStr );
std::ostream&  operator<<( std::ostream& aStream, const wxScopedCharBuffer& aStr );
std::ostream&  operator<<( std::ostream& aStream, const wxScopedWCharBuffer& aStr );


namespace std
{
template <>
struct hash<wxString>
{
    size_t operator()( const wxString& aStr ) const noexcept
    {
        return std::hash<std::wstring>()( aStr.impl() );
    }
};
} // namespace std


// String helpers of wxWidgets
inline size_t wxStrlen( const wxString& aStr )
{
    return aStr.length();
}

inline const wchar_t* wxStr( const wxString& aStr )
{
    return aStr.wc_str();
}

/// Comparison function of wxArrayString::Sort().
int wxStringSortAscending( const wxString& a, const wxString& b );
int wxStringSortDescending( const wxString& a, const wxString& b );
int wxCmpNatural( const wxString& a, const wxString& b );


// A null check that is not one for arrays (they decay to pointers that cannot be null)
template <typename T>
constexpr bool kiwxNotNull( const T& aValue )
{
    if constexpr( std::is_array_v<T> )
        return true;
    else
        return aValue != nullptr;
}


// The argument normalisation of the printf-like functions.
template <typename T>
wxFormatArg wxFormatArg::Make( const T& aValue )
{
    typedef std::decay_t<T> U;
    wxFormatArg             arg;

    if constexpr( std::is_same_v<U, wxString> )
    {
        arg.m_type = STRING;
        arg.m_string = aValue.impl();
    }
    else if constexpr( std::is_same_v<U, char*> || std::is_same_v<U, const char*>
                       || std::is_same_v<U, unsigned char*>
                       || std::is_same_v<U, const unsigned char*> )
    {
        arg.m_type = STRING;

        if( kiwxNotNull( aValue ) )
            arg.m_string = wxString( reinterpret_cast<const char*>( aValue ) ).impl();
        else
            arg.m_isNull = true;
    }
    else if constexpr( std::is_same_v<U, wchar_t*> || std::is_same_v<U, const wchar_t*> )
    {
        arg.m_type = STRING;

        if( kiwxNotNull( aValue ) )
            arg.m_string = aValue;
        else
            arg.m_isNull = true;
    }
    else if constexpr( std::is_same_v<U, wxUniChar> || std::is_same_v<U, wxUniCharRef>
                       || std::is_same_v<U, wchar_t> || std::is_same_v<U, char16_t>
                       || std::is_same_v<U, char32_t> )
    {
        arg.m_type = CHAR;
        arg.m_int = static_cast<long long>( wxUniChar( aValue ).GetValue() );
        arg.m_uint = static_cast<unsigned long long>( arg.m_int );
        arg.m_size = sizeof( wchar_t );
    }
    else if constexpr( std::is_same_v<U, bool> )
    {
        arg.m_type = INT;
        arg.m_int = aValue ? 1 : 0;
        arg.m_uint = arg.m_int;
    }
    else if constexpr( std::is_enum_v<U> )
    {
        arg.m_type = INT;
        arg.m_int = static_cast<long long>( aValue );
        arg.m_uint = static_cast<unsigned long long>( aValue );
        arg.m_size = sizeof( int );
    }
    else if constexpr( std::is_integral_v<U> && std::is_signed_v<U> )
    {
        arg.m_type = INT;
        arg.m_int = aValue;
        arg.m_uint = static_cast<unsigned long long>( static_cast<std::make_unsigned_t<U>>( aValue ) );
        arg.m_size = sizeof( U ) < sizeof( int ) ? sizeof( int ) : sizeof( U );
    }
    else if constexpr( std::is_integral_v<U> )
    {
        arg.m_type = UINT;
        arg.m_uint = aValue;
        arg.m_int = static_cast<long long>( aValue );
        arg.m_size = sizeof( U ) < sizeof( int ) ? sizeof( int ) : sizeof( U );
    }
    else if constexpr( std::is_same_v<U, long double> )
    {
        arg.m_type = LONG_DOUBLE;
        arg.m_longDouble = aValue;
        arg.m_double = static_cast<double>( aValue );
    }
    else if constexpr( std::is_floating_point_v<U> )
    {
        arg.m_type = DOUBLE;
        arg.m_double = aValue;
        arg.m_longDouble = aValue;
    }
    else if constexpr( std::is_same_v<U, std::nullptr_t> )
    {
        arg.m_type = POINTER;
    }
    else if constexpr( std::is_same_v<U, std::string_view> )
    {
        arg.m_type = STRING;
        arg.m_string = wxString( aValue.data(), wxConvLibc, aValue.size() ).impl();
    }
    else if constexpr( std::is_same_v<U, std::wstring_view> )
    {
        arg.m_type = STRING;
        arg.m_string = std::wstring( aValue );
    }
    else if constexpr( std::is_convertible_v<const T&, wxString> )
    {
        arg.m_type = STRING;
        arg.m_string = wxString( aValue ).impl();
    }
    else if constexpr( std::is_pointer_v<U> )
    {
        arg.m_type = POINTER;
        arg.m_pointer = reinterpret_cast<const void*>( aValue );
    }
    else
    {
        static_assert( std::is_pointer_v<U>, "unsupported printf argument type" );
    }

    return arg;
}

#ifndef _WIN32
#pragma GCC visibility pop
#endif
