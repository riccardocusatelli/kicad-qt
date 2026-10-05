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

#include <wx/strconv.h>
#include <wx/string.h>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <iconv.h>


std::string kiwxToUTF8( const wchar_t* aStr, size_t aLen )
{
    std::string out;
    out.reserve( aLen );

    for( size_t i = 0; i < aLen; ++i )
    {
        uint32_t c = static_cast<uint32_t>( aStr[i] );

        if( c < 0x80 )
        {
            out += static_cast<char>( c );
        }
        else if( c < 0x800 )
        {
            out += static_cast<char>( 0xC0 | ( c >> 6 ) );
            out += static_cast<char>( 0x80 | ( c & 0x3F ) );
        }
        else if( c < 0x10000 )
        {
            out += static_cast<char>( 0xE0 | ( c >> 12 ) );
            out += static_cast<char>( 0x80 | ( ( c >> 6 ) & 0x3F ) );
            out += static_cast<char>( 0x80 | ( c & 0x3F ) );
        }
        else
        {
            out += static_cast<char>( 0xF0 | ( c >> 18 ) );
            out += static_cast<char>( 0x80 | ( ( c >> 12 ) & 0x3F ) );
            out += static_cast<char>( 0x80 | ( ( c >> 6 ) & 0x3F ) );
            out += static_cast<char>( 0x80 | ( c & 0x3F ) );
        }
    }

    return out;
}


bool kiwxFromUTF8( const char* aStr, size_t aLen, std::wstring& aOut )
{
    aOut.clear();
    aOut.reserve( aLen );

    const unsigned char* p = reinterpret_cast<const unsigned char*>( aStr );
    const unsigned char* end = p + aLen;

    while( p < end )
    {
        unsigned char c = *p;

        if( c < 0x80 )
        {
            aOut += static_cast<wchar_t>( c );
            ++p;
            continue;
        }

        int      extra;
        uint32_t code;
        uint32_t min;

        if( ( c & 0xE0 ) == 0xC0 )
        {
            extra = 1;
            code = c & 0x1F;
            min = 0x80;
        }
        else if( ( c & 0xF0 ) == 0xE0 )
        {
            extra = 2;
            code = c & 0x0F;
            min = 0x800;
        }
        else if( ( c & 0xF8 ) == 0xF0 )
        {
            extra = 3;
            code = c & 0x07;
            min = 0x10000;
        }
        else
        {
            return false;
        }

        if( end - p <= extra )
            return false;

        for( int i = 1; i <= extra; ++i )
        {
            if( ( p[i] & 0xC0 ) != 0x80 )
                return false;

            code = ( code << 6 ) | ( p[i] & 0x3F );
        }

        if( code < min || code > 0x10FFFF || ( code >= 0xD800 && code <= 0xDFFF ) )
            return false;

        aOut += static_cast<wchar_t>( code );
        p += extra + 1;
    }

    return true;
}


namespace
{

/// Length of a NUL-terminated multibyte string whose NUL is @a aNulLen bytes long.
size_t mbLength( const char* aSrc, size_t aNulLen )
{
    if( aNulLen == 1 )
        return std::strlen( aSrc );

    size_t len = 0;

    for( ;; len += aNulLen )
    {
        bool allZero = true;

        for( size_t i = 0; i < aNulLen; ++i )
        {
            if( aSrc[len + i] )
            {
                allZero = false;
                break;
            }
        }

        if( allZero )
            return len;
    }
}

} // namespace


size_t wxMBConv::ToWChar( wchar_t* aDst, size_t aDstLen, const char* aSrc, size_t aSrcLen ) const
{
    if( !aSrc )
        return wxCONV_FAILED;

    bool terminated = aSrcLen == wxNO_LEN;

    if( terminated )
        aSrcLen = mbLength( aSrc, GetMBNulLen() );

    std::wstring out;

    if( !Decode( aSrc, aSrcLen, out ) )
        return wxCONV_FAILED;

    size_t needed = out.size() + ( terminated ? 1 : 0 );

    if( aDst )
    {
        if( aDstLen < needed )
            return wxCONV_FAILED;

        std::copy( out.begin(), out.end(), aDst );

        if( terminated )
            aDst[out.size()] = 0;
    }

    return needed;
}


