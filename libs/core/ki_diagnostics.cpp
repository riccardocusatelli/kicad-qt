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

#include <core/ki_assert.h>
#include <core/ki_log.h>
#include <core/ki_format.h>

#include <atomic>
#include <cstdint>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>


// ---------------------------------------------------------------------------------------------
// Assertions

static void defaultAssertHandler( const char* aFile, int aLine, const char* aFunc,
                                  const char* aCond, std::string_view aMsg )
{
    std::fprintf( stderr, "%s(%d): assert \"%s\" failed in %s()%s%.*s\n", aFile, aLine, aCond,
                  aFunc, aMsg.empty() ? "" : ": ", static_cast<int>( aMsg.size() ),
                  aMsg.data() );
}


static std::atomic<KI_ASSERT_HANDLER> s_assertHandler{ nullptr };


KI_ASSERT_HANDLER KiSetAssertHandler( KI_ASSERT_HANDLER aHandler )
{
    return s_assertHandler.exchange( aHandler );
}


void KiAssertFailed( const char* aFile, int aLine, const char* aFunc, const char* aCond,
                     std::string_view aMsg )
{
    KI_ASSERT_HANDLER handler = s_assertHandler.load();

    ( handler ? handler : defaultAssertHandler )( aFile, aLine, aFunc, aCond, aMsg );
}


// ---------------------------------------------------------------------------------------------
// Logging

/// True if @a aMask appears in the comma separated list in environment variable @a aVar.
/// A null @a aMask asks whether the list is non-empty.
static bool maskInEnv( const char* aVar, const char* aMask )
{
    const char* env = std::getenv( aVar );

    if( !env )
        return false;

    if( !aMask )
        return *env != '\0';

    size_t len = std::strlen( aMask );

    for( const char* p = env; *p; )
    {
        const char* end = std::strchr( p, ',' );
        size_t      n = end ? size_t( end - p ) : std::strlen( p );

        if( n == len && std::strncmp( p, aMask, n ) == 0 )
            return true;

        if( !end )
            break;

        p = end + 1;
    }

    return false;
}


static bool defaultFilter( KI_LOG::LEVEL aLevel, const char* aMask )
{
    switch( aLevel )
    {
    case KI_LOG::LEVEL::TRACE:   return maskInEnv( "WXTRACE", aMask ) || maskInEnv( "KICAD_TRACE", aMask );
    case KI_LOG::LEVEL::DEBUG:   return std::getenv( "KICAD_DEBUG_LOG" ) != nullptr;
    case KI_LOG::LEVEL::WARNING: return true;
    }

    return true;
}


static void defaultSink( KI_LOG::LEVEL aLevel, const char* aMask, const char* aMessage )
{
    switch( aLevel )
    {
    case KI_LOG::LEVEL::TRACE:   std::fprintf( stderr, "[%s] %s\n", aMask ? aMask : "", aMessage ); break;
    case KI_LOG::LEVEL::DEBUG:   std::fprintf( stderr, "Debug: %s\n", aMessage );                   break;
    case KI_LOG::LEVEL::WARNING: std::fprintf( stderr, "Warning: %s\n", aMessage );                 break;
    }
}


static std::atomic<KI_LOG::FILTER> s_filter{ nullptr };
static std::atomic<KI_LOG::SINK>   s_sink{ nullptr };


static std::string vformat( const char* aFormat, va_list aArgs )
{
    va_list copy;
    va_copy( copy, aArgs );
    int len = std::vsnprintf( nullptr, 0, aFormat, copy );
    va_end( copy );

    if( len <= 0 )
        return std::string();

    std::vector<char> buf( size_t( len ) + 1 );
    std::vsnprintf( buf.data(), buf.size(), aFormat, aArgs );
    return std::string( buf.data(), size_t( len ) );
}


namespace KI_LOG
{

void SetBackend( FILTER aFilter, SINK aSink )
{
    s_filter.store( aFilter );
    s_sink.store( aSink );
}


bool IsEnabled( LEVEL aLevel, const char* aMask )
{
    FILTER filter = s_filter.load();
    return ( filter ? filter : defaultFilter )( aLevel, aMask );
}


bool TraceActive()
{
    return IsEnabled( LEVEL::TRACE, nullptr );
}


void Write( LEVEL aLevel, const char* aMask, const char* aFormat, ... )
{
    va_list args;
    va_start( args, aFormat );
    std::string msg = vformat( aFormat, args );
    va_end( args );

    SINK sink = s_sink.load();
    ( sink ? sink : defaultSink )( aLevel, aMask, msg.c_str() );
}

} // namespace KI_LOG


std::string KiStrFormat( const char* aFormat, ... )
{
    va_list args;
    va_start( args, aFormat );
    std::string msg = vformat( aFormat, args );
    va_end( args );
    return msg;
}


std::string KiStrFormatUnchecked( const char* aFormat, ... )
{
    va_list args;
    va_start( args, aFormat );
    std::string msg = vformat( aFormat, args );
    va_end( args );
    return msg;
}


std::string KiWideToUtf8( const wchar_t* aText, size_t aLen )
{
    std::string out;
    out.reserve( aLen );

    for( size_t i = 0; i < aLen; ++i )
    {
        uint32_t cp = static_cast<uint32_t>( aText[i] );

        // wchar_t is UTF-16 on Windows: combine surrogate pairs
        if constexpr( sizeof( wchar_t ) == 2 )
        {
            if( cp >= 0xD800 && cp <= 0xDBFF && i + 1 < aLen
                && static_cast<uint32_t>( aText[i + 1] ) >= 0xDC00
                && static_cast<uint32_t>( aText[i + 1] ) <= 0xDFFF )
            {
                cp = 0x10000 + ( ( cp - 0xD800 ) << 10 ) + ( static_cast<uint32_t>( aText[i + 1] ) - 0xDC00 );
                ++i;
            }
        }

        if( cp > 0x10FFFF || ( cp >= 0xD800 && cp <= 0xDFFF ) )
            cp = 0xFFFD;

        if( cp <= 0x7F )
        {
            out += char( cp );
        }
        else if( cp <= 0x7FF )
        {
            out += char( 0xC0 | ( cp >> 6 ) );
            out += char( 0x80 | ( cp & 0x3F ) );
        }
        else if( cp <= 0xFFFF )
        {
            out += char( 0xE0 | ( cp >> 12 ) );
            out += char( 0x80 | ( ( cp >> 6 ) & 0x3F ) );
            out += char( 0x80 | ( cp & 0x3F ) );
        }
        else
        {
            out += char( 0xF0 | ( cp >> 18 ) );
            out += char( 0x80 | ( ( cp >> 12 ) & 0x3F ) );
            out += char( 0x80 | ( ( cp >> 6 ) & 0x3F ) );
            out += char( 0x80 | ( cp & 0x3F ) );
        }
    }

    return out;
}
