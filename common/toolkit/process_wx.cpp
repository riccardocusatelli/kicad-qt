/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <toolkit/process.h>

#include <wx/app.h>
#include <wx/log.h>
#include <wx/process.h>
#include <wx/timer.h>

#include <thread_pool.h>


namespace
{

/// A process of ExecuteAsync(): reads its output when it ends.
class ASYNC_PROCESS : public wxProcess
{
public:
    ASYNC_PROCESS( TOOLKIT::PROCESS_END_HANDLER aOnEnd, size_t aMaxOutput ) :
            wxProcess(),
            m_onEnd( std::move( aOnEnd ) ),
            m_maxOutput( aMaxOutput )
    {}

    void OnTerminate( int aPid, int aStatus ) override
    {
        // Print stdout trace info from the monitor thread
        wxLog::GetActiveTarget()->Flush();

        if( m_onEnd )
            m_onEnd( aStatus, readAll( GetInputStream() ), readAll( GetErrorStream() ) );

        // The process deletes itself, unless the monitor thread may still read its output
        if( !m_monitored )
            wxProcess::OnTerminate( aPid, aStatus );
    }

    /// A thread reads the output of the process while it runs.
    void SetMonitored() { m_monitored = true; }

private:
    wxString readAll( wxInputStream* aStream ) const
    {
        wxString text;
        size_t   bytesRead = 0;

        while( aStream && aStream->CanRead() && bytesRead < m_maxOutput )
        {
            char buffer[4096];
            buffer[ aStream->Read( buffer, sizeof( buffer ) - 1 ).LastRead() ] = '\0';
            text.append( buffer, aStream->LastRead() );
            bytesRead += aStream->LastRead();
        }

        return text;
    }

    TOOLKIT::PROCESS_END_HANDLER m_onEnd;
    size_t                       m_maxOutput;
    bool                         m_monitored = false;
};

}


namespace TOOLKIT
{

long ExecuteAsync( const std::vector<wxString>& aArgv, const wxExecuteEnv* aEnv,
                   size_t aMaxOutput, bool aHideConsole, PROCESS_OUTPUT_HANDLER aOnOutput,
                   PROCESS_END_HANDLER aOnEnd )
{
    ASYNC_PROCESS* process = new ASYNC_PROCESS( std::move( aOnEnd ), aMaxOutput );
    process->Redirect();

    std::vector<const wchar_t*> args;

    for( const wxString& arg : aArgv )
        args.emplace_back( arg.wc_str() );

    args.emplace_back( nullptr );

    int  flags = wxEXEC_ASYNC | ( aHideConsole ? wxEXEC_HIDE_CONSOLE : 0 );
    long pid = wxExecute( const_cast<wchar_t**>( args.data() ), flags, process, aEnv );

    if( pid == 0 )
    {
        delete process;
        return 0;
    }

    // On Windows, if there is a lot of stdout written by the process, this can
    // hang up the wxProcess such that it will never call OnTerminate.  To work
    // around this, we use this monitor thread to give the stdout to the output handler
    // (which dumps it to the trace log), which prevents the hangup.  Without an output
    // handler, the output is read at the end, for the commands where we need to read it
    // directly (such as the detection of the version of an interpreter).
    if( aOnOutput )
    {
        auto monitor =
            [aOnOutput]( wxProcess* aProcess )
            {
                wxInputStream* processOut = aProcess->GetInputStream();

                while( aProcess->IsInputOpened() )
                {
                    if( processOut->CanRead() )
                    {
                        char buffer[4096];
                        buffer[processOut->Read( buffer, sizeof( buffer ) - 1 ).LastRead()] = '\0';
                        aOnOutput( wxString( buffer, processOut->LastRead() ) );
                    }
                }
            };

        process->SetMonitored();

        thread_pool& tp = GetKiCadThreadPool();
        auto ret = tp.submit_task( [monitor, process] { monitor( process ); } );
    }

    return pid;
}


void RunOnMainThread( std::function<void()> aFunction, int aDelayMs )
{
    if( !wxTheApp )
        return;

    if( aDelayMs <= 0 )
    {
        wxTheApp->CallAfter( std::move( aFunction ) );
        return;
    }

    // A timer of the main thread, deleted once it fired
    wxTheApp->CallAfter(
            [aFunction = std::move( aFunction ), aDelayMs]()
            {
                wxTimer* timer = new wxTimer();

                timer->Bind( wxEVT_TIMER,
                             [aFunction, timer]( wxTimerEvent& )
                             {
                                 // The timer is deleted after its event
                                 wxTheApp->CallAfter( [timer]() { delete timer; } );
                                 aFunction();
                             } );

                timer->StartOnce( aDelayMs );
            } );
}

} // namespace TOOLKIT
