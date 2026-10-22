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
 * @file utils_msw.cpp
 *
 * The services of the operating system on Windows, as in wxMSW: files and directories,
 * environment, users, processes.  The Unix versions are in src/unix/utils_unix.cpp.
 */

#include <wx/crt.h>
#include <wx/datetime.h>
#include <wx/dir.h>
#include <wx/ffile.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/intl.h>
#include <wx/log.h>
#include <wx/process.h>
#include <wx/utils.h>

#include "../kiwx_compat.h"
#include "../kiwx_private.h"

#include <cerrno>
#include <cstdio>
#include <thread>

#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#include <windows.h>
#include <shellapi.h>


namespace
{

/// Milliseconds between 1601-01-01 (FILETIME) and 1970-01-01.
const long long EPOCH_OFFSET_IN_MSEC = 11644473600000LL;


wxDateTime fromFileTime( const FILETIME& aTime )
{
    ULARGE_INTEGER t;
    t.LowPart = aTime.dwLowDateTime;
    t.HighPart = aTime.dwHighDateTime;
    return wxDateTime::FromMilliseconds( static_cast<long long>( t.QuadPart / 10000 )
                                         - EPOCH_OFFSET_IN_MSEC );
}


FILETIME toFileTime( const wxDateTime& aTime )
{
    ULARGE_INTEGER t;
    t.QuadPart = static_cast<ULONGLONG>( aTime.GetValue().GetValue() + EPOCH_OFFSET_IN_MSEC )
                 * 10000;

    FILETIME ft;
    ft.dwLowDateTime = t.LowPart;
    ft.dwHighDateTime = t.HighPart;
    return ft;
}

} // namespace


// Free functions on files

int wxChmod( const wxString& aPath, int aMode )
{
    // only the write permission exists on Windows
    return _wchmod( aPath.wc_str(), ( aMode & 0222 ) ? _S_IREAD | _S_IWRITE : _S_IREAD );
}


bool wxIsWritable( const wxString& aPath )
{
    return _waccess( aPath.wc_str(), 2 ) == 0;
}


bool wxIsReadable( const wxString& aPath )
{
    return _waccess( aPath.wc_str(), 4 ) == 0;
}


