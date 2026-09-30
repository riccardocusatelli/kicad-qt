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
 * @file ki_log.h
 * Toolkit-independent logging for the low level libraries.
 *
 *   wxLogTrace( mask, fmt, ... )  ->  KI_TRACE( mask, fmt, ... )
 *   wxLogDebug( fmt, ... )        ->  KI_LOG_DEBUG( fmt, ... )
 *   wxLogWarning( fmt, ... )      ->  KI_LOG_WARNING( fmt, ... )
 *
 * Formats are printf style (UTF-8 char strings).  Without a backend, trace masks are enabled
 * through the WXTRACE or KICAD_TRACE environment variables (comma separated, as in wx), debug
 * messages through KICAD_DEBUG_LOG, and everything goes to stderr.  GUI applications install
 * a backend that forwards to their own logger (see common/core_wx_bridge.cpp).
 */

#ifndef KI_LOG_H
#define KI_LOG_H

#include <string>

#include <core/kicore_rt.h>

#if defined( __GNUC__ ) || defined( __clang__ )
#define KI_PRINTF_FORMAT( fmtIdx, argIdx ) __attribute__( ( format( printf, fmtIdx, argIdx ) ) )
#else
#define KI_PRINTF_FORMAT( fmtIdx, argIdx )
#endif


namespace KI_LOG
{

enum class LEVEL
{
    TRACE,
    DEBUG,
    WARNING
};

/// Return true if messages of @a aLevel (and trace @a aMask, for TRACE) should be emitted.
using FILTER = bool ( * )( LEVEL aLevel, const char* aMask );

/// Emit an already formatted UTF-8 message.
using SINK = void ( * )( LEVEL aLevel, const char* aMask, const char* aMessage );

/// Install a backend; nullptr for either argument restores the default one.
KICORE_RT_API void SetBackend( FILTER aFilter, SINK aSink );

KICORE_RT_API bool IsEnabled( LEVEL aLevel, const char* aMask = nullptr );

KICORE_RT_API void Write( LEVEL aLevel, const char* aMask, const char* aFormat, ... )
        KI_PRINTF_FORMAT( 3, 4 );

} // namespace KI_LOG


/// printf-style formatting into a std::string (replacement for wxString::Format in the libs).
KICORE_RT_API std::string KiStrFormat( const char* aFormat, ... ) KI_PRINTF_FORMAT( 1, 2 );


#define KI_TRACE( mask, ... )                                                        \
    do                                                                               \
    {                                                                                \
        if( KI_LOG::IsEnabled( KI_LOG::LEVEL::TRACE, mask ) )                        \
            KI_LOG::Write( KI_LOG::LEVEL::TRACE, mask, __VA_ARGS__ );                \
    } while( 0 )

#define KI_LOG_DEBUG( ... )                                                          \
    do                                                                               \
    {                                                                                \
        if( KI_LOG::IsEnabled( KI_LOG::LEVEL::DEBUG ) )                              \
            KI_LOG::Write( KI_LOG::LEVEL::DEBUG, nullptr, __VA_ARGS__ );             \
    } while( 0 )

#define KI_LOG_WARNING( ... ) KI_LOG::Write( KI_LOG::LEVEL::WARNING, nullptr, __VA_ARGS__ )

#endif // KI_LOG_H
