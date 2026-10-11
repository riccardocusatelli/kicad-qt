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
 * @file utils_unix.cpp
 *
 * The services of the operating system on Unix (POSIX): files and directories, environment,
 * users, processes.  The Windows versions are in src/msw/utils_msw.cpp.
 */

#include <wx/crt.h>
#include <wx/dir.h>
#include <wx/ffile.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/intl.h>
#include <wx/log.h>
#include <wx/mstream.h>
#include <wx/process.h>
#include <wx/tokenzr.h>
#include <wx/txtstrm.h>
#include <wx/utils.h>

#include "../kiwx_private.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <thread>

#include <dirent.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utime.h>

extern char** environ;


namespace
{

std::string fsPath( const wxString& aPath )
{
    return aPath.utf8_string();
}


/// The path without its trailing separators (but "/" stays "/").
wxString stripTrailingSeparators( const wxString& aPath )
{
    wxString path = aPath;

    while( path.length() > 1 && path.Last() == wxT( '/' ) )
        path.RemoveLast();

    return path;
}

} // namespace


namespace
{

struct FIND_STATE
{
    wxArrayString matches;
    size_t        next = 0;
};

FIND_STATE& findState()
{
    static thread_local FIND_STATE state;
    return state;
}

} // namespace


namespace
{

/// Split a command line as wxWidgets does: whitespace separated, quotes group words.
std::vector<std::string> splitCommand( const wxString& aCommand )
{
    std::vector<std::string> args;
    std::string              cmd = aCommand.utf8_string();
    std::string              current;
    char                     quote = 0;
    bool                     inArg = false;

    for( size_t i = 0; i < cmd.size(); ++i )
    {
        char c = cmd[i];

        if( quote )
        {
            if( c == quote )
                quote = 0;
            else if( c == '\\' && quote == '"' && i + 1 < cmd.size()
                     && ( cmd[i + 1] == '"' || cmd[i + 1] == '\\' ) )
                current += cmd[++i];
            else
                current += c;
        }
        else if( c == '"' || c == '\'' )
        {
            quote = c;
            inArg = true;
        }
        else if( c == ' ' || c == '\t' )
        {
            if( inArg )
            {
                args.push_back( current );
                current.clear();
                inArg = false;
            }
        }
        else if( c == '\\' && i + 1 < cmd.size() )
        {
            current += cmd[++i];
            inArg = true;
        }
        else
        {
            current += c;
            inArg = true;
        }
    }

    if( inArg )
        args.push_back( current );

    return args;
}


// Runs a command; with aOutput / aErrors, its standard output / error are captured.
long run( const std::vector<std::string>& aArgs, int aFlags, const wxExecuteEnv* aEnv,
          std::string* aOutput, std::string* aErrors )
{
    if( aArgs.empty() )
        return aFlags & wxEXEC_SYNC ? -1 : 0;

    int outPipe[2] = { -1, -1 };
    int errPipe[2] = { -1, -1 };

    if( aOutput && pipe( outPipe ) != 0 )
        return -1;

    if( aErrors && pipe( errPipe ) != 0 )
        return -1;

    pid_t pid = fork();

    if( pid < 0 )
        return aFlags & wxEXEC_SYNC ? -1 : 0;

    if( pid == 0 )
    {
        if( aOutput )
        {
            dup2( outPipe[1], STDOUT_FILENO );
            close( outPipe[0] );
            close( outPipe[1] );
        }

        if( aErrors )
        {
            dup2( errPipe[1], STDERR_FILENO );
            close( errPipe[0] );
            close( errPipe[1] );
        }

        if( aEnv )
        {
            if( !aEnv->cwd.empty() && chdir( aEnv->cwd.utf8_string().c_str() ) != 0 )
                _exit( 127 );

            if( !aEnv->env.empty() )
            {
                clearenv();

                for( const auto& [name, value] : aEnv->env )
                    setenv( name.utf8_string().c_str(), value.utf8_string().c_str(), 1 );
            }
        }

        std::vector<char*> argv;

        for( const std::string& arg : aArgs )
            argv.push_back( const_cast<char*>( arg.c_str() ) );

        argv.push_back( nullptr );
        execvp( argv[0], argv.data() );
        _exit( 127 );
    }

    auto readAll =
            []( int aFd, std::string* aData )
            {
                char    buf[4096];
                ssize_t n;

                while( ( n = read( aFd, buf, sizeof( buf ) ) ) > 0 )
                    aData->append( buf, n );

                close( aFd );
            };

    if( aOutput )
        close( outPipe[1] );

    if( aErrors )
        close( errPipe[1] );

    if( aOutput && aErrors )
    {
        std::thread errReader( readAll, errPipe[0], aErrors );
        readAll( outPipe[0], aOutput );
        errReader.join();
    }
    else if( aOutput )
    {
        readAll( outPipe[0], aOutput );
    }
    else if( aErrors )
    {
        readAll( errPipe[0], aErrors );
    }

    if( !( aFlags & wxEXEC_SYNC ) && !aOutput && !aErrors )
        return pid;

    int status = 0;

    if( waitpid( pid, &status, 0 ) < 0 )
        return -1;

    return WIFEXITED( status ) ? WEXITSTATUS( status ) : -1;
}


long runForProcess( const std::vector<std::string>& aArgs, int aFlags, wxProcess* aProcess,
                    const wxExecuteEnv* aEnv )
{
    if( !aProcess || !aProcess->IsRedirected() )
        return run( aArgs, aFlags, aEnv, nullptr, nullptr );

    // the output is read until the end: the command runs synchronously
    std::string out, err;
    long        ret = run( aArgs, aFlags | wxEXEC_SYNC, aEnv, &out, &err );
    aProcess->SetCapturedOutput( out, err );
    return ret;
}

} // namespace


