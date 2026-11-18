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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <gestfich.h>
#include <wx/textfile.h>

#include <future>
#include <utility>

#include <api/api_utils.h>
#include <paths.h>
#include <pgm_base.h>
#include <api/python_manager.h>
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


static wxString readAllFromStream( wxInputStream* aStream )
{
    wxString result;

    if( aStream )
    {
        char buffer[4096];

        while( aStream->CanRead() )
        {
            aStream->Read( buffer, sizeof( buffer ) );
            result.Append( buffer, aStream->LastRead() );
        }
    }

    return result;
};


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

        for( const wxString& arg : aArgs )
            argBufs.emplace_back( arg.wc_str() );

        std::vector<const wchar_t*> syncArgs = { interpreterBuf.data() };

        for( const wxWCharBuffer& buf : argBufs )
            syncArgs.emplace_back( buf.data() );

        syncArgs.emplace_back( nullptr );

        wxLogTrace( traceApi, wxString::Format( "Execute sync: %s %s", m_interpreterPath, argsStr ) );

        wxProcess syncProcess;
        syncProcess.Redirect();

        long ret = wxExecute( syncArgs.data(), wxEXEC_BLOCK, &syncProcess, aEnv );

        aCallback( static_cast<int>( ret ), readAllFromStream( syncProcess.GetInputStream() ),
                   readAllFromStream( syncProcess.GetErrorStream() ) );

        return ret;
    }
}


long PYTHON_MANAGER::ExecuteSync( const std::vector<wxString>& aArgs,
                                  wxString* aStdout, wxString* aStderr,
                                  const wxExecuteEnv* aEnv )
{
    wxWCharBuffer interpreterBuf = m_interpreterPath.wc_str();
    std::vector<wxWCharBuffer> argBufs;
    argBufs.reserve( aArgs.size() );

    for( const wxString& arg : aArgs )
        argBufs.emplace_back( arg.wc_str() );

    std::vector<const wchar_t*> args = { interpreterBuf.data() };

    for( const wxWCharBuffer& buf : argBufs )
        args.emplace_back( buf.data() );

    args.emplace_back( nullptr );

    wxString argsStr;

    for( const wxString& arg : aArgs )
        argsStr << arg << " ";

    wxLogTrace( traceApi, wxString::Format( "Execute sync: %s %s", m_interpreterPath, argsStr ) );

    wxProcess process;
    process.Redirect();

    long ret = wxExecute( args.data(), wxEXEC_BLOCK, &process, aEnv );

    if( aStdout )
        *aStdout = readAllFromStream( process.GetInputStream() );

    if( aStderr )
        *aStderr = readAllFromStream( process.GetErrorStream() );

    return ret;
}


wxString PYTHON_MANAGER::FindPythonInterpreter()
{
    // First, attempt to use a Python we distribute with KiCad
#if defined( __WINDOWS__ )
    wxFileName pythonExe = FindKicadFile( "pythonw.exe" );

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


static wxString venvPythonPath( const wxString& aEnvPath )
{
    wxFileName python( aEnvPath, wxEmptyString );

#ifdef _WIN32
    python.AppendDir( "Scripts" );
    python.SetFullName( "pythonw.exe" );
#else
    python.AppendDir( "bin" );
    python.SetFullName( "python" );
#endif

    return python.GetFullPath();
}


std::optional<wxString> PYTHON_MANAGER::GetVenvInterpreter( const wxString& aEnvPath )
{
    wxFileName cfg( aEnvPath, wxS( "pyvenv.cfg" ) );

    if( !cfg.IsFileReadable() )
        return std::nullopt;

    wxTextFile textFile( cfg.GetFullPath() );

    if( !textFile.Open() )
        return std::nullopt;

    wxString executable;

    for( wxString line = textFile.GetFirstLine(); !textFile.Eof(); line = textFile.GetNextLine() )
    {
        if( line.Trim( true ).Trim( false ).StartsWith( wxS( "executable" ) ) )
        {
            executable = line.AfterFirst( '=' ).Trim( true ).Trim( false );
            break;
        }
    }

    textFile.Close();

    if( executable.IsEmpty() )
        return std::nullopt;

    return executable;
}


bool PYTHON_MANAGER::IsVenvUsable( const wxString& aEnvPath )
{
    if( wxString pythonPath = venvPythonPath( aEnvPath ); !wxFileName( pythonPath ).IsFileExecutable() )
        return false;

    if( std::optional<wxString> interpreter = GetVenvInterpreter( aEnvPath ) )
    {
        if( wxFileName base( *interpreter ); !base.IsFileExecutable() )
            return false;
    }

    return true;
}


static wxString canonicalizePath( const wxString& aPath )
{
#ifndef _WIN32
    char buffer[PATH_MAX];

    if( char* resolved = realpath( aPath.ToUTF8().data(), buffer ) )
        return wxString::FromUTF8( resolved );
#endif

    wxFileName fn( aPath );
    fn.Normalize( FN_NORMALIZE_FLAGS );
    return fn.GetFullPath();
}


bool PYTHON_MANAGER::IsVenvStale( const wxString& aEnvPath, const wxString& aConfiguredInterpreter )
{
    if( std::optional<wxString> executable = GetVenvInterpreter( aEnvPath ) )
        return canonicalizePath( *executable ) != canonicalizePath( aConfiguredInterpreter );

    wxFileName cfg( aEnvPath, wxS( "pyvenv.cfg" ) );

    if( !cfg.IsFileReadable() )
        return false;

    wxTextFile textFile( cfg.GetFullPath() );

    if( !textFile.Open() )
        return false;

    wxString home;

    for( wxString line = textFile.GetFirstLine(); !textFile.Eof(); line = textFile.GetNextLine() )
    {
        if( line.Trim( true ).Trim( false ).StartsWith( wxS( "home" ) ) )
        {
            home = line.AfterFirst( '=' ).Trim( true ).Trim( false );
            break;
        }
    }

    textFile.Close();

    if( home.IsEmpty() )
        return false;

    wxFileName configuredFn( canonicalizePath( aConfiguredInterpreter ) );
    return canonicalizePath( home ) != configuredFn.GetPath();
}