size_t wxMBConv::FromWChar( char* aDst, size_t aDstLen, const wchar_t* aSrc,
                            size_t aSrcLen ) const
{
    if( !aSrc )
        return wxCONV_FAILED;

    bool terminated = aSrcLen == wxNO_LEN;

    if( terminated )
        aSrcLen = std::wcslen( aSrc );

    std::string out;

    if( !Encode( aSrc, aSrcLen, out ) )
        return wxCONV_FAILED;

    size_t nul = GetMBNulLen();
    size_t needed = out.size() + ( terminated ? nul : 0 );

    if( aDst )
    {
        if( aDstLen < needed )
            return wxCONV_FAILED;

        std::copy( out.begin(), out.end(), aDst );

        if( terminated )
            std::memset( aDst + out.size(), 0, nul );
    }

    return needed;
}


wxWCharBuffer wxMBConv::cMB2WC( const char* aIn ) const
{
    if( !aIn )
        return wxWCharBuffer();

    std::wstring out;

    if( !Decode( aIn, mbLength( aIn, GetMBNulLen() ), out ) )
        return wxWCharBuffer();

    return wxWCharBuffer( out );
}


wxWCharBuffer wxMBConv::cMB2WC( const char* aIn, size_t aInLen, size_t* aOutLen ) const
{
    if( !aIn )
    {
        if( aOutLen )
            *aOutLen = wxCONV_FAILED;

        return wxWCharBuffer();
    }

    if( aInLen == wxNO_LEN )
        aInLen = mbLength( aIn, GetMBNulLen() );

    std::wstring out;

    if( !Decode( aIn, aInLen, out ) )
    {
        if( aOutLen )
            *aOutLen = wxCONV_FAILED;

        return wxWCharBuffer();
    }

    if( aOutLen )
        *aOutLen = out.size();

    return wxWCharBuffer( out );
}


wxCharBuffer wxMBConv::cWC2MB( const wchar_t* aIn ) const
{
    if( !aIn )
        return wxCharBuffer();

    std::string out;

    if( !Encode( aIn, std::wcslen( aIn ), out ) )
        return wxCharBuffer();

    return wxCharBuffer( out );
}


wxCharBuffer wxMBConv::cWC2MB( const wchar_t* aIn, size_t aInLen, size_t* aOutLen ) const
{
    if( !aIn )
    {
        if( aOutLen )
            *aOutLen = wxCONV_FAILED;

        return wxCharBuffer();
    }

    if( aInLen == wxNO_LEN )
        aInLen = std::wcslen( aIn );

    std::string out;

    if( !Encode( aIn, aInLen, out ) )
    {
        if( aOutLen )
            *aOutLen = wxCONV_FAILED;

        return wxCharBuffer();
    }

    if( aOutLen )
        *aOutLen = out.size();

    return wxCharBuffer( out );
}


size_t wxMBConv::MB2WC( wchar_t* aOut, const char* aIn, size_t aOutLen ) const
{
    size_t rc = ToWChar( aOut, aOutLen, aIn );

    if( rc != wxCONV_FAILED )
        rc--;   // the terminator is not counted

    return rc;
}


size_t wxMBConv::WC2MB( char* aOut, const wchar_t* aIn, size_t aOutLen ) const
{
    size_t rc = FromWChar( aOut, aOutLen, aIn );

    if( rc != wxCONV_FAILED )
        rc -= GetMBNulLen();

    return rc;
}


