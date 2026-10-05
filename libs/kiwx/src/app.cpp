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

#include <wx/app.h>

#include <wx/config.h>
#include <wx/filename.h>
#include <wx/log.h>

#include <exception>


wxAppConsole* wxAppConsole::ms_instance = nullptr;


void wxCmdLineArgsArray::Init( int aArgc, char** aArgv )
{
    m_args.clear();

    // wxWidgets converts with wxConvLocal, UTF-8 here
    for( int i = 0; i < aArgc; ++i )
        m_args.push_back( wxString( aArgv[i], wxConvLocal ) );
}


wxCmdLineArgsArray::operator wchar_t**() const
{
    m_wide.clear();
    m_widePtrs.clear();

    for( const wxString& arg : m_args )
        m_wide.push_back( arg.ToStdWstring() );

    for( std::wstring& arg : m_wide )
        m_widePtrs.push_back( arg.data() );

    m_widePtrs.push_back( nullptr );
    return m_widePtrs.data();
}


wxAppConsole::wxAppConsole()
{
    ms_instance = this;
}


wxAppConsole::~wxAppConsole()
{
    if( ms_instance == this )
        ms_instance = nullptr;
}


int wxAppConsole::OnExit()
{
    // as wxWidgets: no config created any more, the existing one is deleted
    wxConfigBase::DontCreateOnDemand();
    delete wxConfigBase::Set( nullptr );
    return 0;
}


void wxAppConsole::OnAssertFailure( const wxChar* aFile, int aLine, const wxChar* aFunc,
                                    const wxChar* aCond, const wxChar* aMsg )
{
    kiwx::PrintAssert( aFile, aLine, aFunc, aCond, aMsg ? wxString( aMsg ) : wxString() );
}


wxString wxAppConsole::GetAppName() const
{
    wxString name = m_appName;

    // by default the name of the executable
    if( name.empty() && argv.size() )
        wxFileName::SplitPath( argv[0], nullptr, &name, nullptr );

    return name;
}


wxString wxAppConsole::GetAppDisplayName() const
{
    if( !m_appDisplayName.empty() )
        return m_appDisplayName;

    if( !m_appName.empty() )
        return m_appName;

    return GetAppName().Capitalize();
}


int wxEntry( int& aArgc, char** aArgv )
{
    wxAppConsole* app = wxAppConsole::GetInstance();

    if( !app )
        return -1;

    app->argc = aArgc;
    app->argv.Init( aArgc, aArgv );

    int ret = -1;

    try
    {
        if( app->OnInit() )
        {
            struct CALL_ON_EXIT
            {
                ~CALL_ON_EXIT() { wxAppConsole::GetInstance()->OnExit(); }
            } callOnExit;

            ret = app->OnRun();
        }
    }
    catch( ... )
    {
        app->OnUnhandledException();
        ret = -1;
    }

    // the pending log messages, then the log target, as wxEntryCleanup()
    wxLog::FlushActive();
    delete wxLog::SetActiveTarget( nullptr );
    return ret;
}
