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
 * @file format.cpp
 *
 * The printf-like formatting of wxString::Format() and friends.  The arguments arrive
 * normalised (wxFormatArg); each conversion is formatted as the C library would format the
 * argument wxWidgets passes to vswprintf(): integers are read with the size given by the
 * length modifier, floating point and pointers by the C library itself (same locale rules).
 */

#include <wx/string.h>

#include <cstdio>
#include <string>


namespace
{

struct SPEC
{
    std::string flags;
    int         width = -1;
    int         precision = -1;
    std::string length;
    wchar_t     conversion = 0;
};


const wxFormatArg* argAt( const wxFormatArg* aArgs, size_t aCount, size_t aIndex )
{
    return aIndex < aCount ? &aArgs[aIndex] : nullptr;
}


long long intValue( const wxFormatArg& aArg )
{
    switch( aArg.m_type )
    {
    case wxFormatArg::INT:
    case wxFormatArg::CHAR:        return aArg.m_int;
    case wxFormatArg::UINT:        return static_cast<long long>( aArg.m_uint );
    case wxFormatArg::DOUBLE:      return static_cast<long long>( aArg.m_double );
    case wxFormatArg::LONG_DOUBLE: return static_cast<long long>( aArg.m_longDouble );
    case wxFormatArg::POINTER:     return reinterpret_cast<intptr_t>( aArg.m_pointer );
    case wxFormatArg::STRING:      return 0;
    }

    return 0;
}


/// The bits of the argument as the C library reads them with this length modifier.
long long signedAs( long long aValue, const std::string& aLength )
{
    if( aLength == "hh" )
        return static_cast<signed char>( aValue );

    if( aLength == "h" )
        return static_cast<short>( aValue );

    if( aLength.empty() )
        return static_cast<int>( aValue );

    return aValue;
}


unsigned long long unsignedAs( long long aValue, const std::string& aLength )
{
    if( aLength == "hh" )
        return static_cast<unsigned char>( aValue );

    if( aLength == "h" )
        return static_cast<unsigned short>( aValue );

    if( aLength.empty() )
        return static_cast<unsigned int>( aValue );

    return static_cast<unsigned long long>( aValue );
}


std::string cSpec( const SPEC& aSpec, const char* aLength, char aConversion )
{
    std::string spec = "%" + aSpec.flags;

    if( aSpec.width >= 0 )
        spec += std::to_string( aSpec.width );

    if( aSpec.precision >= 0 )
        spec += "." + std::to_string( aSpec.precision );

    spec += aLength;
    spec += aConversion;
    return spec;
}


template <typename T>
std::wstring cFormat( const std::string& aSpec, T aValue )
{
    char buf[512];
    int  len = std::snprintf( buf, sizeof( buf ), aSpec.c_str(), aValue );

    if( len < 0 )
        return std::wstring();

    if( static_cast<size_t>( len ) < sizeof( buf ) )
        return std::wstring( buf, buf + len );

    std::string big( len + 1, '\0' );
    std::snprintf( big.data(), big.size(), aSpec.c_str(), aValue );
    return std::wstring( big.begin(), big.begin() + len );
}


std::wstring padded( std::wstring aText, const SPEC& aSpec )
{
    if( aSpec.width > 0 && aText.size() < static_cast<size_t>( aSpec.width ) )
    {
        std::wstring pad( aSpec.width - aText.size(), L' ' );

        if( aSpec.flags.find( '-' ) != std::string::npos )
            aText += pad;
        else
            aText = pad + aText;
    }

    return aText;
}


std::wstring formatOne( const SPEC& aSpec, const wxFormatArg* aArg )
{
    if( !aArg )
        return std::wstring();

    switch( aSpec.conversion )
    {
    case L'd':
    case L'i':
        return cFormat( cSpec( aSpec, "ll", 'd' ), signedAs( intValue( *aArg ), aSpec.length ) );

    case L'u':
    case L'o':
    case L'x':
    case L'X':
        return cFormat( cSpec( aSpec, "ll", static_cast<char>( aSpec.conversion ) ),
                        unsignedAs( intValue( *aArg ), aSpec.length ) );

    case L'f':
    case L'F':
    case L'e':
    case L'E':
    case L'g':
    case L'G':
    case L'a':
    case L'A':
    {
        char conv = static_cast<char>( aSpec.conversion );

        if( aSpec.length == "L" )
        {
            long double v = aArg->m_type == wxFormatArg::LONG_DOUBLE
                                    ? aArg->m_longDouble
                                    : aArg->m_type == wxFormatArg::DOUBLE
                                              ? aArg->m_double
                                              : static_cast<long double>( intValue( *aArg ) );
            return cFormat( cSpec( aSpec, "L", conv ), v );
        }

        double v = aArg->m_type == wxFormatArg::DOUBLE || aArg->m_type == wxFormatArg::LONG_DOUBLE
                           ? aArg->m_double
                           : static_cast<double>( intValue( *aArg ) );
        return cFormat( cSpec( aSpec, "", conv ), v );
    }

    case L'p':
        return cFormat( cSpec( aSpec, "", 'p' ),
                        aArg->m_type == wxFormatArg::POINTER
                                ? aArg->m_pointer
                                : reinterpret_cast<const void*>( intValue( *aArg ) ) );

    case L'c':
    case L'C':
    {
        std::wstring text( 1, static_cast<wchar_t>( aArg->m_type == wxFormatArg::STRING
                                                            ? ( aArg->m_string.empty()
                                                                        ? 0
                                                                        : aArg->m_string[0] )
                                                            : intValue( *aArg ) ) );
        return padded( text, aSpec );
    }

    case L's':
    case L'S':
    {
        std::wstring text;

        if( aArg->m_type == wxFormatArg::STRING )
        {
            if( aArg->m_isNull )
                text = aSpec.precision >= 0 && aSpec.precision < 6 ? L"" : L"(null)";
            else
                text = aArg->m_string;
        }
        else if( aArg->m_type == wxFormatArg::CHAR )
        {
            text = std::wstring( 1, static_cast<wchar_t>( aArg->m_int ) );
        }
        else if( aArg->m_type == wxFormatArg::DOUBLE || aArg->m_type == wxFormatArg::LONG_DOUBLE )
        {
            text = cFormat( std::string( "%g" ), aArg->m_double );
        }
        else
        {
            text = std::to_wstring( intValue( *aArg ) );
        }

        if( aSpec.precision >= 0 && text.size() > static_cast<size_t>( aSpec.precision ) )
            text.resize( aSpec.precision );

        return padded( text, aSpec );
    }

    default:
        return std::wstring();
    }
}

} // namespace


