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

#include <wx/crt.h>   // as wxWidgets, through wx/hashmap.h
#include <wx/arrstr.h>
#include <wx/string.h>

#include <cstdio>
#include <ctime>

typedef unsigned long wxLogLevel;

enum wxLogLevelValues
{
    wxLOG_FatalError,
    wxLOG_Error,
    wxLOG_Warning,
    wxLOG_Message,
    wxLOG_Status,
    wxLOG_Info,
    wxLOG_Debug,
    wxLOG_Trace,
    wxLOG_Progress,
    wxLOG_User = 100,
    wxLOG_Max = 10000
};

#define wxTraceMemAlloc 0x0001
#define wxTraceMessages 0x0002
#define wxTraceResAlloc 0x0004
#define wxTraceRefCount 0x0008

#define wxLOG_COMPONENT "kiwx"


class wxLogRecordInfo
{
public:
    wxLogRecordInfo() = default;
    wxLogRecordInfo( const char* aFile, int aLine, const char* aFunc, const char* aComponent ) :
            filename( aFile ), line( aLine ), func( aFunc ), component( aComponent ),
            timestamp( std::time( nullptr ) )
    {
    }

    const char* filename = nullptr;
    int         line = 0;
    const char* func = nullptr;
    const char* component = nullptr;
    time_t      timestamp = 0;
    long long   timestampMS = 0;
    unsigned long threadId = 0;
};


/**
 * The log target.  The default target writes to stderr as wxLogStderr does
 * ("HH:MM:SS: Error: message").
 */
class wxLogChain;

class wxLog
{
public:
    wxLog() = default;
    virtual ~wxLog() = default;

    static wxLog* SetActiveTarget( wxLog* aTarget );
    static wxLog* GetActiveTarget();
    static wxLog* SetThreadActiveTarget( wxLog* aTarget ) { return SetActiveTarget( aTarget ); }

    static bool EnableLogging( bool aEnable = true );
    static bool IsEnabled();
    static void SetLogLevel( wxLogLevel aLevel );
    static wxLogLevel GetLogLevel();
    static bool IsLevelEnabled( wxLogLevel aLevel, const wxString& aComponent = wxString() );
    static void SetComponentLevel( const wxString&, wxLogLevel ) {}

    static void SetVerbose( bool aVerbose = true );
    static bool GetVerbose();

    static void AddTraceMask( const wxString& aMask );
    static void RemoveTraceMask( const wxString& aMask );
    static void ClearTraceMasks();
    static const wxArrayString& GetTraceMasks();
    static bool IsAllowedTraceMask( const wxString& aMask );

    static void SetTimestamp( const wxString& aFormat );
    static void DisableTimestamp() { SetTimestamp( wxString() ); }
    static const wxString& GetTimestamp();

    static void SetRepetitionCounting( bool = true ) {}
    static void DontCreateOnDemand() {}
    static void Suspend() {}
    static void Resume() {}

    static void FlushActive();
    virtual void Flush() {}

    /// Log a message through the active target (wxLogger::Log()).
    static void OnLog( wxLogLevel aLevel, const wxString& aMessage, const wxLogRecordInfo& aInfo );

protected:
    friend class wxLogChain;

    virtual void DoLogRecord( wxLogLevel aLevel, const wxString& aMsg,
                              const wxLogRecordInfo& aInfo );
    virtual void DoLogTextAtLevel( wxLogLevel aLevel, const wxString& aMsg );
    virtual void DoLogText( const wxString& aMsg );
};


/// Writes the messages to a stream (stderr by default).
class wxLogStderr : public wxLog
{
public:
    explicit wxLogStderr( FILE* aStream = nullptr ) : m_stream( aStream ? aStream : stderr ) {}

protected:
    void DoLogText( const wxString& aMsg ) override;

private:
    FILE* m_stream;
};


/// Collects the messages; Flush() writes them to stderr.
class wxLogBuffer : public wxLog
{
public:
    const wxString& GetBuffer() const { return m_str; }
    void            Flush() override;

protected:
    void DoLogText( const wxString& aMsg ) override;

private:
    wxString m_str;
};


/// Disables logging while it exists.
class wxLogNull
{
public:
    wxLogNull() : m_wasEnabled( wxLog::EnableLogging( false ) ) {}
    ~wxLogNull() { wxLog::EnableLogging( m_wasEnabled ); }

private:
    bool m_wasEnabled;
};


/// Installs a log target and passes the messages on to the previous one.
class wxLogChain : public wxLog
{
public:
    explicit wxLogChain( wxLog* aLogger );
    ~wxLogChain() override;

