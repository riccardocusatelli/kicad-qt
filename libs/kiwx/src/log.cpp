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

#include <wx/debug.h>
#include <wx/intl.h>
#include <wx/app.h>
#include <wx/log.h>
#include <wx/tokenzr.h>

#include <atomic>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <mutex>


namespace
{

struct LOG_STATE
{
    std::recursive_mutex mutex;
    wxLog*               active = nullptr;
    bool                 activeCreated = false;   ///< the default target, created on demand
    bool                 enabled = true;
    bool                 verbose = false;
    wxLogLevel           level = wxLOG_Max;
    wxArrayString        traceMasks;
    bool                 traceMasksRead = false;
    wxString             timestamp = wxS( "%H:%M:%S" );
};


LOG_STATE& state()
{
    static LOG_STATE s;
    return s;
}


/// The trace masks of the WXTRACE environment variable (comma separated), read once.
void readTraceMasks( LOG_STATE& aState )
{
    if( aState.traceMasksRead )
        return;

    aState.traceMasksRead = true;

    if( const char* env = std::getenv( "WXTRACE" ) )
    {
        wxStringTokenizer tk( wxString( env ), wxT( "," ), wxTOKEN_STRTOK );

        while( tk.HasMoreTokens() )
            aState.traceMasks.Add( tk.GetNextToken() );
    }
}


wxString formatTime( const wxString& aFormat, time_t aTime )
{
    if( aFormat.empty() )
        return wxString();

    struct tm tmv;
    localtime_r( &aTime, &tmv );

    char buf[128];
    size_t len = std::strftime( buf, sizeof( buf ), aFormat.utf8_string().c_str(), &tmv );

    return wxString::FromUTF8( buf, len ) + wxS( ": " );
}

} // namespace


wxLog* wxLog::SetActiveTarget( wxLog* aTarget )
{
    LOG_STATE&                            s = state();
    std::lock_guard<std::recursive_mutex> lock( s.mutex );

    wxLog* old = s.active;

    if( s.activeCreated )
    {
        // The target created on demand belongs to the library.
        delete old;
        old = nullptr;
        s.activeCreated = false;
    }

    s.active = aTarget;
    return old;
}


wxLog* wxLog::GetActiveTarget()
{
    LOG_STATE&                            s = state();
    std::lock_guard<std::recursive_mutex> lock( s.mutex );

    if( !s.active )
    {
        s.active = new wxLogStderr();
        s.activeCreated = true;
    }

    return s.active;
}


bool wxLog::EnableLogging( bool aEnable )
{
    LOG_STATE& s = state();
    bool       old = s.enabled;
    s.enabled = aEnable;
    return old;
}


bool wxLog::IsEnabled()
{
    return state().enabled;
}


void wxLog::SetLogLevel( wxLogLevel aLevel )
{
    state().level = aLevel;
}


wxLogLevel wxLog::GetLogLevel()
{
    return state().level;
}


bool wxLog::IsLevelEnabled( wxLogLevel aLevel, const wxString& )
{
    return IsEnabled() && aLevel <= GetLogLevel();
}


void wxLog::SetVerbose( bool aVerbose )
{
    state().verbose = aVerbose;
}


bool wxLog::GetVerbose()
{
    return state().verbose;
}


void wxLog::AddTraceMask( const wxString& aMask )
{
    LOG_STATE&                            s = state();
    std::lock_guard<std::recursive_mutex> lock( s.mutex );
    readTraceMasks( s );
    s.traceMasks.Add( aMask );
}


void wxLog::RemoveTraceMask( const wxString& aMask )
{
    LOG_STATE&                            s = state();
    std::lock_guard<std::recursive_mutex> lock( s.mutex );
    readTraceMasks( s );
    s.traceMasks.Remove( aMask );
}


void wxLog::ClearTraceMasks()
{
    LOG_STATE&                            s = state();
    std::lock_guard<std::recursive_mutex> lock( s.mutex );
    readTraceMasks( s );
    s.traceMasks.Clear();
}