bool wxMBConvUTF8::Decode( const char* aSrc, size_t aLen, std::wstring& aOut ) const
{
    if( kiwxFromUTF8( aSrc, aLen, aOut ) )
        return true;

    if( m_options == MAP_INVALID_UTF8_NOT )
        return false;

    // Decode what is valid; map the invalid bytes.
    aOut.clear();
    size_t i = 0;

    while( i < aLen )
    {
        unsigned char c = static_cast<unsigned char>( aSrc[i] );
        size_t        len = c < 0x80 ? 1 : ( c & 0xE0 ) == 0xC0 ? 2 : ( c & 0xF0 ) == 0xE0 ? 3
                                                     : ( c & 0xF8 ) == 0xF0 ? 4 : 0;
        std::wstring  ch;

        if( len && i + len <= aLen && kiwxFromUTF8( aSrc + i, len, ch ) && ch.size() == 1 )
        {
            aOut += ch;
            i += len;
            continue;
        }

        if( m_options & MAP_INVALID_UTF8_TO_PUA )
        {
            aOut += static_cast<wchar_t>( 0xE000 + c );
        }
        else
        {
            aOut += L'\\';
            aOut += static_cast<wchar_t>( L'0' + ( c >> 6 ) );
            aOut += static_cast<wchar_t>( L'0' + ( ( c >> 3 ) & 7 ) );
            aOut += static_cast<wchar_t>( L'0' + ( c & 7 ) );
        }

        ++i;
    }

    return true;
}


bool wxMBConvUTF8::Encode( const wchar_t* aSrc, size_t aLen, std::string& aOut ) const
{
    for( size_t i = 0; i < aLen; ++i )
    {
        uint32_t c = static_cast<uint32_t>( aSrc[i] );

        if( c > 0x10FFFF || ( c >= 0xD800 && c <= 0xDFFF ) )
            return false;
    }

    aOut = kiwxToUTF8( aSrc, aLen );
    return true;
}


namespace
{

bool decodeUTF16( const char* aSrc, size_t aLen, std::wstring& aOut, bool aBigEndian )
{
    if( aLen % 2 )
        return false;

    aOut.clear();

    auto unit =
            [&]( size_t i ) -> uint32_t
            {
                unsigned char a = aSrc[i];
                unsigned char b = aSrc[i + 1];
                return aBigEndian ? ( a << 8 ) | b : ( b << 8 ) | a;
            };

    for( size_t i = 0; i < aLen; i += 2 )
    {
        uint32_t c = unit( i );

        if( c >= 0xD800 && c <= 0xDBFF )
        {
            if( i + 2 >= aLen )
                return false;

            uint32_t low = unit( i + 2 );

            if( low < 0xDC00 || low > 0xDFFF )
                return false;

            c = 0x10000 + ( ( c - 0xD800 ) << 10 ) + ( low - 0xDC00 );
            i += 2;
        }
        else if( c >= 0xDC00 && c <= 0xDFFF )
        {
            return false;
        }

        aOut += static_cast<wchar_t>( c );
    }

    return true;
}


bool encodeUTF16( const wchar_t* aSrc, size_t aLen, std::string& aOut, bool aBigEndian )
{
    aOut.clear();

    auto put =
            [&]( uint32_t u )
            {
                char hi = static_cast<char>( u >> 8 );
                char lo = static_cast<char>( u & 0xFF );
                aOut += aBigEndian ? hi : lo;
                aOut += aBigEndian ? lo : hi;
            };

    for( size_t i = 0; i < aLen; ++i )
    {
        uint32_t c = static_cast<uint32_t>( aSrc[i] );

        if( c > 0x10FFFF )
            return false;

        if( c >= 0x10000 )
        {
            c -= 0x10000;
            put( 0xD800 + ( c >> 10 ) );
            put( 0xDC00 + ( c & 0x3FF ) );
        }
        else
        {
            put( c );
        }
    }

    return true;
}


bool decodeUTF32( const char* aSrc, size_t aLen, std::wstring& aOut, bool aBigEndian )
{
    if( aLen % 4 )
        return false;

    aOut.clear();

    for( size_t i = 0; i < aLen; i += 4 )
    {
        const unsigned char* p = reinterpret_cast<const unsigned char*>( aSrc + i );
        uint32_t c = aBigEndian ? ( p[0] << 24 ) | ( p[1] << 16 ) | ( p[2] << 8 ) | p[3]
                                : ( p[3] << 24 ) | ( p[2] << 16 ) | ( p[1] << 8 ) | p[0];

        if( c > 0x10FFFF )
            return false;

        aOut += static_cast<wchar_t>( c );
    }

    return true;
}


bool encodeUTF32( const wchar_t* aSrc, size_t aLen, std::string& aOut, bool aBigEndian )
{
    aOut.clear();

    for( size_t i = 0; i < aLen; ++i )
    {
        uint32_t c = static_cast<uint32_t>( aSrc[i] );

        for( int b = 0; b < 4; ++b )
        {
            int shift = aBigEndian ? ( 3 - b ) * 8 : b * 8;
            aOut += static_cast<char>( ( c >> shift ) & 0xFF );
        }
    }

    return true;
}


class LATIN1_CONV : public wxMBConv
{
public:
    wxMBConv* Clone() const override { return new LATIN1_CONV(); }

