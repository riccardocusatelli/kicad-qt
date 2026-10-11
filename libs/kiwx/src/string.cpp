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

#include <wx/string.h>

#include <algorithm>
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <clocale>
#include <cmath>
#include <cwctype>
#include <iomanip>
#include <locale.h>
#include <ostream>
#include <sstream>


namespace
{

/// The whitespace of Trim() (wxSafeIsspace(): ASCII only).
bool isSpace( wchar_t c )
{
    return c < 127 && std::iswspace( c );
}


/// wcstod() in the C locale, whatever the current locale.
double cLocaleWcstod( const wchar_t* aStr, wchar_t** aEnd )
{
#ifdef _WIN32
    static _locale_t loc = _create_locale( LC_ALL, "C" );
    return _wcstod_l( aStr, aEnd, loc );
#else
    static locale_t loc = newlocale( LC_ALL_MASK, "C", (locale_t) 0 );
    return wcstod_l( aStr, aEnd, loc );
#endif
}

} // namespace


const wxString& kiwxEmptyString()
{
    static const wxString empty;
    return empty;
}


// wxCStrData

const wchar_t* wxCStrData::AsWChar() const
{
    return m_str->wc_str() + m_offset;
}


const char* wxCStrData::AsChar() const
{
    const char* utf8 = m_str->utf8Cache();

    if( m_offset == 0 )
        return utf8;

    // Skip m_offset characters of the UTF-8 copy.
    const char* p = utf8;

    for( size_t i = 0; i < m_offset && *p; ++i )
    {
        ++p;

        while( ( static_cast<unsigned char>( *p ) & 0xC0 ) == 0x80 )
            ++p;
    }

    return p;
}


wxString wxCStrData::AsString() const
{
    return m_offset ? m_str->Mid( m_offset ) : *m_str;
}


wxScopedCharBuffer wxCStrData::AsCharBuf() const
{
    return wxScopedCharBuffer::CreateOwned( AsChar() );
}


wxScopedWCharBuffer wxCStrData::AsWCharBuf() const
{
    return wxScopedWCharBuffer::CreateOwned( AsWChar() );
}


wxUniChar wxCStrData::operator[]( size_t aIndex ) const
{
    return m_str->GetChar( m_offset + aIndex );
}


// wxString: construction and conversion

wxString::wxString( const char* aStr, const wxMBConv& aConv, size_t aLen )
{
    if( !aStr )
        return;

    std::wstring out;

    if( aLen == npos )
    {
        wxWCharBuffer buf = aConv.cMB2WC( aStr );

        if( buf.data() )
            m_impl = buf.data();
    }
    else if( aConv.Decode( aStr, aLen, out ) )
    {
        m_impl = std::move( out );
    }
}


wxString::wxString( const wxCStrData& aData ) :
        m_impl( aData.GetString()->impl(), std::min( aData.GetOffset(), aData.GetString()->length() ) )
{
}


void wxString::assignUtf8( const char* aStr, size_t aLen )
{
    m_utf8.clear();

    if( !aStr || !aLen )
    {
        m_impl.clear();
        return;
    }

    // Plain ASCII is by far the most common case.
    bool ascii = true;

    for( size_t i = 0; i < aLen; ++i )
    {
        if( static_cast<unsigned char>( aStr[i] ) >= 0x80 )
        {
            ascii = false;
            break;
        }
    }

    if( ascii )
    {
        m_impl.assign( aStr, aStr + aLen );
        return;
    }

    if( !kiwxFromUTF8( aStr, aLen, m_impl ) )
        m_impl.clear();   // as wxConvLibc: invalid input gives an empty string
}


const char* wxString::utf8Cache() const
{
    if( m_utf8.empty() && !m_impl.empty() )
        m_utf8 = kiwxToUTF8( m_impl.data(), m_impl.size() );

    return m_utf8.c_str();
}


wxCharBuffer wxString::mb_str( const wxMBConv& aConv ) const
{
    std::string out;

    if( !aConv.Encode( m_impl.data(), m_impl.size(), out ) )
        return wxCharBuffer( "" );

    return wxCharBuffer( out );
}