    void   SetLog( wxLog* aLogger );
    void   PassMessages( bool aPass ) { m_passMessages = aPass; }
    bool   IsPassingMessages() const { return m_passMessages; }
    wxLog* GetOldLog() const { return m_old; }

protected:
    void DoLogRecord( wxLogLevel aLevel, const wxString& aMsg,
                      const wxLogRecordInfo& aInfo ) override;

private:
    wxLog* m_old = nullptr;
    wxLog* m_new = nullptr;
    bool   m_passMessages = true;
};


/// The last error of the C library (errno).
unsigned long wxSysErrorCode();
const wxChar* wxSysErrorMsg( unsigned long aErrCode = 0 );
wxString      wxSysErrorMsgStr( unsigned long aErrCode = 0 );


namespace kiwx
{
void Log( wxLogLevel aLevel, const wxString& aMessage, const char* aFile, int aLine,
          const char* aFunc );
void LogTrace( const wxString& aMask, const wxString& aMessage, const char* aFile, int aLine,
               const char* aFunc );
} // namespace kiwx


#define KIWX_LOG_FUNCTION( name, level, suffix )                                               \
    template <typename... ARGS>                                                                \
    inline void name( const wxString& aFormat, const ARGS&... aArgs )                          \
    {                                                                                          \
        if( !wxLog::IsLevelEnabled( level ) )                                                  \
            return;                                                                            \
                                                                                               \
        kiwx::Log( level, wxString::Format( aFormat, aArgs... ) + suffix, nullptr, 0,          \
                   nullptr );                                                                  \
    }

KIWX_LOG_FUNCTION( wxLogFatalError, wxLOG_FatalError, wxString() )
KIWX_LOG_FUNCTION( wxLogError, wxLOG_Error, wxString() )
KIWX_LOG_FUNCTION( wxLogWarning, wxLOG_Warning, wxString() )
KIWX_LOG_FUNCTION( wxLogMessage, wxLOG_Message, wxString() )
KIWX_LOG_FUNCTION( wxLogStatus, wxLOG_Status, wxString() )
KIWX_LOG_FUNCTION( wxLogInfo, wxLOG_Info, wxString() )
KIWX_LOG_FUNCTION( wxLogDebug, wxLOG_Debug, wxString() )

#undef KIWX_LOG_FUNCTION

template <typename... ARGS>
inline void wxLogVerbose( const wxString& aFormat, const ARGS&... aArgs )
{
    if( wxLog::GetVerbose() && wxLog::IsLevelEnabled( wxLOG_Info ) )
        kiwx::Log( wxLOG_Info, wxString::Format( aFormat, aArgs... ), nullptr, 0, nullptr );
}

template <typename... ARGS>
inline void wxLogSysError( const wxString& aFormat, const ARGS&... aArgs )
{
    if( !wxLog::IsLevelEnabled( wxLOG_Error ) )
        return;

    unsigned long err = wxSysErrorCode();
    kiwx::Log( wxLOG_Error,
               wxString::Format( aFormat, aArgs... )
                       + wxString::Format( wxT( " (error %lu: %s)" ), err, wxSysErrorMsgStr( err ) ),
               nullptr, 0, nullptr );
}

template <typename... ARGS>
inline void wxLogTrace( const wxString& aMask, const wxString& aFormat, const ARGS&... aArgs )
{
    if( wxLog::IsAllowedTraceMask( aMask ) )
        kiwx::LogTrace( aMask, wxString::Format( aFormat, aArgs... ), nullptr, 0, nullptr );
}

template <typename... ARGS>
inline void wxLogTrace( const char* aMask, const wxString& aFormat, const ARGS&... aArgs )
{
    wxLogTrace( wxString( aMask ), aFormat, aArgs... );
}

template <typename... ARGS>
inline void wxLogTrace( const wchar_t* aMask, const wxString& aFormat, const ARGS&... aArgs )
{
    wxLogTrace( wxString( aMask ), aFormat, aArgs... );
}

/// Show a message without the GUI (it can be called in any state): on the standard error.
void wxSafeShowMessage( const wxString& aTitle, const wxString& aText );

#define wxLOG_IS_ENABLED( level ) wxLog::IsLevelEnabled( wxLOG_##level )

#define wxLogGeneric( level, ... )                                                             \
    kiwx::Log( level, wxString::Format( __VA_ARGS__ ), __FILE__, __LINE__, __func__ )

#ifndef _WIN32
#pragma GCC visibility pop
#endif