    bool Decode( const char* aSrc, size_t aLen, std::wstring& aOut ) const override
    {
        aOut.resize( aLen );

        for( size_t i = 0; i < aLen; ++i )
            aOut[i] = static_cast<unsigned char>( aSrc[i] );

        return true;
    }

    bool Encode( const wchar_t* aSrc, size_t aLen, std::string& aOut ) const override
    {
        aOut.resize( aLen );

        for( size_t i = 0; i < aLen; ++i )
        {
            if( static_cast<uint32_t>( aSrc[i] ) > 0xFF )
                return false;

            aOut[i] = static_cast<char>( aSrc[i] );
        }

        return true;
    }
};


bool iconvConvert( const std::string& aFrom, const std::string& aTo, const char* aSrc,
                   size_t aLen, std::string& aOut )
{
    iconv_t cd = iconv_open( aTo.c_str(), aFrom.c_str() );

    if( cd == (iconv_t) -1 )
        return false;

    aOut.clear();
    char*  in = const_cast<char*>( aSrc );
    size_t inLeft = aLen;
    bool   ok = true;

    while( inLeft > 0 )
    {
        char   buf[4096];
        char*  out = buf;
        size_t outLeft = sizeof( buf );
        size_t rc = iconv( cd, &in, &inLeft, &out, &outLeft );

        aOut.append( buf, sizeof( buf ) - outLeft );

        if( rc == (size_t) -1 && errno != E2BIG )
        {
            ok = false;
            break;
        }
    }

    iconv_close( cd );
    return ok;
}


std::string charsetOf( wxFontEncoding aEncoding )
{
    switch( aEncoding )
    {
    case wxFONTENCODING_SYSTEM:
    case wxFONTENCODING_DEFAULT:
    case wxFONTENCODING_UTF8:       return "UTF-8";
    case wxFONTENCODING_UTF16BE:    return "UTF-16BE";
    case wxFONTENCODING_UTF16LE:    return "UTF-16LE";
    case wxFONTENCODING_UTF32BE:    return "UTF-32BE";
    case wxFONTENCODING_UTF32LE:    return "UTF-32LE";
    case wxFONTENCODING_UTF7:       return "UTF-7";
    case wxFONTENCODING_KOI8:       return "KOI8-R";
    case wxFONTENCODING_KOI8_U:     return "KOI8-U";
    case wxFONTENCODING_EUC_JP:     return "EUC-JP";
    case wxFONTENCODING_ISO2022_JP: return "ISO-2022-JP";
    case wxFONTENCODING_CP932:      return "SHIFT_JIS";
    case wxFONTENCODING_CP936:      return "GBK";
    case wxFONTENCODING_CP949:      return "EUC-KR";
    case wxFONTENCODING_CP950:      return "BIG5";
    case wxFONTENCODING_MACROMAN:   return "MACINTOSH";
    default:
        break;
    }

    if( aEncoding >= wxFONTENCODING_ISO8859_1 && aEncoding < wxFONTENCODING_ISO8859_MAX )
        return "ISO-8859-" + std::to_string( aEncoding - wxFONTENCODING_ISO8859_1 + 1 );

    static const int cp[] = { 437, 850, 852, 855, 866, 874, 932, 936, 949, 950, 1250, 1251,
                              1252, 1253, 1254, 1255, 1256, 1257, 1258, 1361 };

    if( aEncoding >= wxFONTENCODING_CP437 && aEncoding < wxFONTENCODING_CP12_MAX )
        return "CP" + std::to_string( cp[aEncoding - wxFONTENCODING_CP437] );

    return "UTF-8";
}


std::string normaliseCharset( std::string aName )
{
    std::transform( aName.begin(), aName.end(), aName.begin(),
                    []( unsigned char c ) { return std::toupper( c ); } );

    if( aName == "UTF8" )
        return "UTF-8";

    if( aName.rfind( "WINDOWS-", 0 ) == 0 )
        return "CP" + aName.substr( 8 );

    return aName;
}

} // namespace