wxScopedCharBuffer wxString::utf8_str() const
{
    std::string out = kiwxToUTF8( m_impl.data(), m_impl.size() );
    return wxScopedCharBuffer::CreateOwned( out.c_str(), out.size() );
}


std::string wxString::utf8_string() const
{
    return kiwxToUTF8( m_impl.data(), m_impl.size() );
}


std::string wxString::ToStdString( const wxMBConv& aConv ) const
{
    std::string out;

    if( !aConv.Encode( m_impl.data(), m_impl.size(), out ) )
        return std::string();

    return out;
}


wxCharBuffer wxString::ToAscii( char aReplacement ) const
{
    std::string out;
    out.reserve( m_impl.size() );

    for( wchar_t c : m_impl )
        out += c < 0x80 ? static_cast<char>( c ) : aReplacement;

    return wxCharBuffer( out );
}


wxCharBuffer wxString::To8BitData() const
{
    std::string out;
    out.reserve( m_impl.size() );

    for( wchar_t c : m_impl )
        out += static_cast<char>( c );

    return wxCharBuffer( out );
}


wxString wxString::FromUTF8( const char* aStr, size_t aLen )
{
    wxString result;

    if( !aStr )
        return result;

    if( aLen == npos )
        aLen = std::strlen( aStr );

    if( !kiwxFromUTF8( aStr, aLen, result.m_impl ) )
        result.m_impl.clear();

    return result;
}


wxString wxString::FromAscii( const char* aStr, size_t aLen )
{
    wxString result;

    if( !aStr )
        return result;

    if( aLen == npos )
        aLen = std::strlen( aStr );

    result.m_impl.reserve( aLen );

    for( size_t i = 0; i < aLen; ++i )
        result.m_impl += static_cast<wchar_t>( static_cast<unsigned char>( aStr[i] ) );

    return result;
}


wxString wxString::From8BitData( const char* aStr, size_t aLen )
{
    return FromAscii( aStr, aLen );
}


// wxString: operators

namespace
{

wxString formatNumber( const char* aFormat, ... ) __attribute__( ( format( printf, 1, 2 ) ) );

wxString formatNumber( const char* aFormat, ... )
{
    char    buf[64];
    va_list args;
    va_start( args, aFormat );
    std::vsnprintf( buf, sizeof( buf ), aFormat, args );
    va_end( args );
    return wxString( buf );
}

} // namespace


wxString& wxString::operator<<( int aValue )
{
    return append( formatNumber( "%d", aValue ) );
}


wxString& wxString::operator<<( unsigned int aValue )
{
    return append( formatNumber( "%u", aValue ) );
}


wxString& wxString::operator<<( long aValue )
{
    return append( formatNumber( "%ld", aValue ) );
}


wxString& wxString::operator<<( unsigned long aValue )
{
    return append( formatNumber( "%lu", aValue ) );
}


wxString& wxString::operator<<( long long aValue )
{
    return append( formatNumber( "%lld", aValue ) );
}


wxString& wxString::operator<<( unsigned long long aValue )
{
    return append( formatNumber( "%llu", aValue ) );
}


wxString& wxString::operator<<( float aValue )
{
    return append( formatNumber( "%f", static_cast<double>( aValue ) ) );
}


wxString& wxString::operator<<( double aValue )
{
    return append( formatNumber( "%g", aValue ) );
}


// wxString: editing

wxString& wxString::RemoveLast( size_t aCount )
{
    if( aCount >= m_impl.size() )
        m_impl.clear();
    else
        m_impl.erase( m_impl.size() - aCount );

    invalidate();
    return *this;
}


wxString& wxString::Truncate( size_t aLen )
{
    if( aLen < m_impl.size() )
    {
        m_impl.erase( aLen );
        invalidate();
    }

    return *this;
}