int wxChmod( const wxString& aPath, int aMode )
{
    return chmod( fsPath( aPath ).c_str(), aMode );
}


bool wxIsWritable( const wxString& aPath )
{
    return access( fsPath( aPath ).c_str(), W_OK ) == 0;
}


bool wxIsReadable( const wxString& aPath )
{
    return access( fsPath( aPath ).c_str(), R_OK ) == 0;
}


bool wxIsExecutable( const wxString& aPath )
{
    return access( fsPath( aPath ).c_str(), X_OK ) == 0;
}


wxString wxFindFirstFile( const wxString& aSpec, int aFlags )
{
    FIND_STATE& state = findState();
    state.matches.clear();
    state.next = 0;

    wxString dir = wxPathOnly( aSpec );
    wxString mask = wxFileNameFromPath( aSpec );

    if( dir.empty() )
        dir = wxT( "." );

    if( DIR* d = opendir( fsPath( dir ).c_str() ) )
    {
        while( struct dirent* e = readdir( d ) )
        {
            wxString name( e->d_name );

            if( name == wxT( "." ) || name == wxT( ".." ) )
                continue;

            if( wxMatchWild( mask, name ) )
            {
                wxString full = aSpec.Contains( wxT( "/" ) ) ? dir + wxT( "/" ) + name : name;
                bool     isDir = wxDirExists( dir + wxT( "/" ) + name );

                if( aFlags == 0 || ( isDir && ( aFlags & 2 ) ) || ( !isDir && ( aFlags & 1 ) ) )
                    state.matches.Add( full );
            }
        }

        closedir( d );
    }

    return wxFindNextFile();
}


wxString wxRealPath( const wxString& aPath )
{
    char buf[PATH_MAX];
    return realpath( fsPath( aPath ).c_str(), buf ) ? wxString( buf ) : aPath;
}


