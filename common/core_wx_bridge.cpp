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
 * @file core_wx_bridge.cpp
 * Routes the toolkit-independent assertions and logging of the low level libraries
 * (core_base, kimath, sexpr, kinng) to wxWidgets, so wx applications behave exactly as when
 * those libraries called wxASSERT / wxLogTrace directly: wxSetAssertHandler(), the QA assert
 * throwers, WXTRACE masks and log levels keep working.
 *
 * Lives in kicommon (a shared library), so the registration runs whenever kicommon is loaded.
 * The handler and backend themselves are stored in kicore_rt, so the copies of kimath linked
 * into kigal and every kiface see them too.
 */

#include <core/ki_assert.h>
#include <core/ki_log.h>

#include <wx/debug.h>
#include <wx/log.h>
#include <wx/string.h>


static void wxAssertBridge( const char* aFile, int aLine, const char* aFunc, const char* aCond,
                            std::string_view aMsg )
{
#if wxDEBUG_LEVEL
    wxOnAssert( aFile, aLine, aFunc, aCond,
                wxString::FromUTF8( aMsg.data(), aMsg.size() ) );
#else
    wxUnusedVar( aFile ), wxUnusedVar( aLine ), wxUnusedVar( aFunc ), wxUnusedVar( aCond );
    wxUnusedVar( aMsg );
#endif
}


static bool wxLogFilter( KI_LOG::LEVEL aLevel, const char* aMask )
{
    if( !wxLog::IsEnabled() )
        return false;

    switch( aLevel )
    {
    case KI_LOG::LEVEL::TRACE:
        // Fast path for hot loops (triangulation): no masks set means no string conversion.
        if( !aMask || wxLog::GetTraceMasks().empty() )
            return false;

        return wxLog::IsAllowedTraceMask( wxString::FromUTF8( aMask ) );

    case KI_LOG::LEVEL::DEBUG:
        return wxLog::IsLevelEnabled( wxLOG_Debug, wxString::FromAscii( wxLOG_COMPONENT ) );

    case KI_LOG::LEVEL::WARNING:
        return wxLog::IsLevelEnabled( wxLOG_Warning, wxString::FromAscii( wxLOG_COMPONENT ) );
    }

    return true;
}


static void wxLogSink( KI_LOG::LEVEL aLevel, const char* aMask, const char* aMessage )
{
    wxString msg = wxString::FromUTF8( aMessage );

    switch( aLevel )
    {
    case KI_LOG::LEVEL::TRACE:   wxLogTrace( wxString::FromUTF8( aMask ), wxS( "%s" ), msg ); break;
    case KI_LOG::LEVEL::DEBUG:   wxLogDebug( wxS( "%s" ), msg );                               break;
    case KI_LOG::LEVEL::WARNING: wxLogWarning( wxS( "%s" ), msg );                             break;
    }
}


namespace
{

struct CORE_WX_BRIDGE
{
    CORE_WX_BRIDGE()
    {
        KiSetAssertHandler( &wxAssertBridge );
        KI_LOG::SetBackend( &wxLogFilter, &wxLogSink );
    }

    ~CORE_WX_BRIDGE()
    {
        // kicommon may be unloaded before the libraries' last users: fall back to stderr.
        KiSetAssertHandler( nullptr );
        KI_LOG::SetBackend( nullptr, nullptr );
    }
};

CORE_WX_BRIDGE s_coreWxBridge;

} // namespace