const wxArrayString& wxLog::GetTraceMasks()
{
    LOG_STATE&                            s = state();
    std::lock_guard<std::recursive_mutex> lock( s.mutex );
    readTraceMasks( s );
    return s.traceMasks;
}


bool wxLog::IsAllowedTraceMask( const wxString& aMask )
{
    LOG_STATE&                            s = state();
    std::lock_guard<std::recursive_mutex> lock( s.mutex );
    readTraceMasks( s );

    for( const wxString& mask : s.traceMasks )
    {
        if( mask == aMask || mask == wxS( "*" ) )
            return true;
    }

    return false;
}


void wxLog::SetTimestamp( const wxString& aFormat )
{
    state().timestamp = aFormat;
}


const wxString& wxLog::GetTimestamp()
{
    return state().timestamp;
}


void wxLog::FlushActive()
{
    if( wxLog* log = GetActiveTarget() )
        log->Flush();
}


void wxLog::OnLog( wxLogLevel aLevel, const wxString& aMessage, const wxLogRecordInfo& aInfo )
{
    if( !IsLevelEnabled( aLevel ) )
        return;

    if( wxLog* log = GetActiveTarget() )
        log->DoLogRecord( aLevel, aMessage, aInfo );

    if( aLevel == wxLOG_FatalError )
        std::abort();
}


void wxLog::DoLogRecord( wxLogLevel aLevel, const wxString& aMsg, const wxLogRecordInfo& aInfo )
{
    // The format of wxLogFormatter.
    wxString prefix = formatTime( GetTimestamp(), aInfo.timestamp ? aInfo.timestamp : std::time( nullptr ) );

    switch( aLevel )
    {
    case wxLOG_Error:   prefix += _( "Error: " );   break;
    case wxLOG_Warning: prefix += _( "Warning: " ); break;
    case wxLOG_Debug:   prefix += wxS( "Debug: " ); break;
    case wxLOG_Trace:   prefix += wxS( "Trace: " ); break;
    default: break;
    }

    DoLogTextAtLevel( aLevel, prefix + aMsg );
}


void wxLog::DoLogTextAtLevel( wxLogLevel aLevel, const wxString& aMsg )
{
    if( aLevel == wxLOG_Debug || aLevel == wxLOG_Trace )
    {
        std::fputs( ( aMsg.utf8_string() + "\n" ).c_str(), stderr );
        return;
    }

    DoLogText( aMsg );
}


void wxLog::DoLogText( const wxString& )
{
}


void wxLogStderr::DoLogText( const wxString& aMsg )
{
    std::fputs( ( aMsg.utf8_string() + "\n" ).c_str(), m_stream );
    std::fflush( m_stream );
}


void wxLogBuffer::DoLogText( const wxString& aMsg )
{
    m_str << aMsg << wxS( "\n" );
}


void wxLogBuffer::Flush()
{
    if( !m_str.empty() )
    {
        std::fputs( m_str.utf8_string().c_str(), stderr );
        m_str.clear();
    }
}


wxLogChain::wxLogChain( wxLog* aLogger ) : m_new( aLogger )
{
    m_old = wxLog::SetActiveTarget( this );
}


wxLogChain::~wxLogChain()
{
    wxLog::SetActiveTarget( m_old );

    if( m_new != this )
        delete m_new;
}


void wxLogChain::SetLog( wxLog* aLogger )
{
    if( m_new != this )
        delete m_new;

    m_new = aLogger;
}


void wxLogChain::DoLogRecord( wxLogLevel aLevel, const wxString& aMsg, const wxLogRecordInfo& aInfo )
{
    if( m_old && m_passMessages )
        m_old->DoLogRecord( aLevel, aMsg, aInfo );

    if( m_new && m_new != this )
        m_new->DoLogRecord( aLevel, aMsg, aInfo );
}


unsigned long wxSysErrorCode()
{
    return errno;
}


const wxChar* wxSysErrorMsg( unsigned long aErrCode )
{
    static thread_local wxString msg;
    msg = wxSysErrorMsgStr( aErrCode );
    return msg.wc_str();
}