wxFileKind wxGetFileKind( int aFd )
{
    struct stat st;

    if( fstat( aFd, &st ) != 0 )
        return wxFILE_KIND_UNKNOWN;

    if( isatty( aFd ) )
        return wxFILE_KIND_TERMINAL;

    if( S_ISFIFO( st.st_mode ) )
        return wxFILE_KIND_PIPE;

    return wxFILE_KIND_DISK;
}


bool wxGetEnv( const wxString& aVar, wxString* aValue )
{
    const char* v = std::getenv( fsPath( aVar ).c_str() );

    if( !v )
        return false;

    if( aValue )
        *aValue = wxString( v );

    return true;
}


bool wxSetEnv( const wxString& aVar, const wxString& aValue )
{
    return setenv( fsPath( aVar ).c_str(), fsPath( aValue ).c_str(), 1 ) == 0;
}


bool wxUnsetEnv( const wxString& aVar )
{
    return unsetenv( fsPath( aVar ).c_str() ) == 0;
}


bool wxGetEnvMap( wxEnvVariableHashMap* aMap )
{
    if( !aMap )
        return false;

    aMap->clear();

    for( char** e = environ; e && *e; ++e )
    {
        wxString entry( *e );
        aMap->emplace( entry.BeforeFirst( '=' ), entry.AfterFirst( '=' ) );
    }

    return true;
}


wxString wxGetHomeDir()
{
    wxString home;

    if( wxGetEnv( wxT( "HOME" ), &home ) && !home.empty() )
        return home;

    if( struct passwd* pw = getpwuid( getuid() ) )
        return wxString( pw->pw_dir );

    return wxT( "/" );
}


wxString wxGetUserHome( const wxString& aUser )
{
    if( aUser.empty() )
        return wxGetHomeDir();

    if( struct passwd* pw = getpwnam( fsPath( aUser ).c_str() ) )
        return wxString( pw->pw_dir );

    return wxString();
}


wxString wxGetUserId()
{
    if( struct passwd* pw = getpwuid( getuid() ) )
        return wxString( pw->pw_name );

    return wxString();
}


wxString wxGetUserName()
{
    if( struct passwd* pw = getpwuid( getuid() ) )
    {
        wxString gecos( pw->pw_gecos );
        gecos = gecos.BeforeFirst( ',' );
        return gecos.empty() ? wxString( pw->pw_name ) : gecos;
    }

    return wxString();
}


wxString wxGetHostName()
{
    char buf[256];

    if( gethostname( buf, sizeof( buf ) ) != 0 )
        return wxString();

    buf[sizeof( buf ) - 1] = 0;
    return wxString( buf ).BeforeFirst( '.' );
}


wxString wxGetFullHostName()
{
    char buf[256];

    if( gethostname( buf, sizeof( buf ) ) != 0 )
        return wxString();

    buf[sizeof( buf ) - 1] = 0;
    return wxString( buf );
}


unsigned long wxGetProcessId()
{
    return static_cast<unsigned long>( getpid() );
}


wxString wxGetOsDescription()
{
    struct utsname u;

    if( uname( &u ) != 0 )
        return wxT( "Unknown" );

    return wxString::Format( wxT( "%s %s %s" ), u.sysname, u.release, u.machine );
}


int wxGetOsVersion( int* aMajor, int* aMinor, int* aMicro )
{
    struct utsname u;
    int            major = 0;
    int            minor = 0;
    int            micro = 0;

    if( uname( &u ) == 0 )
        std::sscanf( u.release, "%d.%d.%d", &major, &minor, &micro );

    if( aMajor )
        *aMajor = major;

    if( aMinor )
        *aMinor = minor;

    if( aMicro )
        *aMicro = micro;

    return 0;
}


wxString wxGetCpuArchitectureName()
{
    struct utsname u;
    return uname( &u ) == 0 ? wxString( u.machine ) : wxString();
}


