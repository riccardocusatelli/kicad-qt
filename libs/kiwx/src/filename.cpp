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

#include <wx/crt.h>
#include <wx/file.h>
#include <wx/ffile.h>
#include <wx/filename.h>
#include <wx/mstream.h>
#include <wx/process.h>
#include <wx/txtstrm.h>
#include <wx/intl.h>
#include <wx/log.h>
#include <wx/tokenzr.h>
#include <wx/utils.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <thread>

#include <dirent.h>
#include <fcntl.h>
#include <fnmatch.h>
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


// Free functions on files

bool wxFileExists( const wxString& aFile )
{
    struct stat st;
    return !aFile.empty() && stat( fsPath( aFile ).c_str(), &st ) == 0 && S_ISREG( st.st_mode );
}


bool wxDirExists( const wxString& aDir )
{
    struct stat st;
    wxString    dir = stripTrailingSeparators( aDir );
    return !dir.empty() && stat( fsPath( dir ).c_str(), &st ) == 0 && S_ISDIR( st.st_mode );
}


bool wxIsAbsolutePath( const wxString& aPath )
{
    return !aPath.empty() && aPath[0] == wxT( '/' );
}


bool wxEndsWithPathSeparator( const wxString& aPath )
{
    return !aPath.empty() && wxFileName::IsPathSeparator( aPath.Last() );
}


wxString wxPathOnly( const wxString& aPath )
{
    if( aPath.empty() )
        return wxString();

    int pos = aPath.Find( wxT( '/' ), true );

    if( pos == wxNOT_FOUND )
        return wxString();

    if( pos == 0 )
        return wxT( "/" );

    return aPath.Left( pos );
}


wxString wxFileNameFromPath( const wxString& aPath )
{
    return wxFileName( aPath ).GetFullName();
}


bool wxMkdir( const wxString& aDir, int aPerm )
{
    if( mkdir( fsPath( aDir ).c_str(), aPerm ) == 0 )
        return true;

    wxLogSysError( _( "Directory '%s' couldn't be created" ), aDir );
    return false;
}


bool wxRmdir( const wxString& aDir, int )
{
    if( rmdir( fsPath( aDir ).c_str() ) == 0 )
        return true;

    wxLogSysError( _( "Directory '%s' couldn't be deleted" ), aDir );
    return false;
}


int wxChmod( const wxString& aPath, int aMode )
{
    return chmod( fsPath( aPath ).c_str(), aMode );
}


bool wxRemoveFile( const wxString& aFile )
{
    if( unlink( fsPath( aFile ).c_str() ) == 0 )
        return true;

    wxLogSysError( _( "File '%s' couldn't be removed" ), aFile );
    return false;
}


bool wxRenameFile( const wxString& aFrom, const wxString& aTo, bool aOverwrite )
{
    if( !aOverwrite && wxFileExists( aTo ) )
        return false;

    if( std::rename( fsPath( aFrom ).c_str(), fsPath( aTo ).c_str() ) == 0 )
        return true;

    // Across file systems: copy and remove.
    if( wxCopyFile( aFrom, aTo, aOverwrite ) && wxRemoveFile( aFrom ) )
        return true;

    wxLogSysError( _( "File '%s' couldn't be renamed '%s'" ), aFrom, aTo );
    return false;
}


bool wxCopyFile( const wxString& aFrom, const wxString& aTo, bool aOverwrite )
{
    if( !aOverwrite && wxFileExists( aTo ) )
    {
        wxLogSysError( _( "Impossible to overwrite the file '%s'" ), aTo );
        return false;
    }

    struct stat st;

    if( stat( fsPath( aFrom ).c_str(), &st ) != 0 )
        return false;

    int in = open( fsPath( aFrom ).c_str(), O_RDONLY );

    if( in < 0 )
        return false;

    unlink( fsPath( aTo ).c_str() );
    int out = open( fsPath( aTo ).c_str(), O_WRONLY | O_CREAT | O_TRUNC, st.st_mode & 0777 );

    if( out < 0 )
    {
        close( in );
        return false;
    }

    char    buf[65536];
    ssize_t n;
    bool    ok = true;

    while( ( n = read( in, buf, sizeof( buf ) ) ) > 0 )
    {
        if( write( out, buf, n ) != n )
        {
            ok = false;
            break;
        }
    }

    if( n < 0 )
        ok = false;

    close( in );

    if( close( out ) != 0 )
        ok = false;

    chmod( fsPath( aTo ).c_str(), st.st_mode & 07777 );
    return ok;
}


