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
 * @file filefn.cpp
 *
 * The functions on files and directories that the C++ library provides on all the systems
 * (std::filesystem), with the behaviour of wxWidgets.  The ones that need the services of the
 * operating system are in src/unix and src/msw.
 */

#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/intl.h>
#include <wx/log.h>
#include <wx/utils.h>

#include "kiwx_compat.h"
#include "kiwx_private.h"

#include <chrono>
#include <cstdio>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;


namespace
{

/// The path without its trailing separators: "/dir/" -> "/dir", but "/" stays (and, on Windows,
/// "C:\" stays and "C:" is the root "C:\", as wxMSW looks for directories).
wxString stripTrailingSeparators( const wxString& aPath )
{
    wxString path = aPath;

#ifdef _WIN32
    while( path.length() > 1 && wxIsPathSeparator( path.Last() )
           && !( path.length() == 3 && path[1] == wxT( ':' ) ) )
    {
        path.RemoveLast();
    }

    if( path.length() == 2 && path[1] == wxT( ':' ) )
        path += wxT( '\\' );
#else
    while( path.length() > 1 && path.Last() == wxT( '/' ) )
        path.RemoveLast();
#endif

    return path;
}


/// The type of a file (following the symbolic links unless @a aNoFollow), fs::file_type::none
/// if it does not exist.
fs::file_type fileType( const wxString& aPath, bool aNoFollow = false )
{
    if( aPath.empty() )
        return fs::file_type::none;

    std::error_code ec;
    fs::file_status st = aNoFollow ? fs::symlink_status( kiwx::FsPath( aPath ), ec )
                                   : fs::status( kiwx::FsPath( aPath ), ec );

    if( ec || st.type() == fs::file_type::not_found )
        return fs::file_type::none;

    return st.type();
}

} // namespace


bool wxFileExists( const wxString& aFile )
{
    return fileType( aFile ) == fs::file_type::regular;
}


bool wxDirExists( const wxString& aDir )
{
    return fileType( stripTrailingSeparators( aDir ) ) == fs::file_type::directory;
}


bool wxMkdir( const wxString& aDir, int aPerm )
{
    std::error_code ec;
    fs::path        path = kiwx::FsPath( aDir );

    // created with the default permissions, less the umask, as mkdir()
    if( fs::create_directory( path, ec ) )
    {
#ifndef _WIN32
        if( aPerm != wxS_DIR_DEFAULT )
            fs::permissions( path, static_cast<fs::perms>( aPerm ), ec );
#endif
        return true;
    }

    if( !ec )
        ec = std::make_error_code( std::errc::file_exists );

    kiwx::SetErrno( ec );
    wxLogSysError( _( "Directory '%s' couldn't be created" ), aDir );
    return false;
}


bool wxRmdir( const wxString& aDir, int )
{
    std::error_code ec;

    // only directories (std::filesystem::remove() removes files too)
    if( fileType( aDir, true ) != fs::file_type::directory )
        ec = std::make_error_code( std::errc::not_a_directory );
    else if( fs::remove( kiwx::FsPath( aDir ), ec ) )
        return true;

    kiwx::SetErrno( ec );
    wxLogSysError( _( "Directory '%s' couldn't be deleted" ), aDir );
    return false;
}


bool wxRemoveFile( const wxString& aFile )
{
    std::error_code ec;

    // not the directories (std::filesystem::remove() removes the empty ones)
    if( fileType( aFile, true ) == fs::file_type::directory )
        ec = std::make_error_code( std::errc::is_a_directory );
    else if( fs::remove( kiwx::FsPath( aFile ), ec ) )
        return true;
    else if( !ec )
        ec = std::make_error_code( std::errc::no_such_file_or_directory );

    kiwx::SetErrno( ec );
    wxLogSysError( _( "File '%s' couldn't be removed" ), aFile );
    return false;
}


bool wxRenameFile( const wxString& aFrom, const wxString& aTo, bool aOverwrite )
{
    if( !aOverwrite && wxFileExists( aTo ) )
        return false;

    // std::filesystem::rename() replaces the destination, on Windows too
    std::error_code ec;
    fs::rename( kiwx::FsPath( aFrom ), kiwx::FsPath( aTo ), ec );

    if( !ec )
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

    // the permissions are copied too
    std::error_code ec;
    fs::copy_file( kiwx::FsPath( aFrom ), kiwx::FsPath( aTo ), fs::copy_options::overwrite_existing,
                   ec );

    if( ec )
    {
        kiwx::SetErrno( ec );
        return false;
    }

    return true;
}