wxLinuxDistributionInfo wxGetLinuxDistributionInfo()
{
    wxLinuxDistributionInfo info;
    std::ifstream           in( "/etc/os-release" );
    std::string             line;

    auto value =
            []( const std::string& aLine )
            {
                std::string v = aLine.substr( aLine.find( '=' ) + 1 );

                if( v.size() >= 2 && v.front() == '"' && v.back() == '"' )
                    v = v.substr( 1, v.size() - 2 );

                return wxString::FromUTF8( v );
            };

    while( std::getline( in, line ) )
    {
        if( line.rfind( "ID=", 0 ) == 0 )
            info.Id = value( line ).Capitalize();
        else if( line.rfind( "VERSION_ID=", 0 ) == 0 )
            info.Release = value( line );
        else if( line.rfind( "VERSION_CODENAME=", 0 ) == 0 )
            info.CodeName = value( line );
        else if( line.rfind( "PRETTY_NAME=", 0 ) == 0 )
            info.Description = value( line );
    }

    return info;
}


long wxExecute( const wxString& aCommand, int aFlags, wxProcess* aProcess, const wxExecuteEnv* aEnv )
{
    return runForProcess( splitCommand( aCommand ), aFlags, aProcess, aEnv );
}


long wxExecute( const wxString& aCommand, wxArrayString& aOutput, int aFlags, const wxExecuteEnv* aEnv )
{
    std::string out;
    long        ret = run( splitCommand( aCommand ), aFlags | wxEXEC_SYNC, aEnv, &out, nullptr );
    kiwx::OutputLines( out, aOutput );
    return ret;
}


long wxExecute( const wxString& aCommand, wxArrayString& aOutput, wxArrayString& aErrors, int aFlags,
                const wxExecuteEnv* aEnv )
{
    std::string out, err;
    long        ret = run( splitCommand( aCommand ), aFlags | wxEXEC_SYNC, aEnv, &out, &err );
    kiwx::OutputLines( out, aOutput );
    kiwx::OutputLines( err, aErrors );
    return ret;
}


long wxExecute( const char* const* aArgv, int aFlags, wxProcess* aProcess, const wxExecuteEnv* aEnv )
{
    std::vector<std::string> args;

    for( ; aArgv && *aArgv; ++aArgv )
        args.emplace_back( *aArgv );

    return runForProcess( args, aFlags, aProcess, aEnv );
}


long wxExecute( const wchar_t* const* aArgv, int aFlags, wxProcess* aProcess, const wxExecuteEnv* aEnv )
{
    std::vector<std::string> args;

    for( ; aArgv && *aArgv; ++aArgv )
        args.push_back( wxString( *aArgv ).utf8_string() );

    return runForProcess( args, aFlags, aProcess, aEnv );
}


int wxKill( long aPid, wxSignal aSig, wxKillError* aRc, int aFlags )
{
    int  err = kill( static_cast<pid_t>( ( aFlags & wxKILL_CHILDREN ) ? -aPid : aPid ),
                     static_cast<int>( aSig ) );
    wxKillError rc = wxKILL_OK;

    if( err != 0 )
    {
        switch( errno )
        {
        case EINVAL: rc = wxKILL_BAD_SIGNAL; break;
        case EPERM:  rc = wxKILL_ACCESS_DENIED; break;
        case ESRCH:  rc = wxKILL_NO_PROCESS; break;
        default:     rc = wxKILL_ERROR; break;
        }
    }

    if( aRc )
        *aRc = rc;

    return err;
}


bool wxShell( const wxString& aCommand )
{
    std::string cmd = aCommand.empty() ? std::string( "/bin/sh" ) : aCommand.utf8_string();
    return std::system( cmd.c_str() ) == 0;
}


bool wxFileName::GetTimes( wxDateTime* aAccess, wxDateTime* aMod, wxDateTime* aCreate ) const
{
    struct stat st;

    if( stat( fsPath( GetFullPath() ).c_str(), &st ) != 0 )
        return false;

    auto toDateTime =
            []( const struct timespec& aTs )
            {
                return wxDateTime::FromMilliseconds( static_cast<long long>( aTs.tv_sec ) * 1000
                                                     + aTs.tv_nsec / 1000000 );
            };

    if( aAccess )
        *aAccess = toDateTime( st.st_atim );

    if( aMod )
        *aMod = toDateTime( st.st_mtim );

    if( aCreate )
        *aCreate = toDateTime( st.st_ctim );

    return true;
}