size_t wxString::Replace( const wxString& aOld, const wxString& aNew, bool aReplaceAll )
{
    if( aOld.empty() )
        return 0;

    size_t count = 0;
    size_t pos = 0;

    if( !aReplaceAll )
    {
        pos = m_impl.find( aOld.m_impl );

        if( pos == std::wstring::npos )
            return 0;

        m_impl.replace( pos, aOld.length(), aNew.m_impl );
        invalidate();
        return 1;
    }

    std::wstring result;
    size_t       start = 0;

    while( ( pos = m_impl.find( aOld.m_impl, start ) ) != std::wstring::npos )
    {
        result.append( m_impl, start, pos - start );
        result += aNew.m_impl;
        start = pos + aOld.length();
        count++;
    }

    if( count )
    {
        result.append( m_impl, start, std::wstring::npos );
        m_impl = std::move( result );
        invalidate();
    }

    return count;
}


// wxString: comparison and search

int wxString::CmpNoCase( const wxString& aStr ) const
{
    size_t len = std::min( m_impl.size(), aStr.m_impl.size() );

    for( size_t i = 0; i < len; ++i )
    {
        wint_t a = std::towlower( m_impl[i] );
        wint_t b = std::towlower( aStr.m_impl[i] );

        if( a != b )
            return a < b ? -1 : 1;
    }

    if( m_impl.size() == aStr.m_impl.size() )
        return 0;

    return m_impl.size() < aStr.m_impl.size() ? -1 : 1;
}


bool wxString::IsSameAs( const wxString& aStr, bool aCaseSensitive ) const
{
    if( aCaseSensitive )
        return m_impl == aStr.m_impl;

    return m_impl.size() == aStr.m_impl.size() && CmpNoCase( aStr ) == 0;
}


bool wxString::IsSameAs( wxUniChar aCh, bool aCaseSensitive ) const
{
    if( m_impl.size() != 1 )
        return false;

    wchar_t c = static_cast<wchar_t>( aCh.GetValue() );

    if( aCaseSensitive )
        return m_impl[0] == c;

    return std::towlower( m_impl[0] ) == std::towlower( c );
}


bool wxString::Matches( const wxString& aMask ) const
{
    // Iterative wildcard matching ('*' any sequence, '?' any character), with backtracking
    // to the last '*'.
    const wchar_t* str = m_impl.c_str();
    const wchar_t* mask = aMask.m_impl.c_str();
    const wchar_t* starMask = nullptr;
    const wchar_t* starStr = nullptr;

    while( *str )
    {
        if( *mask == L'*' )
        {
            starMask = mask++;
            starStr = str;
        }
        else if( *mask == L'?' || *mask == *str )
        {
            ++mask;
            ++str;
        }
        else if( starMask )
        {
            mask = starMask + 1;
            str = ++starStr;
        }
        else
        {
            return false;
        }
    }

    while( *mask == L'*' )
        ++mask;

    return *mask == 0;
}


bool wxString::StartsWith( const wxString& aPrefix, wxString* aRest ) const
{
    if( m_impl.compare( 0, aPrefix.length(), aPrefix.m_impl ) != 0 )
        return false;

    if( aRest )
        *aRest = Mid( aPrefix.length() );

    return true;
}


bool wxString::EndsWith( const wxString& aSuffix, wxString* aRest ) const
{
    if( aSuffix.length() > m_impl.size() )
        return false;

    size_t start = m_impl.size() - aSuffix.length();

    if( m_impl.compare( start, npos, aSuffix.m_impl ) != 0 )
        return false;

    if( aRest )
        *aRest = Left( start );

    return true;
}


int wxString::Find( wxUniChar aCh, bool aFromEnd ) const
{
    wchar_t c = static_cast<wchar_t>( aCh.GetValue() );
    size_t  pos = aFromEnd ? m_impl.rfind( c ) : m_impl.find( c );

    return pos == std::wstring::npos ? wxNOT_FOUND : static_cast<int>( pos );
}


int wxString::Find( const wxString& aStr ) const
{
    size_t pos = m_impl.find( aStr.m_impl );
    return pos == std::wstring::npos ? wxNOT_FOUND : static_cast<int>( pos );
}