wxString kiwx::Format( const wxString& aFormat, const wxFormatArg* aArgs, size_t aCount )
{
    const std::wstring& fmt = aFormat.impl();
    std::wstring        out;
    size_t              nextArg = 0;
    size_t              i = 0;

    out.reserve( fmt.size() + 16 * aCount );

    auto readNumber =
            [&]( size_t& aPos ) -> int
            {
                int n = 0;

                while( aPos < fmt.size() && fmt[aPos] >= L'0' && fmt[aPos] <= L'9' )
                    n = n * 10 + ( fmt[aPos++] - L'0' );

                return n;
            };

    while( i < fmt.size() )
    {
        wchar_t c = fmt[i];

        if( c != L'%' )
        {
            out += c;
            ++i;
            continue;
        }

        size_t start = i++;

        if( i < fmt.size() && fmt[i] == L'%' )
        {
            out += L'%';
            ++i;
            continue;
        }

        SPEC   spec;
        size_t argIndex = nextArg;
        bool   positional = false;

        // Positional argument: %n$
        size_t look = i;

        if( look < fmt.size() && fmt[look] >= L'1' && fmt[look] <= L'9' )
        {
            int n = readNumber( look );

            if( look < fmt.size() && fmt[look] == L'$' )
            {
                argIndex = n - 1;
                positional = true;
                i = look + 1;
            }
        }

        while( i < fmt.size() && wcschr( L"-+ #0'", fmt[i] ) && fmt[i] )
            spec.flags += static_cast<char>( fmt[i++] );

        if( i < fmt.size() && fmt[i] == L'*' )
        {
            ++i;
            const wxFormatArg* w = argAt( aArgs, aCount, argIndex++ );
            spec.width = w ? static_cast<int>( intValue( *w ) ) : 0;

            if( spec.width < 0 )
            {
                spec.flags += '-';
                spec.width = -spec.width;
            }
        }
        else if( i < fmt.size() && fmt[i] >= L'0' && fmt[i] <= L'9' )
        {
            spec.width = readNumber( i );
        }

        if( i < fmt.size() && fmt[i] == L'.' )
        {
            ++i;

            if( i < fmt.size() && fmt[i] == L'*' )
            {
                ++i;
                const wxFormatArg* p = argAt( aArgs, aCount, argIndex++ );
                spec.precision = p ? static_cast<int>( intValue( *p ) ) : 0;

                if( spec.precision < 0 )
                    spec.precision = -1;
            }
            else
            {
                spec.precision = readNumber( i );
            }
        }

        // Length modifiers
        while( i < fmt.size() && wcschr( L"hlLqjzt", fmt[i] ) && fmt[i] )
        {
            wchar_t m = fmt[i++];

            if( m == L'q' || m == L'j' || m == L'z' || m == L't' )
                spec.length += "ll";
            else
                spec.length += static_cast<char>( m );
        }

        if( fmt.compare( i, 3, L"I64" ) == 0 )
        {
            spec.length = "ll";
            i += 3;
        }
        else if( fmt.compare( i, 3, L"I32" ) == 0 )
        {
            i += 3;
        }

        if( spec.length == "lll" || spec.length == "llll" )
            spec.length = "ll";

        if( i >= fmt.size() )
        {
            out.append( fmt, start, std::wstring::npos );
            break;
        }

        spec.conversion = fmt[i++];

        if( spec.conversion == L'n' )
        {
            argIndex++;
        }
        else if( wcschr( L"diuoxXfFeEgGaAcCsSp", spec.conversion ) )
        {
            out += formatOne( spec, argAt( aArgs, aCount, argIndex ) );
            argIndex++;
        }
        else
        {
            // Unknown conversion: copied as is.
            out.append( fmt, start, i - start );
            continue;
        }

        if( !positional )
            nextArg = argIndex;
    }

    return wxString( std::move( out ) );
}