wxString wxSysErrorMsgStr( unsigned long aErrCode )
{
    if( aErrCode == 0 )
        aErrCode = errno;

    char        buf[256];
    const char* text = strerror_r( static_cast<int>( aErrCode ), buf, sizeof( buf ) );
    return wxString( text );
}


void kiwx::Log( wxLogLevel aLevel, const wxString& aMessage, const char* aFile, int aLine,
                const char* aFunc )
{
    wxLog::OnLog( aLevel, aMessage, wxLogRecordInfo( aFile, aLine, aFunc, wxLOG_COMPONENT ) );
}


void kiwx::LogTrace( const wxString& aMask, const wxString& aMessage, const char* aFile, int aLine,
                     const char* aFunc )
{
    wxLog::OnLog( wxLOG_Trace, wxString::Format( wxS( "(%s) %s" ), aMask, aMessage ),
                  wxLogRecordInfo( aFile, aLine, aFunc, wxLOG_COMPONENT ) );
}


// Assertions

void kiwx::PrintAssert( const wxString& aFile, int aLine, const wxString& aFunc,
                        const wxString& aCond, const wxString& aMsg )
{
    // The text of wxWidgets' console assert handler.
    wxString text = wxString::Format( wxS( "%s(%d): assert \"%s\" failed" ), aFile, aLine, aCond );

    if( !aFunc.empty() )
        text += wxString::Format( wxS( " in %s()" ), aFunc );

    if( !aMsg.empty() )
        text += wxS( ": " ) + aMsg;

    std::fputs( ( text.utf8_string() + "\n" ).c_str(), stderr );
}


namespace
{

// As wxWidgets: the application handles the assert when there is one.
void defaultAssertHandler( const wxString& aFile, int aLine, const wxString& aFunc,
                           const wxString& aCond, const wxString& aMsg )
{
    if( wxAppConsole* app = wxAppConsole::GetInstance() )
    {
        app->OnAssertFailure( aFile.wc_str(), aLine, aFunc.wc_str(), aCond.wc_str(),
                              aMsg.wc_str() );
    }
    else
    {
        kiwx::PrintAssert( aFile, aLine, aFunc, aCond, aMsg );
    }
}

} // namespace


wxAssertHandler_t wxTheAssertHandler = defaultAssertHandler;
bool              wxTrapInAssert = false;


wxAssertHandler_t wxSetAssertHandler( wxAssertHandler_t aHandler )
{
    wxAssertHandler_t old = wxTheAssertHandler;
    wxTheAssertHandler = aHandler;
    return old;
}


void wxSetDefaultAssertHandler()
{
    wxTheAssertHandler = defaultAssertHandler;
}


bool wxIsDebuggerRunning()
{
    return false;
}


void wxOnAssert( const wxString& aFile, int aLine, const wxString& aFunc, const wxString& aCond,
                 const wxString& aMsg )
{
    static std::atomic<bool> inAssert( false );

    if( !wxTheAssertHandler || inAssert.exchange( true ) )
        return;

    wxTheAssertHandler( aFile, aLine, aFunc, aCond, aMsg );
    inAssert = false;
}


void wxOnAssert( const char* aFile, int aLine, const char* aFunc, const char* aCond,
                 const char* aMsg )
{
    wxOnAssert( wxString( aFile ), aLine, wxString( aFunc ), wxString( aCond ),
                aMsg ? wxString( aMsg ) : wxString() );
}


void wxOnAssert( const char* aFile, int aLine, const char* aFunc, const char* aCond,
                 const wxString& aMsg )
{
    wxOnAssert( wxString( aFile ), aLine, wxString( aFunc ), wxString( aCond ), aMsg );
}


void wxOnAssert( const char* aFile, int aLine, const char* aFunc, const char* aCond,
                 const wchar_t* aMsg )
{
    wxOnAssert( wxString( aFile ), aLine, wxString( aFunc ), wxString( aCond ),
                aMsg ? wxString( aMsg ) : wxString() );
}