size_t wxString::Freq( wxUniChar aCh ) const
{
    wchar_t c = static_cast<wchar_t>( aCh.GetValue() );
    return std::count( m_impl.begin(), m_impl.end(), c );
}


// wxString: substrings

wxString wxString::Mid( size_t aFirst, size_t aCount ) const
{
    if( aFirst > m_impl.size() )
        return wxString();

    if( aCount == npos || aFirst + aCount > m_impl.size() )
        aCount = m_impl.size() - aFirst;

    return wxString( m_impl.substr( aFirst, aCount ) );
}


wxString wxString::Left( size_t aCount ) const
{
    return wxString( m_impl.substr( 0, std::min( aCount, m_impl.size() ) ) );
}


wxString wxString::Right( size_t aCount ) const
{
    if( aCount >= m_impl.size() )
        return *this;

    return wxString( m_impl.substr( m_impl.size() - aCount ) );
}


wxString wxString::BeforeFirst( wxUniChar aCh, wxString* aRest ) const
{
    int pos = Find( aCh );

    if( pos == wxNOT_FOUND )
    {
        if( aRest )
            aRest->clear();

        return *this;
    }

    if( aRest )
        *aRest = Mid( pos + 1 );

    return Left( pos );
}


wxString wxString::BeforeLast( wxUniChar aCh, wxString* aRest ) const
{
    int pos = Find( aCh, true );

    if( pos == wxNOT_FOUND )
    {
        if( aRest )
            *aRest = *this;

        return wxString();
    }

    if( aRest )
        *aRest = Mid( pos + 1 );

    return Left( pos );
}


wxString wxString::AfterFirst( wxUniChar aCh ) const
{
    int pos = Find( aCh );
    return pos == wxNOT_FOUND ? wxString() : Mid( pos + 1 );
}


wxString wxString::AfterLast( wxUniChar aCh ) const
{
    int pos = Find( aCh, true );
    return pos == wxNOT_FOUND ? *this : Mid( pos + 1 );
}


// wxString: case and whitespace

wxString& wxString::MakeUpper()
{
    for( wchar_t& c : m_impl )
        c = static_cast<wchar_t>( std::towupper( c ) );

    invalidate();
    return *this;
}


wxString& wxString::MakeLower()
{
    for( wchar_t& c : m_impl )
        c = static_cast<wchar_t>( std::towlower( c ) );

    invalidate();
    return *this;
}


wxString& wxString::MakeCapitalized()
{
    if( !m_impl.empty() )
    {
        m_impl[0] = static_cast<wchar_t>( std::towupper( m_impl[0] ) );

        for( size_t i = 1; i < m_impl.size(); ++i )
            m_impl[i] = static_cast<wchar_t>( std::towlower( m_impl[i] ) );

        invalidate();
    }

    return *this;
}


wxString& wxString::Trim( bool aFromRight )
{
    if( m_impl.empty() )
        return *this;

    if( aFromRight )
    {
        size_t end = m_impl.size();

        while( end > 0 && isSpace( m_impl[end - 1] ) )
            --end;

        m_impl.erase( end );
    }
    else
    {
        size_t start = 0;

        while( start < m_impl.size() && isSpace( m_impl[start] ) )
            ++start;

        m_impl.erase( 0, start );
    }

    invalidate();
    return *this;
}


wxString wxString::Strip( stripType aWhat ) const
{
    wxString result( *this );

    if( aWhat & leading )
        result.Trim( false );

    if( aWhat & trailing )
        result.Trim( true );

    return result;
}


wxString& wxString::Pad( size_t aCount, wxUniChar aCh, bool aFromRight )
{
    std::wstring pad( aCount, static_cast<wchar_t>( aCh.GetValue() ) );

    if( aFromRight )
        m_impl += pad;
    else
        m_impl.insert( 0, pad );

    invalidate();
    return *this;
}


// wxString: classification