wxString wxGetCwd()
{
    std::error_code ec;
    fs::path        cwd = fs::current_path( ec );
    return ec ? wxString() : kiwx::FromFsPath( cwd );
}


bool wxSetWorkingDirectory( const wxString& aDir )
{
    std::error_code ec;
    fs::current_path( kiwx::FsPath( aDir ), ec );
    kiwx::SetErrno( ec );
    return !ec;
}


time_t wxFileModificationTime( const wxString& aFile )
{
    std::error_code ec;
    auto            mtime = fs::last_write_time( kiwx::FsPath( aFile ), ec );

    if( ec )
        return -1;

    return std::chrono::system_clock::to_time_t(
            std::chrono::clock_cast<std::chrono::system_clock>( mtime ) );
}


bool wxGetDiskSpace( const wxString& aPath, wxLongLong* aTotal, wxLongLong* aFree )
{
    // only for existing paths (std::filesystem::space() takes the volume of the others)
    if( fileType( aPath ) == fs::file_type::none )
        return false;

    std::error_code ec;
    fs::space_info  info = fs::space( kiwx::FsPath( aPath ), ec );

    if( ec )
        return false;

    if( aTotal )
        *aTotal = static_cast<long long>( info.capacity );

    // the space available to the user
    if( aFree )
        *aFree = static_cast<long long>( info.available );

    return true;
}


long long kiwxFileSize( const wxString& aFile )
{
    std::error_code ec;
    uintmax_t       size = fs::file_size( kiwx::FsPath( aFile ), ec );
    return ec ? -1 : static_cast<long long>( size );
}


wxString wxNow()
{
    // as ctime(), without the new line
    struct tm tmv;
    kiwx::LocalTime( std::time( nullptr ), tmv );

    // ctime() format; %e is not in all the C libraries
    char   day[8];
    char   buf[64];
    std::snprintf( day, sizeof( day ), "%2d", tmv.tm_mday );
    std::string fmt = std::string( "%a %b " ) + day + " %H:%M:%S %Y";
    size_t      len = std::strftime( buf, sizeof( buf ), fmt.c_str(), &tmv );
    return wxString::FromUTF8( buf, len );
}


bool wxFileName::Exists( const wxString& aPath, int aFlags )
{
    fs::file_type type = fileType( aPath, aFlags & wxFILE_EXISTS_NO_FOLLOW );

    if( type == fs::file_type::none )
        return false;

    if( ( aFlags & wxFILE_EXISTS_REGULAR ) && type == fs::file_type::regular )
        return true;

    if( ( aFlags & wxFILE_EXISTS_DIR ) && type == fs::file_type::directory )
        return true;

    if( ( aFlags & wxFILE_EXISTS_SYMLINK & ~wxFILE_EXISTS_NO_FOLLOW ) && type == fs::file_type::symlink )
        return true;

    if( ( aFlags & wxFILE_EXISTS_DEVICE )
        && ( type == fs::file_type::block || type == fs::file_type::character ) )
    {
        return true;
    }

    if( ( aFlags & wxFILE_EXISTS_FIFO ) && type == fs::file_type::fifo )
        return true;

    if( ( aFlags & wxFILE_EXISTS_SOCKET ) && type == fs::file_type::socket )
        return true;

    return aFlags == wxFILE_EXISTS_ANY;
}


bool wxFileName::Rmdir( const wxString& aDir, int aFlags )
{
    std::error_code ec;
    fs::path        path = kiwx::FsPath( aDir );

    if( aFlags & wxPATH_RMDIR_RECURSIVE )
        return fs::remove_all( path, ec ) > 0 && !ec;

    if( aFlags & wxPATH_RMDIR_FULL )
    {
        // Only empty subdirectories.
        for( const auto& entry : fs::recursive_directory_iterator( path, ec ) )
        {
            if( !entry.is_directory() )
                return false;
        }

        return fs::remove_all( path, ec ) > 0 && !ec;
    }

    return wxRmdir( aDir );
}