bool wxFileName::SetTimes( const wxDateTime* aAccess, const wxDateTime* aMod, const wxDateTime* ) const
{
    if( !aAccess && !aMod )
        return true;

    wxDateTime access, mod;

    if( !GetTimes( &access, &mod, nullptr ) )
        return false;

    struct timeval tv[2];
    long long      a = aAccess ? aAccess->GetValue().GetValue() : access.GetValue().GetValue();
    long long      m = aMod ? aMod->GetValue().GetValue() : mod.GetValue().GetValue();
    tv[0].tv_sec = a / 1000;
    tv[0].tv_usec = ( a % 1000 ) * 1000;
    tv[1].tv_sec = m / 1000;
    tv[1].tv_usec = ( m % 1000 ) * 1000;

    return utimes( fsPath( GetFullPath() ).c_str(), tv ) == 0;
}


bool wxFileName::Touch() const
{
    std::string path = fsPath( GetFullPath() );

    if( utime( path.c_str(), nullptr ) == 0 )
        return true;

    int fd = open( path.c_str(), O_WRONLY | O_CREAT, 0666 );

    if( fd < 0 )
        return false;

    close( fd );
    return true;
}


wxFileName wxFileName::ResolveLink()
{
    char        buf[PATH_MAX];
    std::string path = fsPath( GetFullPath() );
    ssize_t     len = readlink( path.c_str(), buf, sizeof( buf ) - 1 );

    if( len < 0 )
        return *this;

    wxString target = wxString::FromUTF8( buf, len );
    wxFileName result( target );

    if( result.IsRelative() )
        result.MakeAbsolute( GetPath() );

    return result;
}


wxString wxFileName::GetTempDir()
{
    for( const wchar_t* var : { L"TMPDIR", L"TMP", L"TEMP", L"TEMPDIR" } )
    {
        wxString dir;

        if( wxGetEnv( var, &dir ) && !dir.empty() )
            return stripTrailingSeparators( dir );
    }

    return wxT( "/tmp" );
}


namespace
{

/// mkstemp() on prefix + "XXXXXX"; @return the name and the open descriptor (-1 on failure).
wxString makeTemp( const wxString& aPrefix, int* aFd )
{
    wxString dir, name;
    wxFileName::SplitPath( aPrefix, &dir, &name, nullptr );

    if( dir.empty() )
        dir = wxFileName::GetTempDir();

    wxString    path = dir + wxT( "/" ) + name + wxT( "XXXXXX" );
    std::string buf = path.utf8_string();
    int         fd = mkstemp( buf.data() );

    *aFd = fd;
    return fd < 0 ? wxString() : wxString::FromUTF8( buf );
}

} // namespace


wxString wxFileName::CreateTempFileName( const wxString& aPrefix, wxFile* aFileTemp )
{
    int      fd;
    wxString name = makeTemp( aPrefix, &fd );

    if( name.empty() )
        return name;

    if( aFileTemp )
        aFileTemp->Attach( fd );
    else
        close( fd );

    return name;
}


wxString wxFileName::CreateTempFileName( const wxString& aPrefix, wxFFile* aFileTemp )
{
    int      fd;
    wxString name = makeTemp( aPrefix, &fd );

    if( name.empty() )
        return name;

    if( aFileTemp )
        aFileTemp->Attach( fdopen( fd, "w+b" ), name );
    else
        close( fd );

    return name;
}


wxString wxFindNextFile()
{
    FIND_STATE& state = findState();
    return state.next < state.matches.size() ? state.matches[state.next++] : wxString();
}