bool wxIsExecutable( const wxString& aPath )
{
    // no execute permission on Windows: the files that can be run
    if( !wxFileExists( aPath ) )
        return false;

    wxString ext = wxFileName( aPath ).GetExt().Lower();
    return ext == wxT( "exe" ) || ext == wxT( "com" ) || ext == wxT( "bat" ) || ext == wxT( "cmd" );
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

    wxString         dir = wxPathOnly( aSpec );
    WIN32_FIND_DATAW found;
    HANDLE           find = FindFirstFileW( aSpec.wc_str(), &found );

    if( find != INVALID_HANDLE_VALUE )
    {
        do
        {
            wxString name( found.cFileName );

            if( name == wxT( "." ) || name == wxT( ".." ) )
                continue;

            bool isDir = ( found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) != 0;

            if( aFlags == 0 || ( isDir && ( aFlags & wxDIR ) ) || ( !isDir && ( aFlags & wxFILE ) ) )
                state.matches.Add( dir.empty() ? name : dir + wxT( '\\' ) + name );
        } while( FindNextFileW( find, &found ) );

        FindClose( find );
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
    // the "." and ".." resolved, with the Windows separators
    wxFileName fn( aPath );
    fn.Normalize( wxPATH_NORM_DOTS );
    return fn.GetFullPath();
}


wxFileKind wxGetFileKind( int aFd )
{
    HANDLE h = reinterpret_cast<HANDLE>( _get_osfhandle( aFd ) );

    if( h == INVALID_HANDLE_VALUE )
        return wxFILE_KIND_UNKNOWN;

    switch( GetFileType( h ) )
    {
    case FILE_TYPE_CHAR: return wxFILE_KIND_TERMINAL;
    case FILE_TYPE_DISK: return wxFILE_KIND_DISK;
    case FILE_TYPE_PIPE: return wxFILE_KIND_PIPE;
    default:             return wxFILE_KIND_UNKNOWN;
    }
}


// Environment and system

bool wxGetEnv( const wxString& aVar, wxString* aValue )
{
    DWORD len = GetEnvironmentVariableW( aVar.wc_str(), nullptr, 0 );

    if( len == 0 )
        return false;

    if( aValue )
    {
        std::wstring buf( len, L'\0' );
        len = GetEnvironmentVariableW( aVar.wc_str(), buf.data(), len );
        buf.resize( len );
        *aValue = wxString( buf );
    }

    return true;
}


namespace
{

/// Set (or unset, with @a aValue null) a variable: through the CRT, which updates the
/// environment of the process too, so that getenv() sees it (as wxMSW).
bool setEnv( const wxString& aVar, const wchar_t* aValue )
{
    wxString entry = aVar + wxT( "=" );

    if( aValue )
        entry += aValue;

    return _wputenv( entry.wc_str() ) == 0;
}

} // namespace


bool wxSetEnv( const wxString& aVar, const wxString& aValue )
{
    return setEnv( aVar, aValue.wc_str() );
}


bool wxUnsetEnv( const wxString& aVar )
{
    return setEnv( aVar, nullptr );
}


bool wxGetEnvMap( wxEnvVariableHashMap* aMap )
{
    if( !aMap )
        return false;

    aMap->clear();

    wchar_t* block = GetEnvironmentStringsW();

    if( !block )
        return false;

    for( const wchar_t* e = block; *e; e += wcslen( e ) + 1 )
    {
        // the "=C:=C:\dir" entries are the current directories of the drives
        if( *e == L'=' )
            continue;

        wxString entry( e );
        aMap->emplace( entry.BeforeFirst( '=' ), entry.AfterFirst( '=' ) );
    }

    FreeEnvironmentStringsW( block );
    return true;
}


wxString wxGetHomeDir()
{
    // As wxMSW: HOME, else HOMEDRIVE and HOMEPATH (unless just "\"), else USERPROFILE, else
    // the directory of the program
    wxString home;

    if( !wxGetEnv( wxT( "HOME" ), &home ) )
    {
        wxString drive, path;
        wxGetEnv( wxT( "HOMEDRIVE" ), &drive );

        if( wxGetEnv( wxT( "HOMEPATH" ), &path ) && path != wxT( "\\" ) )
            home = drive + path;
    }

    if( home.empty() )
        wxGetEnv( wxT( "USERPROFILE" ), &home );

    if( !home.empty() )
        return wxExpandEnvVars( home );

    wchar_t exe[MAX_PATH];
    DWORD   len = GetModuleFileNameW( nullptr, exe, MAX_PATH );
    return wxPathOnly( wxString( exe, len ) );
}


wxString wxGetUserHome( const wxString& aUser )
{
    if( aUser.empty() || aUser == wxGetUserId() )
        return wxGetHomeDir();

    return wxString();
}


wxString wxGetUserId()
{
    wchar_t buf[256];
    DWORD   len = 256;

    if( GetUserNameW( buf, &len ) )
        return wxString( buf );

    wxString name;
    wxGetEnv( wxT( "USERNAME" ), &name );
    return name;
}


wxString wxGetUserName()
{
    // wxMSW has no full user name either (without the network API): the login name
    wxString id = wxGetUserId();
    return id.empty() ? wxString( wxT( "Unknown User" ) ) : id;
}


wxString wxGetHostName()
{
    wchar_t buf[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD   len = MAX_COMPUTERNAME_LENGTH + 1;
    return GetComputerNameW( buf, &len ) ? wxString( buf, len ) : wxString();
}


wxString wxGetFullHostName()
{
    wchar_t buf[256];
    DWORD   len = 256;

    if( GetComputerNameExW( ComputerNameDnsFullyQualified, buf, &len ) )
        return wxString( buf, len );

    return wxGetHostName();
}


unsigned long wxGetProcessId()
{
    return GetCurrentProcessId();
}


namespace
{

/// The version of Windows, from RtlGetVersion() (GetVersionEx() lies to programs without a
/// manifest).
OSVERSIONINFOEXW windowsVersion()
{
    OSVERSIONINFOEXW info = {};
    info.dwOSVersionInfoSize = sizeof( info );

    typedef LONG( WINAPI * RtlGetVersion_t )( OSVERSIONINFOEXW* );

    if( HMODULE ntdll = GetModuleHandleW( L"ntdll.dll" ) )
    {
        auto rtlGetVersion = reinterpret_cast<RtlGetVersion_t>(
                reinterpret_cast<void*>( GetProcAddress( ntdll, "RtlGetVersion" ) ) );

        if( rtlGetVersion )
            rtlGetVersion( &info );
    }

    return info;
}

} // namespace


wxString wxGetOsDescription()
{
    OSVERSIONINFOEXW info = windowsVersion();
    wxString         name;

    if( info.dwMajorVersion == 10 && info.dwBuildNumber >= 22000 )
        name = info.wProductType == VER_NT_WORKSTATION ? wxT( "Windows 11" ) : wxT( "Windows Server" );
    else if( info.dwMajorVersion == 10 )
        name = info.wProductType == VER_NT_WORKSTATION ? wxT( "Windows 10" ) : wxT( "Windows Server" );
    else
        name = wxString::Format( wxT( "Windows %lu.%lu" ), info.dwMajorVersion, info.dwMinorVersion );

    name += wxString::Format( wxT( " (build %lu" ), info.dwBuildNumber );

    if( info.szCSDVersion[0] )
        name += wxT( ", " ) + wxString( info.szCSDVersion );

    name += wxT( ")" );

    if( sizeof( void* ) == 8 )
        name += wxT( ", 64-bit edition" );

    return name;
}


int wxGetOsVersion( int* aMajor, int* aMinor, int* aMicro )
{
    OSVERSIONINFOEXW info = windowsVersion();

    if( aMajor )
        *aMajor = static_cast<int>( info.dwMajorVersion );

    if( aMinor )
        *aMinor = static_cast<int>( info.dwMinorVersion );

    if( aMicro )
        *aMicro = static_cast<int>( info.dwBuildNumber );

    return 0;
}


wxString wxGetCpuArchitectureName()
{
    SYSTEM_INFO si;
    GetNativeSystemInfo( &si );

    switch( si.wProcessorArchitecture )
    {
    case PROCESSOR_ARCHITECTURE_AMD64: return wxT( "x64" );
    case PROCESSOR_ARCHITECTURE_ARM:   return wxT( "ARM" );
    case PROCESSOR_ARCHITECTURE_ARM64: return wxT( "ARM64" );
    case PROCESSOR_ARCHITECTURE_IA64:  return wxT( "Itanium" );
    case PROCESSOR_ARCHITECTURE_INTEL: return wxT( "x86" );
    default:                           return wxString();
    }
}


wxLinuxDistributionInfo wxGetLinuxDistributionInfo()
{
    return wxLinuxDistributionInfo();
}


// Processes

namespace
{

/// An argument quoted for the command line of CreateProcess(), as the C runtime parses it.
std::wstring quoteArg( const std::wstring& aArg )
{
    if( !aArg.empty() && aArg.find_first_of( L" \t\n\v\"" ) == std::wstring::npos )
        return aArg;

    std::wstring quoted = L"\"";

    for( size_t i = 0;; ++i )
    {
        size_t backslashes = 0;

        while( i < aArg.size() && aArg[i] == L'\\' )
        {
            ++i;
            ++backslashes;
        }

        if( i == aArg.size() )
        {
            // double the backslashes before the closing quote
            quoted.append( backslashes * 2, L'\\' );
            break;
        }

        if( aArg[i] == L'"' )
        {
            quoted.append( backslashes * 2 + 1, L'\\' );
            quoted += L'"';
        }
        else
        {
            quoted.append( backslashes, L'\\' );
            quoted += aArg[i];
        }
    }

    quoted += L'"';
    return quoted;
}


/// Reads a pipe until its end.
void readAll( HANDLE aPipe, std::string* aData )
{
    char  buf[4096];
    DWORD n;

    while( ReadFile( aPipe, buf, sizeof( buf ), &n, nullptr ) && n > 0 )
        aData->append( buf, n );

    CloseHandle( aPipe );
}


/// Runs a command line; with aOutput / aErrors, its standard output / error are captured.
long run( const wxString& aCommandLine, int aFlags, const wxExecuteEnv* aEnv, std::string* aOutput,
          std::string* aErrors )
{
    if( aCommandLine.empty() )
        return aFlags & wxEXEC_SYNC ? -1 : 0;

    SECURITY_ATTRIBUTES sa = { sizeof( sa ), nullptr, TRUE };
    HANDLE              outRead = nullptr, outWrite = nullptr;
    HANDLE              errRead = nullptr, errWrite = nullptr;

    auto makePipe =
            [&]( HANDLE& aRead, HANDLE& aWrite )
            {
                if( !CreatePipe( &aRead, &aWrite, &sa, 0 ) )
                    return false;

                // only the write end is inherited by the child
                SetHandleInformation( aRead, HANDLE_FLAG_INHERIT, 0 );
                return true;
            };

    if( ( aOutput && !makePipe( outRead, outWrite ) ) || ( aErrors && !makePipe( errRead, errWrite ) ) )
        return aFlags & wxEXEC_SYNC ? -1 : 0;

    STARTUPINFOW si = {};
    si.cb = sizeof( si );

    if( aOutput || aErrors )
    {
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdInput = GetStdHandle( STD_INPUT_HANDLE );
        si.hStdOutput = aOutput ? outWrite : GetStdHandle( STD_OUTPUT_HANDLE );
        si.hStdError = aErrors ? errWrite : GetStdHandle( STD_ERROR_HANDLE );
    }

    if( !( aFlags & wxEXEC_SHOW_CONSOLE ) )
    {
        si.dwFlags |= STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
    }

    DWORD        creationFlags = CREATE_UNICODE_ENVIRONMENT;
    std::wstring envBlock;
    const wchar_t* cwd = nullptr;

    if( aEnv )
    {
        if( !aEnv->cwd.empty() )
            cwd = aEnv->cwd.wc_str();

        if( !aEnv->env.empty() )
        {
            for( const auto& [name, value] : aEnv->env )
            {
                envBlock += ( name + wxT( "=" ) + value ).ToStdWstring();
                envBlock += L'\0';
            }

            envBlock += L'\0';
        }
    }

    std::wstring        cmd = aCommandLine.ToStdWstring();
    PROCESS_INFORMATION pi = {};
    BOOL ok = CreateProcessW( nullptr, cmd.data(), nullptr, nullptr, TRUE, creationFlags,
                              envBlock.empty() ? nullptr : envBlock.data(), cwd, &si, &pi );

    // the write ends belong to the child now
    if( outWrite )
        CloseHandle( outWrite );

    if( errWrite )
        CloseHandle( errWrite );

    if( !ok )
    {
        if( outRead )
            CloseHandle( outRead );

        if( errRead )
            CloseHandle( errRead );

        wxLogSysError( _( "Execution of command '%s' failed" ), aCommandLine );
        return aFlags & wxEXEC_SYNC ? -1 : 0;
    }

    CloseHandle( pi.hThread );

    if( aOutput && aErrors )
    {
        std::thread errReader( readAll, errRead, aErrors );
        readAll( outRead, aOutput );
        errReader.join();
    }
    else if( aOutput )
    {
        readAll( outRead, aOutput );
    }
    else if( aErrors )
    {
        readAll( errRead, aErrors );
    }

    if( !( aFlags & wxEXEC_SYNC ) && !aOutput && !aErrors )
    {
        CloseHandle( pi.hProcess );
        return static_cast<long>( pi.dwProcessId );
    }

    DWORD exitCode = static_cast<DWORD>( -1 );
    WaitForSingleObject( pi.hProcess, INFINITE );
    GetExitCodeProcess( pi.hProcess, &exitCode );
    CloseHandle( pi.hProcess );
    return static_cast<long>( exitCode );
}


long runForProcess( const wxString& aCommandLine, int aFlags, wxProcess* aProcess,
                    const wxExecuteEnv* aEnv )
{
    if( !aProcess || !aProcess->IsRedirected() )
        return run( aCommandLine, aFlags, aEnv, nullptr, nullptr );

    // the output is read until the end: the command runs synchronously
    std::string out, err;
    long        ret = run( aCommandLine, aFlags | wxEXEC_SYNC, aEnv, &out, &err );
    aProcess->SetCapturedOutput( out, err );
    return ret;
}


template <typename CHAR>
wxString commandLine( const CHAR* const* aArgv )
{
    std::wstring cmd;

    for( ; aArgv && *aArgv; ++aArgv )
    {
        if( !cmd.empty() )
            cmd += L' ';

        cmd += quoteArg( wxString( *aArgv ).ToStdWstring() );
    }

    return wxString( cmd );
}

} // namespace


// On Windows the command line goes as it is to the program, which splits it itself.

long wxExecute( const wxString& aCommand, int aFlags, wxProcess* aProcess, const wxExecuteEnv* aEnv )
{
    return runForProcess( aCommand, aFlags, aProcess, aEnv );
}


long wxExecute( const wxString& aCommand, wxArrayString& aOutput, int aFlags, const wxExecuteEnv* aEnv )
{
    std::string out;
    long        ret = run( aCommand, aFlags | wxEXEC_SYNC, aEnv, &out, nullptr );
    kiwx::OutputLines( out, aOutput );
    return ret;
}


long wxExecute( const wxString& aCommand, wxArrayString& aOutput, wxArrayString& aErrors, int aFlags,
                const wxExecuteEnv* aEnv )
{
    std::string out, err;
    long        ret = run( aCommand, aFlags | wxEXEC_SYNC, aEnv, &out, &err );
    kiwx::OutputLines( out, aOutput );
    kiwx::OutputLines( err, aErrors );
    return ret;
}


long wxExecute( const char* const* aArgv, int aFlags, wxProcess* aProcess, const wxExecuteEnv* aEnv )
{
    return runForProcess( commandLine( aArgv ), aFlags, aProcess, aEnv );
}


long wxExecute( const wchar_t* const* aArgv, int aFlags, wxProcess* aProcess, const wxExecuteEnv* aEnv )
{
    return runForProcess( commandLine( aArgv ), aFlags, aProcess, aEnv );
}


int wxKill( long aPid, wxSignal aSig, wxKillError* aRc, int )
{
    // No signals on Windows: wxSIGNONE checks that the process exists, the others end it
    DWORD       access = aSig == wxSIGNONE ? SYNCHRONIZE : PROCESS_TERMINATE;
    HANDLE      process = OpenProcess( access, FALSE, static_cast<DWORD>( aPid ) );
    wxKillError rc = wxKILL_OK;

    if( !process )
    {
        rc = GetLastError() == ERROR_ACCESS_DENIED ? wxKILL_ACCESS_DENIED : wxKILL_NO_PROCESS;
    }
    else
    {
        if( aSig == wxSIGNONE )
        {
            // a process that has ended may still be open somewhere
            if( WaitForSingleObject( process, 0 ) == WAIT_OBJECT_0 )
                rc = wxKILL_NO_PROCESS;
        }
        else if( !TerminateProcess( process, static_cast<UINT>( -1 ) ) )
        {
            rc = wxKILL_ERROR;
        }

        CloseHandle( process );
    }

    if( aRc )
        *aRc = rc;

    return rc == wxKILL_OK ? 0 : -1;
}


bool wxShell( const wxString& aCommand )
{
    wxString shell;

    if( !wxGetEnv( wxT( "COMSPEC" ), &shell ) )
        shell = wxT( "\\COMMAND.COM" );

    wxString cmd = aCommand.empty() ? shell : shell + wxT( " /c " ) + aCommand;
    return wxExecute( cmd, wxEXEC_SYNC ) == 0;
}


/// Open a document or a URL with the shell, as a double click in the Explorer.
static bool launchDefault( const wxString& aTarget )
{
    HINSTANCE ret = ShellExecuteW( nullptr, L"open", aTarget.wc_str(), nullptr, nullptr,
                                   SW_SHOWNORMAL );

    // greater than 32 on success (a HINSTANCE for compatibility with the 16 bit Windows)
    return reinterpret_cast<INT_PTR>( ret ) > 32;
}


bool wxLaunchDefaultApplication( const wxString& aDocument, int aFlags )
{
    return launchDefault( aDocument );
}


bool wxLaunchDefaultBrowser( const wxString& aUrl, int aFlags )
{
    return launchDefault( aUrl );
}


// wxFileName

bool wxFileName::GetTimes( wxDateTime* aAccess, wxDateTime* aMod, wxDateTime* aCreate ) const
{
    WIN32_FILE_ATTRIBUTE_DATA data;

    if( !GetFileAttributesExW( GetFullPath().wc_str(), GetFileExInfoStandard, &data ) )
        return false;

    if( aAccess )
        *aAccess = fromFileTime( data.ftLastAccessTime );

    if( aMod )
        *aMod = fromFileTime( data.ftLastWriteTime );

    if( aCreate )
        *aCreate = fromFileTime( data.ftCreationTime );

    return true;
}


bool wxFileName::SetTimes( const wxDateTime* aAccess, const wxDateTime* aMod,
                           const wxDateTime* aCreate ) const
{
    if( !aAccess && !aMod && !aCreate )
        return true;

    // FILE_FLAG_BACKUP_SEMANTICS for the directories
    HANDLE file = CreateFileW( GetFullPath().wc_str(), FILE_WRITE_ATTRIBUTES,
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                               OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr );

    if( file == INVALID_HANDLE_VALUE )
        return false;

    FILETIME access, mod, create;

    if( aAccess )
        access = toFileTime( *aAccess );

    if( aMod )
        mod = toFileTime( *aMod );

    if( aCreate )
        create = toFileTime( *aCreate );

    bool ok = SetFileTime( file, aCreate ? &create : nullptr, aAccess ? &access : nullptr,
                           aMod ? &mod : nullptr ) != 0;
    CloseHandle( file );
    return ok;
}


bool wxFileName::Touch() const
{
    wxDateTime now = wxDateTime::UNow();
    return SetTimes( &now, &now, nullptr );
}


wxFileName wxFileName::ResolveLink()
{
    // as wxMSW, the links are not resolved
    return *this;
}


wxString wxFileName::GetTempDir()
{
    wxString dir;

    for( const wchar_t* var : { L"TMPDIR", L"TMP", L"TEMP" } )
    {
        if( wxGetEnv( var, &dir ) && wxDirExists( dir ) )
            break;

        dir.clear();
    }

    if( dir.empty() )
    {
        wchar_t buf[MAX_PATH + 1];
        DWORD   len = GetTempPathW( MAX_PATH + 1, buf );
        dir = wxString( buf, len );
    }

    if( dir.empty() )
        return wxT( "." );

    // no trailing separators
    size_t lastNonSep = dir.find_last_not_of( GetPathSeparators() );

    if( lastNonSep == wxString::npos )
        return wxString( GetPathSeparator() );

    dir.erase( lastNonSep + 1 );
    return dir;
}


namespace
{

/// GetTempFileName() (the file is created, empty) in the directory of @a aPrefix.
wxString makeTemp( const wxString& aPrefix )
{
    wxString dir, name;
    wxFileName::SplitPath( aPrefix, &dir, &name, nullptr );

    if( dir.empty() )
        dir = wxFileName::GetTempDir();

    wchar_t buf[MAX_PATH];

    if( !GetTempFileNameW( dir.wc_str(), name.wc_str(), 0, buf ) )
    {
        wxLogSysError( _( "Failed to create a temporary file name" ) );
        return wxString();
    }

    return wxString( buf );
}

} // namespace


wxString wxFileName::CreateTempFileName( const wxString& aPrefix, wxFile* aFileTemp )
{
    wxString name = makeTemp( aPrefix );

    if( name.empty() || !aFileTemp )
        return name;

    if( !aFileTemp->Open( name, wxFile::read_write ) )
    {
        wxRemoveFile( name );
        return wxString();
    }

    return name;
}


wxString wxFileName::CreateTempFileName( const wxString& aPrefix, wxFFile* aFileTemp )
{
    wxString name = makeTemp( aPrefix );

    if( name.empty() || !aFileTemp )
        return name;

    if( !aFileTemp->Open( name, wxT( "w+b" ) ) )
    {
        wxRemoveFile( name );
        return wxString();
    }

    return name;
}


wxString wxFileName::GetLongPath() const
{
    wxString path = GetFullPath();
    DWORD    len = GetLongPathNameW( path.wc_str(), nullptr, 0 );

    if( len == 0 )
        return path;    // a path that does not exist stays as it is

    std::wstring buf( len, L'\0' );
    len = GetLongPathNameW( path.wc_str(), buf.data(), len );

    if( len == 0 )
        return path;

    buf.resize( len );
    return wxString( buf );
}


wxString wxFileName::GetShortPath() const
{
    wxString path = GetFullPath();
    DWORD    len = GetShortPathNameW( path.wc_str(), nullptr, 0 );

    if( len == 0 )
        return path;

    std::wstring buf( len, L'\0' );
    len = GetShortPathNameW( path.wc_str(), buf.data(), len );

    if( len == 0 )
        return path;

    buf.resize( len );
    return wxString( buf );
}
