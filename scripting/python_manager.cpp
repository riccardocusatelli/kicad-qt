/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Jon Evans <jon@craftyjon.com>
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

#include <config.h>
#include <gestfich.h>

#include <future>
#include <utility>

#include <api/api_utils.h>
#include <paths.h>
#include <pgm_base.h>
#include <python_manager.h>
#include <toolkit/process.h>
#include <wx_filename.h>
#include <core/ki_log.h>


/// The maximum length of the output of the Python interpreter given to the callbacks
static constexpr size_t MAX_OUTPUT_LEN = 1024L * 1024L;


PYTHON_MANAGER::PYTHON_MANAGER( const wxString& aInterpreterPath )
{
    wxFileName path( aInterpreterPath );
    path.Normalize( FN_NORMALIZE_FLAGS );
    m_interpreterPath = path.GetFullPath();
}


long PYTHON_MANAGER::Execute( const std::vector<wxString>& aArgs,
        const std::function<void(int, const wxString&, const wxString&)>& aCallback,
        const wxExecuteEnv* aEnv, bool aSaveOutput )
{
    wxString              argsStr;
    std::vector<wxString> args = { m_interpreterPath };

    for( const wxString& arg : aArgs )
    {
        args.emplace_back( arg );
        argsStr << arg << " ";
    }

    // Unless the output is needed (which is currently only used for detecting the interpreter
    // version), it is dumped to the trace log while the process runs (see TOOLKIT::ExecuteAsync)
    TOOLKIT::PROCESS_OUTPUT_HANDLER traceOutput;

    if( !aSaveOutput )
    {
        traceOutput =
                []( const wxString& aOutput )
                {
                    wxString stdOut = aOutput.BeforeLast( '\n' );
                    KI_LOG_TRACE( traceApi, wxString::Format( "Python: %s", stdOut ) );
                };
    }

    KI_LOG_TRACE( traceApi, wxString::Format( "Execute: %s %s", m_interpreterPath, argsStr ) );
    long pid = TOOLKIT::ExecuteAsync( args, aEnv, MAX_OUTPUT_LEN, false, traceOutput, aCallback );

    if( pid == 0 )
        aCallback( -1, wxEmptyString, _( "Process could not be created" ) );

    return pid;
}


wxString PYTHON_MANAGER::FindPythonInterpreter()
{
    // First, attempt to use a Python we distribute with KiCad
#if defined( __WINDOWS__ )
    wxFileName pythonExe = FindKicadFile( "pythonw.exe" );

    if( pythonExe.IsFileExecutable() )
        return pythonExe.GetFullPath();
#elif defined( __WXMAC__ )
    wxFileName pythonExe( PATHS::GetOSXKicadDataDir(), wxEmptyString );
    pythonExe.RemoveLastDir();
    pythonExe.AppendDir( wxT( "Frameworks" ) );
    pythonExe.AppendDir( wxT( "Python.framework" ) );
    pythonExe.AppendDir( wxT( "Versions" ) );
    pythonExe.AppendDir( wxT( "Current" ) );
    pythonExe.AppendDir( wxT( "bin" ) );
    pythonExe.SetFullName(wxT( "python3" ) );

    if( pythonExe.IsFileExecutable() )
        return pythonExe.GetFullPath();
#else
    wxFileName pythonExe;
#endif

    // In case one is forced with cmake
    pythonExe.Assign( wxString::FromUTF8Unchecked( PYTHON_EXECUTABLE ) );

    if( pythonExe.IsFileExecutable() )
        return pythonExe.GetFullPath();

    // Fall back on finding any Python in the user's path

#ifdef _WIN32
    wxArrayString output;

    if( 0 == wxExecute( wxS( "where pythonw.exe" ), output, wxEXEC_SYNC ) )
    {
        if( !output.IsEmpty() )
            return output[0];
    }
#else
    wxArrayString output;

    if( 0 == wxExecute( wxS( "which -a python3" ), output, wxEXEC_SYNC ) )
    {
        if( !output.IsEmpty() )
            return output[0];
    }

    if( 0 == wxExecute( wxS( "which -a python" ), output, wxEXEC_SYNC ) )
    {
        if( !output.IsEmpty() )
            return output[0];
    }
#endif

    return wxEmptyString;
}


std::optional<wxString> PYTHON_MANAGER::GetPythonEnvironment( const wxString& aNamespace )
{
    wxFileName path( PATHS::GetUserCachePath(), wxEmptyString );
    path.AppendDir( wxS( "python-environments" ) );
    path.AppendDir( aNamespace );

    if( !PATHS::EnsurePathExists( path.GetPath() ) )
        return std::nullopt;

    return path.GetPath();
}


std::optional<wxString> PYTHON_MANAGER::GetVirtualPython( const wxString& aNamespace )
{
    std::optional<wxString> envPath = GetPythonEnvironment( aNamespace );

    if( !envPath )
        return std::nullopt;

    wxFileName python( *envPath, wxEmptyString );

#ifdef _WIN32
    python.AppendDir( "Scripts" );
    python.SetFullName( "pythonw.exe" );
#else
    python.AppendDir( "bin" );
    python.SetFullName( "python" );
#endif

    if( !python.IsFileExecutable() )
        return std::nullopt;

    return python.GetFullPath();
}