wxString wxGetCwd()
{
    char buf[PATH_MAX];
    return getcwd( buf, sizeof( buf ) ) ? wxString( buf ) : wxString();
}


bool wxSetWorkingDirectory( const wxString& aDir )
{
    return chdir( fsPath( aDir ).c_str() ) == 0;
}


time_t wxFileModificationTime( const wxString& aFile )
{
    struct stat st;
    return stat( fsPath( aFile ).c_str(), &st ) == 0 ? st.st_mtime : -1;
}


bool wxIsWild( const wxString& aPattern )
{
    return aPattern.find_first_of( wxT( "*?[" ) ) != wxString::npos;
}


bool wxMatchWild( const wxString& aPattern, const wxString& aText, bool aDotSpecial )
{
    if( aText.empty() )
        return aPattern.empty() || aPattern == wxT( "*" );

    if( aDotSpecial && aText[0] == wxT( '.' ) && ( aPattern.empty() || aPattern[0] != wxT( '.' ) ) )
        return false;

    return aText.Matches( aPattern );
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


wxString wxFindNextFile()
{
    FIND_STATE& state = findState();
    return state.next < state.matches.size() ? state.matches[state.next++] : wxString();
}


wxString wxRealPath( const wxString& aPath )
{
    char buf[PATH_MAX];
    return realpath( fsPath( aPath ).c_str(), buf ) ? wxString( buf ) : aPath;
}


bool wxGetDiskSpace( const wxString& aPath, wxLongLong* aTotal, wxLongLong* aFree )
{
    struct statvfs fs;

    if( statvfs( fsPath( aPath ).c_str(), &fs ) != 0 )
        return false;

    if( aTotal )
        *aTotal = static_cast<long long>( fs.f_blocks ) * fs.f_frsize;

    if( aFree )
        *aFree = static_cast<long long>( fs.f_bavail ) * fs.f_frsize;

    return true;
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


wxFileKind wxGetFileKind( FILE* aFile )
{
    return aFile ? wxGetFileKind( fileno( aFile ) ) : wxFILE_KIND_UNKNOWN;
}


long long kiwxFileSize( const wxString& aFile )
{
    struct stat st;
    return stat( fsPath( aFile ).c_str(), &st ) == 0 ? static_cast<long long>( st.st_size ) : -1;
}


// Environment and system

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


wxString wxExpandEnvVars( const wxString& aStr )
{
    // The rules of wxWidgets on Unix: $VAR, ${VAR}, $(VAR); unknown variables are kept as
    // they are; a backslash before $ suppresses the expansion.
    const std::wstring& str = aStr.impl();
    wxString            result;
    result.reserve( str.size() );

    for( size_t n = 0; n < str.size(); n++ )
    {
        wchar_t c = str[n];

        if( c == L'$' )
        {
            wchar_t bracket = 0;

            if( n + 1 < str.size() )
            {
                if( str[n + 1] == L'(' )
                {
                    bracket = L')';
                    n++;
                }
                else if( str[n + 1] == L'{' )
                {
                    bracket = L'}';
                    n++;
                }
            }

            size_t m = n + 1;

            while( m < str.size() && ( std::iswalnum( str[m] ) || str[m] == L'_' ) )
                m++;

            wxString varName( str.substr( n + 1, m - n - 1 ) );
            wxString value;
            bool     found = !varName.empty() && wxGetEnv( varName, &value );

            if( found )
            {
                result += value;
            }
            else
            {
                if( bracket )
                    result += str[n - 1];

                result += str[n];
                result += varName;
            }

            if( bracket )
            {
                if( m == str.size() || str[m] != bracket )
                {
                    wxLogWarning( _( "Environment variables expansion failed: missing '%c' at "
                                     "position %u in '%s'." ),
                                  (char) bracket, (unsigned int) ( m + 1 ), aStr );
                }
                else
                {
                    if( !found )
                        result += bracket;

                    m++;
                }
            }

            n = m - 1;
            continue;
        }

        if( c == L'\\' && n + 1 < str.size() && ( str[n + 1] == L'%' || str[n + 1] == L'$' ) )
        {
            result += str[++n];
            continue;
        }

        result += c;
    }

    return result;
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


bool wxGetUserId( wxChar* aBuf, int aSize )
{
    wxString id = wxGetUserId();

    if( id.empty() || aSize <= 0 )
        return false;

    std::wcsncpy( aBuf, id.wc_str(), aSize - 1 );
    aBuf[aSize - 1] = 0;
    return true;
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


wxString wxGetNativeCpuArchitectureName()
{
    return wxGetCpuArchitectureName();
}


bool wxIsPlatform64Bit()
{
    return sizeof( void* ) == 8;
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


void wxMilliSleep( unsigned long aMs )
{
    std::this_thread::sleep_for( std::chrono::milliseconds( aMs ) );
}


void wxMicroSleep( unsigned long aUs )
{
    std::this_thread::sleep_for( std::chrono::microseconds( aUs ) );
}


void wxSleep( int aSeconds )
{
    std::this_thread::sleep_for( std::chrono::seconds( aSeconds ) );
}


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


// The lines of a captured output, as wxWidgets reads them (wxTextInputStream::ReadLine()).
void toLines( const std::string& aData, wxArrayString& aLines )
{
    wxMemoryInputStream in( aData.data(), aData.size() );
    wxTextInputStream   text( in );

    while( in.CanRead() && !in.Eof() )
    {
        wxString line = text.ReadLine();

        if( in.Eof() && line.empty() )
            break;

        aLines.Add( line );
    }
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


long wxExecute( const wxString& aCommand, int aFlags, wxProcess* aProcess, const wxExecuteEnv* aEnv )
{
    return runForProcess( splitCommand( aCommand ), aFlags, aProcess, aEnv );
}


long wxExecute( const wxString& aCommand, wxArrayString& aOutput, int aFlags, const wxExecuteEnv* aEnv )
{
    std::string out;
    long        ret = run( splitCommand( aCommand ), aFlags | wxEXEC_SYNC, aEnv, &out, nullptr );
    toLines( out, aOutput );
    return ret;
}


long wxExecute( const wxString& aCommand, wxArrayString& aOutput, wxArrayString& aErrors, int aFlags,
                const wxExecuteEnv* aEnv )
{
    std::string out, err;
    long        ret = run( splitCommand( aCommand ), aFlags | wxEXEC_SYNC, aEnv, &out, &err );
    toLines( out, aOutput );
    toLines( err, aErrors );
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


// wxPathList

bool wxPathList::Add( const wxString& aPath )
{
    // as a directory, normalized without wxPATH_NORM_DOTS (paths starting with "..")
    wxFileName fn( aPath + wxFileName::GetPathSeparator() );

    if( !fn.Normalize( wxPATH_NORM_TILDE | wxPATH_NORM_LONG | wxPATH_NORM_ENV_VARS ) )
        return false;

    wxString toadd = fn.GetPath();

    if( Index( toadd ) == wxNOT_FOUND )
        wxArrayString::Add( toadd );

    return true;
}


void wxPathList::Add( const wxArrayString& aArr )
{
    for( size_t j = 0; j < aArr.GetCount(); j++ )
        Add( aArr[j] );
}


void wxPathList::AddEnvList( const wxString& aEnvVariable )
{
    wxString val;

    if( wxGetEnv( aEnvVariable, &val ) )
    {
        wxArrayString arr = wxStringTokenize( val, wxT( ":;" ) );

        for( const wxString& item : arr )
            wxArrayString::Add( item );
    }
}


bool wxPathList::EnsureFileAccessible( const wxString& aPath )
{
    return Add( wxPathOnly( aPath ) );
}


wxString wxPathList::FindValidPath( const wxString& aFile ) const
{
    wxFileName fn( aFile );
    wxString   strend;

    if( !fn.Normalize( wxPATH_NORM_TILDE | wxPATH_NORM_LONG | wxPATH_NORM_ENV_VARS ) )
        return wxEmptyString;

    wxASSERT_MSG( !fn.IsDir(), wxT( "Cannot search for directories; only for files" ) );

    // an absolute path: only its name is searched
    if( fn.IsAbsolute() )
        strend = fn.GetFullName();
    else
        strend = fn.GetFullPath();

    for( size_t i = 0; i < GetCount(); i++ )
    {
        wxString strstart = Item( i );

        if( !strstart.IsEmpty() && strstart.Last() != wxFileName::GetPathSeparator() )
            strstart += wxFileName::GetPathSeparator();

        if( wxFileExists( strstart + strend ) )
            return strstart + strend;
    }

    return wxEmptyString;
}


wxString wxPathList::FindAbsoluteValidPath( const wxString& aFile ) const
{
    wxString f = FindValidPath( aFile );

    if( f.empty() || wxIsAbsolutePath( f ) )
        return f;

    wxString buf = ::wxGetCwd();

    if( !wxEndsWithPathSeparator( buf ) )
        buf += wxFILE_SEP_PATH;

    return buf + f;
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


wxKillError wxProcess::Kill( int aPid, wxSignal aSig, int aFlags )
{
    wxKillError rc;
    wxKill( aPid, aSig, &rc, aFlags );
    return rc;
}


bool wxProcess::Exists( int aPid )
{
    switch( Kill( aPid, wxSIGNONE ) )
    {
    case wxKILL_OK:
    case wxKILL_ACCESS_DENIED: return true;
    default:                   return false;
    }
}


bool wxShell( const wxString& aCommand )
{
    std::string cmd = aCommand.empty() ? std::string( "/bin/sh" ) : aCommand.utf8_string();
    return std::system( cmd.c_str() ) == 0;
}


wxString wxNow()
{
    time_t now = std::time( nullptr );
    char   buf[64];
    ctime_r( &now, buf );
    wxString s( buf );
    return s.Trim();
}


// wxFileName

void wxFileName::Clear()
{
    m_dirs.clear();
    m_volume.clear();
    m_name.clear();
    m_ext.clear();
    m_relative = true;
    m_hasExt = false;
    m_dontFollowLinks = false;
}


wxUniChar wxFileName::GetPathSeparator( wxPathFormat aFormat )
{
    return aFormat == wxPATH_DOS ? wxT( '\\' ) : wxT( '/' );
}


wxString wxFileName::GetPathSeparators( wxPathFormat aFormat )
{
    return aFormat == wxPATH_DOS ? wxT( "\\/" ) : wxT( "/" );
}


bool wxFileName::IsPathSeparator( wxUniChar aCh, wxPathFormat aFormat )
{
    return aCh != 0 && GetPathSeparators( aFormat ).Find( aCh ) != wxNOT_FOUND;
}


wxString wxFileName::GetForbiddenChars( wxPathFormat aFormat )
{
    wxString chars = wxT( "*?" );

    if( aFormat == wxPATH_DOS )
        chars += wxT( "\\/:\"<>|" );

    return chars;
}


void wxFileName::SplitVolume( const wxString& aFullPath, wxString* aVolume, wxString* aPath,
                              wxPathFormat )
{
    // No volumes on Unix.
    if( aVolume )
        aVolume->clear();

    if( aPath )
        *aPath = aFullPath;
}


void wxFileName::SplitPath( const wxString& aFullPathWithVolume, wxString* aVolume, wxString* aPath,
                            wxString* aName, wxString* aExt, bool* aHasExt, wxPathFormat aFormat )
{
    wxString fullPath;
    SplitVolume( aFullPathWithVolume, aVolume, &fullPath, aFormat );

    size_t posLastSlash = fullPath.find_last_of( GetPathSeparators( aFormat ) );
    size_t posLastDot = fullPath.find_last_of( wxFILE_SEP_EXT );

    // A dot at the start of the name is not an extension (hidden files).
    if( posLastDot != wxString::npos
        && ( posLastDot == 0 || IsPathSeparator( fullPath[posLastDot - 1], aFormat ) ) )
    {
        posLastDot = wxString::npos;
    }

    // The dot must be in the name, not in the path.
    if( posLastDot != wxString::npos && posLastSlash != wxString::npos && posLastDot < posLastSlash )
        posLastDot = wxString::npos;

    if( aPath )
    {
        if( posLastSlash == wxString::npos )
        {
            aPath->clear();
        }
        else
        {
            // The files of the root directory have the path "/", not "".
            size_t len = posLastSlash;

            if( !len )
                len++;

            *aPath = fullPath.Left( len );
        }
    }

    if( aName )
    {
        size_t nStart = posLastSlash == wxString::npos ? 0 : posLastSlash + 1;
        size_t count = posLastDot == wxString::npos ? wxString::npos : posLastDot - nStart;

        *aName = fullPath.Mid( nStart, count );
    }

    if( posLastDot == wxString::npos )
    {
        if( aExt )
            aExt->clear();

        if( aHasExt )
            *aHasExt = false;
    }
    else
    {
        if( aExt )
            *aExt = fullPath.Mid( posLastDot + 1 );

        if( aHasExt )
            *aHasExt = true;
    }
}


void wxFileName::SplitPath( const wxString& aFullPath, wxString* aPath, wxString* aName,
                            wxString* aExt, wxPathFormat aFormat )
{
    wxString volume;
    SplitPath( aFullPath, &volume, aPath, aName, aExt, nullptr, aFormat );

    if( aPath && !volume.empty() )
        aPath->Prepend( volume );
}


wxString wxFileName::StripExtension( const wxString& aFullPath )
{
    wxFileName fn( aFullPath );
    fn.ClearExt();
    return fn.GetFullPath();
}


void wxFileName::Assign( const wxString& aFullPath, wxPathFormat aFormat )
{
    wxString volume, path, name, ext;
    bool     hasExt;
    SplitPath( aFullPath, &volume, &path, &name, &ext, &hasExt, aFormat );
    Assign( volume, path, name, ext, hasExt, aFormat );
}


void wxFileName::Assign( const wxString& aVolume, const wxString& aPath, const wxString& aName,
                         const wxString& aExt, bool aHasExt, wxPathFormat aFormat )
{
    SetPath( aPath, aFormat );

    m_volume = aVolume;
    m_ext = aExt;
    m_name = aName;
    m_hasExt = aHasExt;
}


void wxFileName::Assign( const wxString& aPathOrig, const wxString& aFullName, wxPathFormat aFormat )
{
    // The path is a directory even without a trailing separator.
    wxString fullPath = aPathOrig;

    if( !fullPath.empty() && !wxEndsWithPathSeparator( fullPath ) )
        fullPath += GetPathSeparator( aFormat );

    wxString volume, path, name, ext;
    wxString volDummy, pathDummy, nameDummy, extDummy;
    bool     hasExt;

    SplitPath( aFullName, &volDummy, &pathDummy, &name, &ext, &hasExt, aFormat );
    SplitPath( fullPath, &volume, &path, &nameDummy, &extDummy, nullptr, aFormat );

    Assign( volume, path, name, ext, hasExt, aFormat );
}


void wxFileName::Assign( const wxString& aPathOrig, const wxString& aName, const wxString& aExt,
                         wxPathFormat aFormat )
{
    wxString volume, path;
    SplitVolume( aPathOrig, &volume, &path, aFormat );
    Assign( volume, path, aName, aExt, aFormat );
}


void wxFileName::AssignCwd( const wxString& aVolume )
{
    AssignDir( GetCwd( aVolume ) );
}


void wxFileName::AssignHomeDir()
{
    AssignDir( GetHomeDir() );
}


void wxFileName::AssignTempFileName( const wxString& aPrefix )
{
    wxString tempName = CreateTempFileName( aPrefix );

    if( tempName.empty() )
        Clear();
    else
        Assign( tempName );
}


void wxFileName::AssignTempFileName( const wxString& aPrefix, wxFile* aFileTemp )
{
    wxString tempName = CreateTempFileName( aPrefix, aFileTemp );

    if( tempName.empty() )
        Clear();
    else
        Assign( tempName );
}


void wxFileName::AssignTempFileName( const wxString& aPrefix, wxFFile* aFileTemp )
{
    wxString tempName = CreateTempFileName( aPrefix, aFileTemp );

    if( tempName.empty() )
        Clear();
    else
        Assign( tempName );
}


void wxFileName::SetPath( const wxString& aPathOrig, wxPathFormat aFormat )
{
    m_dirs.clear();

    if( aPathOrig.empty() )
    {
        m_relative = true;
        return;
    }

    wxString volume, path;
    SplitVolume( aPathOrig, &volume, &path, aFormat );

    if( !volume.empty() )
    {
        m_relative = false;
        SetVolume( volume );
    }

    if( path.empty() )
        return;

    m_relative = !IsPathSeparator( path[0], aFormat );

    // Empty components (a//b) are ignored; "/" alone gives no components and m_relative false.
    wxStringTokenizer tn( path, GetPathSeparators( aFormat ) );

    while( tn.HasMoreTokens() )
    {
        wxString token = tn.GetNextToken();

        if( !token.empty() )
            m_dirs.Add( token );
    }
}


wxString wxFileName::GetPath( int aFlags, wxPathFormat aFormat ) const
{
    wxString fullPath;

    if( aFlags & wxPATH_GET_VOLUME )
        fullPath += m_volume;

    if( !m_relative )
        fullPath += GetPathSeparator( aFormat );

    const size_t dirCount = m_dirs.size();

    for( size_t i = 0; i < dirCount; i++ )
    {
        fullPath += m_dirs[i];

        if( ( aFlags & wxPATH_GET_SEPARATOR ) || i != dirCount - 1 )
            fullPath += GetPathSeparator( aFormat );
    }

    return fullPath;
}


wxString wxFileName::GetFullPath( wxPathFormat aFormat ) const
{
    return GetPath( wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR, aFormat ) + GetFullName();
}


void wxFileName::SetFullName( const wxString& aFullName )
{
    SplitPath( aFullName, nullptr, nullptr, &m_name, &m_ext, &m_hasExt );
}


wxString wxFileName::GetFullName() const
{
    wxString fullName = m_name;

    if( m_hasExt )
        fullName << wxFILE_SEP_EXT << m_ext;

    return fullName;
}


bool wxFileName::IsValidDirComponent( const wxString& aDir )
{
    if( aDir.empty() )
        return false;

    for( wxUniChar c : aDir )
    {
        if( IsPathSeparator( c ) )
            return false;
    }

    return true;
}


bool wxFileName::AppendDir( const wxString& aDir )
{
    if( !IsValidDirComponent( aDir ) )
        return false;

    m_dirs.Add( aDir );
    return true;
}


bool wxFileName::InsertDir( size_t aBefore, const wxString& aDir )
{
    if( !IsValidDirComponent( aDir ) )
        return false;

    m_dirs.Insert( aDir, aBefore );
    return true;
}


bool wxFileName::Normalize( int aFlags, const wxString& aCwd, wxPathFormat aFormat )
{
    // Environment variables first: they may change the whole path.
    if( aFlags & wxPATH_NORM_ENV_VARS )
    {
        wxString pathOrig = GetFullPath( aFormat );
        wxString path = wxExpandEnvVars( pathOrig );

        if( path != pathOrig )
            Assign( path );
    }

    wxArrayString dirs = GetDirs();
    wxFileName    curDir;

    if( ( aFlags & wxPATH_NORM_ABSOLUTE ) && !IsAbsolute( aFormat ) )
    {
        if( aCwd.empty() )
            curDir.AssignCwd( GetVolume() );
        else
            curDir.AssignDir( aCwd );
    }

    if( ( aFlags & wxPATH_NORM_TILDE ) && m_relative && !dirs.empty() )
    {
        wxString dir = dirs[0];

        if( !dir.empty() && dir[0] == wxT( '~' ) )
        {
            curDir.AssignDir( wxGetUserHome( dir.Mid( 1 ) ) );
            dirs.RemoveAt( 0 );
        }
    }

    if( curDir.IsOk() )
    {
        if( !HasVolume() && curDir.HasVolume() )
        {
            SetVolume( curDir.GetVolume() );

            if( !m_relative )
                curDir.Clear();
        }

        wxArrayString dirsNew = curDir.GetDirs();
        dirs.insert( dirs.begin(), dirsNew.begin(), dirsNew.end() );

        if( !curDir.m_relative )
            m_relative = false;
    }

    m_dirs.clear();

    for( const wxString& dir : dirs )
    {
        if( aFlags & wxPATH_NORM_DOTS )
        {
            if( dir == wxT( "." ) )
                continue;

            if( dir == wxT( ".." ) )
            {
                if( m_dirs.empty() )
                {
                    // More ".." than components: dropped for absolute paths ("/.." is "/"),
                    // kept for relative ones.
                    if( !m_relative )
                        continue;
                }
                else if( m_dirs.Last() != wxT( ".." ) )
                {
                    m_dirs.pop_back();
                    continue;
                }
            }
        }

        m_dirs.Add( dir );
    }

    return true;
}


bool wxFileName::MakeRelativeTo( const wxString& aPathBase, wxPathFormat aFormat )
{
    wxFileName fnBase = wxFileName::DirName( aPathBase, aFormat );
    wxString   cwd = wxGetCwd();

    const int normFlags = wxPATH_NORM_ALL & ~( wxPATH_NORM_CASE | wxPATH_NORM_SHORTCUT );
    Normalize( normFlags, cwd, aFormat );
    fnBase.Normalize( normFlags, cwd, aFormat );

    if( !GetVolume().IsSameAs( fnBase.GetVolume() ) )
        return false;

    m_volume.clear();

    while( !m_dirs.empty() && !fnBase.m_dirs.empty() && m_dirs[0] == fnBase.m_dirs[0] )
    {
        m_dirs.RemoveAt( 0 );
        fnBase.m_dirs.RemoveAt( 0 );
    }

    for( size_t i = 0; i < fnBase.m_dirs.size(); i++ )
        m_dirs.Insert( wxT( ".." ), 0 );

    // A directory relative to itself is ".".
    if( m_dirs.empty() && IsDir() )
        m_dirs.Add( wxT( "." ) );

    m_relative = true;
    return true;
}


bool wxFileName::ReplaceEnvVariable( const wxString& aEnvName, const wxString& aReplacementFmt,
                                     wxPathFormat aFormat )
{
    wxString value;

    if( !wxGetEnv( aEnvName, &value ) || value.empty() )
        return false;

    wxString replacement = wxString::Format( aReplacementFmt, aEnvName );
    wxString path = GetPath( wxPATH_GET_VOLUME, aFormat );

    if( !path.Replace( value, replacement, false ) )
        return false;

    SetPath( path, aFormat );
    return true;
}


bool wxFileName::ReplaceHomeDir( wxPathFormat aFormat )
{
    wxString home = wxGetHomeDir();
    wxString path = GetPath( wxPATH_GET_VOLUME, aFormat );

    if( home.empty() || !path.StartsWith( home ) )
        return false;

    path = wxT( "~" ) + path.Mid( home.length() );
    SetPath( path, aFormat );
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


bool wxFileName::SameAs( const wxFileName& aOther, wxPathFormat aFormat ) const
{
    wxFileName fn1 = *this;
    wxFileName fn2 = aOther;

    wxString cwd = wxGetCwd();
    const int flags = wxPATH_NORM_ALL & ~( wxPATH_NORM_CASE | wxPATH_NORM_SHORTCUT );
    fn1.Normalize( flags, cwd, aFormat );
    fn2.Normalize( flags, cwd, aFormat );

    return fn1.GetFullPath( aFormat ) == fn2.GetFullPath( aFormat );
}


bool wxFileName::FileExists( const wxString& aFile )
{
    return wxFileExists( aFile );
}


bool wxFileName::DirExists( const wxString& aDir )
{
    return wxDirExists( aDir );
}


bool wxFileName::Exists( const wxString& aPath, int aFlags )
{
    struct stat st;
    std::string path = fsPath( aPath );
    bool        follow = !( aFlags & wxFILE_EXISTS_NO_FOLLOW );

    if( ( follow ? stat( path.c_str(), &st ) : lstat( path.c_str(), &st ) ) != 0 )
        return false;

    if( ( aFlags & wxFILE_EXISTS_REGULAR ) && S_ISREG( st.st_mode ) )
        return true;

    if( ( aFlags & wxFILE_EXISTS_DIR ) && S_ISDIR( st.st_mode ) )
        return true;

    if( ( aFlags & wxFILE_EXISTS_SYMLINK & ~wxFILE_EXISTS_NO_FOLLOW ) && S_ISLNK( st.st_mode ) )
        return true;

    if( ( aFlags & wxFILE_EXISTS_DEVICE ) && ( S_ISBLK( st.st_mode ) || S_ISCHR( st.st_mode ) ) )
        return true;

    if( ( aFlags & wxFILE_EXISTS_FIFO ) && S_ISFIFO( st.st_mode ) )
        return true;

    if( ( aFlags & wxFILE_EXISTS_SOCKET ) && S_ISSOCK( st.st_mode ) )
        return true;

    return aFlags == wxFILE_EXISTS_ANY;
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


wxDateTime wxFileName::GetModificationTime() const
{
    wxDateTime mod;
    GetTimes( nullptr, &mod, nullptr );
    return mod;
}


wxDateTime wxFileName::GetAccessTime() const
{
    wxDateTime access;
    GetTimes( &access, nullptr, nullptr );
    return access;
}


wxString wxFileName::GetCwd( const wxString& )
{
    return wxGetCwd();
}


bool wxFileName::SetCwd( const wxString& aCwd )
{
    return wxSetWorkingDirectory( aCwd );
}


wxString wxFileName::GetHomeDir()
{
    return wxGetHomeDir();
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


bool wxFileName::Mkdir( const wxString& aDir, int aPerm, int aFlags )
{
    if( aFlags & wxPATH_MKDIR_FULL )
    {
        wxFileName fn;
        fn.AssignDir( aDir );

        wxString currPath = fn.GetVolume();
        const wxArrayString& dirs = fn.GetDirs();

        for( size_t i = 0; i < dirs.size(); i++ )
        {
            if( i > 0 || fn.IsAbsolute() )
                currPath += wxFILE_SEP_PATH;

            currPath += dirs[i];

            if( !wxDirExists( currPath ) && !wxMkdir( currPath, aPerm ) )
                return false;
        }

        return true;
    }

    return wxMkdir( aDir, aPerm );
}


bool wxFileName::Rmdir( const wxString& aDir, int aFlags )
{
    std::error_code ec;
    std::filesystem::path path( fsPath( aDir ) );

    if( aFlags & wxPATH_RMDIR_RECURSIVE )
        return std::filesystem::remove_all( path, ec ) > 0 && !ec;

    if( aFlags & wxPATH_RMDIR_FULL )
    {
        // Only empty subdirectories.
        for( const auto& entry : std::filesystem::recursive_directory_iterator( path, ec ) )
        {
            if( !entry.is_directory() )
                return false;
        }

        return std::filesystem::remove_all( path, ec ) > 0 && !ec;
    }

    return wxRmdir( aDir );
}


wxULongLong wxFileName::GetSize( const wxString& aFile )
{
    long long size = kiwxFileSize( aFile );
    return size < 0 ? wxInvalidSize : wxULongLong( static_cast<unsigned long long>( size ) );
}


wxString wxFileName::GetHumanReadableSize( const wxULongLong& aSize, const wxString& aNullSize,
                                           int aPrecision, wxSizeConvention aConv )
{
    if( aSize.GetValue() == 0 || aSize.GetValue() == wxInvalidSize.GetValue() )
        return aNullSize;

    double   multiplier = 1024.0;
    wxString infix;

    if( aConv == wxSIZE_CONV_IEC )
        infix = wxT( "i" );
    else if( aConv == wxSIZE_CONV_SI )
        multiplier = 1000.0;

    double         size = aSize.ToDouble();
    const wchar_t* units[] = { L"K", L"M", L"G", L"T", L"P", L"E", L"Z", L"Y" };

    if( size < multiplier )
        return wxString::Format( wxT( "%s B" ), aSize.ToString() );

    double scale = multiplier;
    int    unit = 0;

    while( unit < 7 && size >= scale * multiplier )
    {
        scale *= multiplier;
        unit++;
    }

    return wxString::Format( wxT( "%.*f %s%sB" ), aPrecision, size / scale, units[unit], infix );
}