bool wxMBConvUTF16LE::Decode( const char* aSrc, size_t aLen, std::wstring& aOut ) const
{
    return decodeUTF16( aSrc, aLen, aOut, false );
}


bool wxMBConvUTF16LE::Encode( const wchar_t* aSrc, size_t aLen, std::string& aOut ) const
{
    return encodeUTF16( aSrc, aLen, aOut, false );
}


bool wxMBConvUTF16BE::Decode( const char* aSrc, size_t aLen, std::wstring& aOut ) const
{
    return decodeUTF16( aSrc, aLen, aOut, true );
}


bool wxMBConvUTF16BE::Encode( const wchar_t* aSrc, size_t aLen, std::string& aOut ) const
{
    return encodeUTF16( aSrc, aLen, aOut, true );
}


bool wxMBConvUTF32LE::Decode( const char* aSrc, size_t aLen, std::wstring& aOut ) const
{
    return decodeUTF32( aSrc, aLen, aOut, false );
}


bool wxMBConvUTF32LE::Encode( const wchar_t* aSrc, size_t aLen, std::string& aOut ) const
{
    return encodeUTF32( aSrc, aLen, aOut, false );
}


bool wxMBConvUTF32BE::Decode( const char* aSrc, size_t aLen, std::wstring& aOut ) const
{
    return decodeUTF32( aSrc, aLen, aOut, true );
}


bool wxMBConvUTF32BE::Encode( const wchar_t* aSrc, size_t aLen, std::string& aOut ) const
{
    return encodeUTF32( aSrc, aLen, aOut, true );
}


wxCSConv::wxCSConv( const wxString& aCharset ) :
        m_charset( normaliseCharset( aCharset.ToStdString() ) )
{
    init();
}


wxCSConv::wxCSConv( wxFontEncoding aEncoding ) : m_charset( charsetOf( aEncoding ) )
{
    init();
}


wxCSConv::wxCSConv( const wxCSConv& aOther ) : m_charset( aOther.m_charset )
{
    init();
}


wxCSConv::~wxCSConv() = default;


wxCSConv& wxCSConv::operator=( const wxCSConv& aOther )
{
    m_charset = aOther.m_charset;
    init();
    return *this;
}


void wxCSConv::init()
{
    m_direct.reset();

    if( m_charset == "UTF-8" )
        m_direct = std::make_unique<wxMBConvUTF8>();
    else if( m_charset == "ISO-8859-1" || m_charset == "ISO8859-1" || m_charset == "LATIN1" )
        m_direct = std::make_unique<LATIN1_CONV>();
    else if( m_charset == "UTF-16LE" )
        m_direct = std::make_unique<wxMBConvUTF16LE>();
    else if( m_charset == "UTF-16BE" )
        m_direct = std::make_unique<wxMBConvUTF16BE>();
    else if( m_charset == "UTF-32LE" )
        m_direct = std::make_unique<wxMBConvUTF32LE>();
    else if( m_charset == "UTF-32BE" )
        m_direct = std::make_unique<wxMBConvUTF32BE>();

    if( m_direct )
    {
        m_ok = true;
        return;
    }

    iconv_t cd = iconv_open( "UTF-32LE", m_charset.c_str() );
    m_ok = cd != (iconv_t) -1;

    if( m_ok )
        iconv_close( cd );
}