bool wxString::IsAscii() const
{
    for( wchar_t c : m_impl )
    {
        if( static_cast<uint32_t>( c ) >= 0x80 )
            return false;
    }

    return true;
}


bool wxString::IsNumber() const
{
    if( m_impl.empty() )
        return true;

    size_t i = 0;

    if( m_impl[0] == L'-' || m_impl[0] == L'+' )
        i++;

    for( ; i < m_impl.size(); ++i )
    {
        if( !std::iswdigit( m_impl[i] ) )
            return false;
    }

    return true;
}


bool wxString::IsWord() const
{
    for( wchar_t c : m_impl )
    {
        if( !std::iswalpha( c ) )
            return false;
    }

    return true;
}


// wxString: numbers (same conventions as wxWidgets: the value is stored when a number was
// read, the result says whether the whole string was a number).

bool wxString::ToLong( long* aValue, int aBase ) const
{
    const wchar_t* start = m_impl.c_str();
    wchar_t*       end;

    errno = 0;
    long val = std::wcstol( start, &end, aBase );

    if( end == start || errno == ERANGE )
        return false;

    *aValue = val;
    return !*end;
}


bool wxString::ToULong( unsigned long* aValue, int aBase ) const
{
    const wchar_t* start = m_impl.c_str();
    wchar_t*       end;

    errno = 0;
    unsigned long val = std::wcstoul( start, &end, aBase );

    if( end == start || errno == ERANGE )
        return false;

    *aValue = val;
    return !*end;
}


bool wxString::ToLongLong( long long* aValue, int aBase ) const
{
    const wchar_t* start = m_impl.c_str();
    wchar_t*       end;

    errno = 0;
    long long val = std::wcstoll( start, &end, aBase );

    if( end == start || errno == ERANGE )
        return false;

    *aValue = val;
    return !*end;
}


bool wxString::ToULongLong( unsigned long long* aValue, int aBase ) const
{
    const wchar_t* start = m_impl.c_str();
    wchar_t*       end;

    errno = 0;
    unsigned long long val = std::wcstoull( start, &end, aBase );

    if( end == start || errno == ERANGE )
        return false;

    *aValue = val;
    return !*end;
}


bool wxString::ToInt( int* aValue, int aBase ) const
{
    const wchar_t* start = m_impl.c_str();
    wchar_t*       end;

    errno = 0;
    long val = std::wcstol( start, &end, aBase );

    if( end == start || errno == ERANGE || val < INT_MIN || val > INT_MAX )
        return false;

    *aValue = static_cast<int>( val );
    return !*end;
}


bool wxString::ToUInt( unsigned int* aValue, int aBase ) const
{
    const wchar_t* start = m_impl.c_str();
    wchar_t*       end;

    errno = 0;
    unsigned long val = std::wcstoul( start, &end, aBase );

    if( end == start || errno == ERANGE || val > UINT_MAX )
        return false;

    *aValue = static_cast<unsigned int>( val );
    return !*end;
}


bool wxString::ToDouble( double* aValue ) const
{
    const wchar_t* start = m_impl.c_str();
    wchar_t*       end;

    errno = 0;
    double val = std::wcstod( start, &end );

    if( end == start || errno == ERANGE )
        return false;

    *aValue = val;
    return !*end;
}


bool wxString::ToCDouble( double* aValue ) const
{
    const wchar_t* start = m_impl.c_str();
    wchar_t*       end;

    errno = 0;
    double val = cLocaleWcstod( start, &end );

    if( end == start || errno == ERANGE )
        return false;

    *aValue = val;
    return !*end;
}


wxString wxString::FromDouble( double aValue, int aPrecision )
{
    if( aPrecision == -1 )
        return Format( wxT( "%g" ), aValue );

    return Format( wxT( "%.*f" ), aPrecision, aValue );
}


wxString wxString::FromCDouble( double aValue, int aPrecision )
{
    std::ostringstream os;
    os.imbue( std::locale::classic() );

    if( aPrecision != -1 )
        os << std::fixed << std::setprecision( aPrecision );

    os << aValue;
    return wxString( os.str() );
}