bool wxCSConv::IsOk() const
{
    return m_ok;
}


size_t wxCSConv::GetMBNulLen() const
{
    return m_direct ? m_direct->GetMBNulLen() : 1;
}


bool wxCSConv::Decode( const char* aSrc, size_t aLen, std::wstring& aOut ) const
{
    if( m_direct )
        return m_direct->Decode( aSrc, aLen, aOut );

    std::string utf32;

    if( !m_ok || !iconvConvert( m_charset, "UTF-32LE", aSrc, aLen, utf32 ) )
        return false;

    return decodeUTF32( utf32.data(), utf32.size(), aOut, false );
}


bool wxCSConv::Encode( const wchar_t* aSrc, size_t aLen, std::string& aOut ) const
{
    if( m_direct )
        return m_direct->Encode( aSrc, aLen, aOut );

    std::string utf32;
    encodeUTF32( aSrc, aLen, utf32, false );

    return m_ok && iconvConvert( "UTF-32LE", m_charset, utf32.data(), utf32.size(), aOut );
}


bool wxConvAuto::Decode( const char* aSrc, size_t aLen, std::wstring& aOut ) const
{
    const unsigned char* p = reinterpret_cast<const unsigned char*>( aSrc );

    if( aLen >= 4 && p[0] == 0xFF && p[1] == 0xFE && p[2] == 0 && p[3] == 0 )
        return decodeUTF32( aSrc + 4, aLen - 4, aOut, false );

    if( aLen >= 4 && p[0] == 0 && p[1] == 0 && p[2] == 0xFE && p[3] == 0xFF )
        return decodeUTF32( aSrc + 4, aLen - 4, aOut, true );

    if( aLen >= 2 && p[0] == 0xFF && p[1] == 0xFE )
        return decodeUTF16( aSrc + 2, aLen - 2, aOut, false );

    if( aLen >= 2 && p[0] == 0xFE && p[1] == 0xFF )
        return decodeUTF16( aSrc + 2, aLen - 2, aOut, true );

    if( aLen >= 3 && p[0] == 0xEF && p[1] == 0xBB && p[2] == 0xBF )
        return kiwxFromUTF8( aSrc + 3, aLen - 3, aOut );

    if( kiwxFromUTF8( aSrc, aLen, aOut ) )
        return true;

    if( m_fallback == wxFONTENCODING_DEFAULT || m_fallback == wxFONTENCODING_SYSTEM
        || m_fallback == wxFONTENCODING_ISO8859_1 )
    {
        return LATIN1_CONV().Decode( aSrc, aLen, aOut );
    }

    return wxCSConv( m_fallback ).Decode( aSrc, aLen, aOut );
}


bool wxConvAuto::Encode( const wchar_t* aSrc, size_t aLen, std::string& aOut ) const
{
    aOut = kiwxToUTF8( aSrc, aLen );
    return true;
}


wxMBConvUTF8& wxGet_wxConvUTF8()
{
    static wxMBConvUTF8 conv;
    return conv;
}


wxMBConv& wxGet_wxConvLibc()
{
    static wxMBConvLibc conv;
    return conv;
}


wxCSConv& wxGet_wxConvLocal()
{
    static wxCSConv conv( wxFONTENCODING_UTF8 );
    return conv;
}


wxCSConv& wxGet_wxConvISO8859_1()
{
    static wxCSConv conv( wxFONTENCODING_ISO8859_1 );
    return conv;
}


wxMBConv*& wxGet_wxConvCurrent()
{
    static wxMBConv* conv = &wxGet_wxConvLibc();
    return conv;
}


wxMBConv*& wxGet_wxConvFileName()
{
    static wxMBConv* conv = &wxGet_wxConvUTF8();
    return conv;
}


wxMBConv*& wxGet_wxConvUI()
{
    static wxMBConv* conv = &wxGet_wxConvUTF8();
    return conv;
}