// Free functions

wxString operator+( const wxString& aLeft, const wxString& aRight )
{
    wxString result( aLeft );
    result += aRight;
    return result;
}



wxString operator+( const wxString& aLeft, const char* aRight )
{
    return aLeft + wxString( aRight );
}


wxString operator+( const char* aLeft, const wxString& aRight )
{
    return wxString( aLeft ) + aRight;
}


wxString operator+( const wxString& aLeft, const wchar_t* aRight )
{
    return aLeft + wxString( aRight );
}


wxString operator+( const wchar_t* aLeft, const wxString& aRight )
{
    return wxString( aLeft ) + aRight;
}


wxString operator+( const wxString& aLeft, wxUniChar aRight )
{
    return aLeft + wxString( aRight );
}


wxString operator+( wxUniChar aLeft, const wxString& aRight )
{
    return wxString( aLeft ) + aRight;
}


wxString operator+( const wxString& aLeft, char aRight )
{
    return aLeft + wxString( aRight );
}


wxString operator+( char aLeft, const wxString& aRight )
{
    return wxString( aLeft ) + aRight;
}


wxString operator+( const wxString& aLeft, wchar_t aRight )
{
    return aLeft + wxString( aRight );
}


wxString operator+( wchar_t aLeft, const wxString& aRight )
{
    return wxString( aLeft ) + aRight;
}


wxString operator+( const wxString& aLeft, wxUniCharRef aRight )
{
    return aLeft + wxString( aRight );
}


wxString operator+( const wxString& aLeft, const wxCStrData& aRight )
{
    return aLeft + wxString( aRight );
}


wxString operator+( const wxCStrData& aLeft, const wxString& aRight )
{
    return wxString( aLeft ) + aRight;
}


std::ostream& operator<<( std::ostream& aStream, const wxString& aStr )
{
    return aStream << aStr.utf8_string();
}


std::wostream& operator<<( std::wostream& aStream, const wxString& aStr )
{
    return aStream << aStr.impl();
}


std::ostream& operator<<( std::ostream& aStream, const wxCStrData& aStr )
{
    return aStream << aStr.AsChar();
}


std::ostream& operator<<( std::ostream& aStream, const wxScopedCharBuffer& aStr )
{
    return aStream << aStr.data();
}


std::ostream& operator<<( std::ostream& aStream, const wxScopedWCharBuffer& aStr )
{
    return aStream << wxString( aStr ).c_str().AsChar();
}


int wxStringSortAscending( const wxString& a, const wxString& b )
{
    return a.Cmp( b );
}


int wxStringSortDescending( const wxString& a, const wxString& b )
{
    return b.Cmp( a );
}


int wxCmpNatural( const wxString& a, const wxString& b )
{
    size_t i = 0;
    size_t j = 0;
    const std::wstring& s1 = a.impl();
    const std::wstring& s2 = b.impl();

    while( i < s1.size() && j < s2.size() )
    {
        if( std::iswdigit( s1[i] ) && std::iswdigit( s2[j] ) )
        {
            size_t ei = i;
            size_t ej = j;

            while( ei < s1.size() && std::iswdigit( s1[ei] ) )
                ++ei;

            while( ej < s2.size() && std::iswdigit( s2[ej] ) )
                ++ej;

            unsigned long long n1 = std::wcstoull( s1.substr( i, ei - i ).c_str(), nullptr, 10 );
            unsigned long long n2 = std::wcstoull( s2.substr( j, ej - j ).c_str(), nullptr, 10 );

            if( n1 != n2 )
                return n1 < n2 ? -1 : 1;

            i = ei;
            j = ej;
            continue;
        }

        wint_t c1 = std::towlower( s1[i] );
        wint_t c2 = std::towlower( s2[j] );

        if( c1 != c2 )
            return c1 < c2 ? -1 : 1;

        ++i;
        ++j;
    }

    if( i < s1.size() )
        return 1;

    if( j < s2.size() )
        return -1;

    return 0;
}
