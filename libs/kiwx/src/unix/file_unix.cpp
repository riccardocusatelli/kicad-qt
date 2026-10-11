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
 * @file file_unix.cpp
 *
 * wxFile, wxFFile and wxDir on Unix (POSIX).  The Windows versions are in
 * src/msw/file_msw.cpp.
 */

#include <wx/dir.h>
#include <wx/ffile.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/intl.h>
#include <wx/log.h>

#include "../kiwx_private.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>


namespace
{

std::string fsPath( const wxString& aPath )
{
    return aPath.utf8_string();
}

} // namespace


bool wxFile::Access( const wxString& aName, OpenMode aMode )
{
    int how = aMode == read ? R_OK : aMode == read_write ? R_OK | W_OK : W_OK;
    return access( fsPath( aName ).c_str(), how ) == 0;
}


bool wxFile::Create( const wxString& aFileName, bool aOverwrite, int aPerms )
{
    Close();

    int flags = O_WRONLY | O_CREAT | ( aOverwrite ? O_TRUNC : O_EXCL );
    m_fd = open( fsPath( aFileName ).c_str(), flags, aPerms );

    if( m_fd < 0 )
    {
        m_lastError = errno;
        m_fd = fd_invalid;
        wxLogSysError( _( "can't create file '%s'" ), aFileName );
        return false;
    }

    return true;
}


bool wxFile::Open( const wxString& aFileName, OpenMode aMode, int aPerms )
{
    Close();

    int flags = O_RDONLY;

    switch( aMode )
    {
    case read:         flags = O_RDONLY; break;
    case write:        flags = O_WRONLY | O_CREAT | O_TRUNC; break;
    case write_excl:   flags = O_WRONLY | O_CREAT | O_EXCL; break;
    case write_append: flags = O_WRONLY | O_CREAT | O_APPEND; break;
    case read_write:   flags = O_RDWR; break;
    }

    m_fd = open( fsPath( aFileName ).c_str(), flags, aPerms );

    if( m_fd < 0 )
    {
        m_lastError = errno;
        m_fd = fd_invalid;
        wxLogSysError( _( "can't open file '%s'" ), aFileName );
        return false;
    }

    return true;
}


bool wxFile::Close()
{
    if( !IsOpened() )
        return true;

    int rc = close( m_fd );
    m_fd = fd_invalid;

    if( rc != 0 )
    {
        m_lastError = errno;
        return false;
    }

    return true;
}


ssize_t wxFile::Read( void* aBuf, size_t aCount )
{
    ssize_t n = ::read( m_fd, aBuf, aCount );

    if( n < 0 )
    {
        m_lastError = errno;
        m_error = true;
        return wxInvalidOffset;
    }

    return n;
}


size_t wxFile::Write( const void* aBuf, size_t aCount )
{
    const char* p = static_cast<const char*>( aBuf );
    size_t      done = 0;

    while( done < aCount )
    {
        ssize_t n = ::write( m_fd, p + done, aCount - done );

        if( n < 0 )
        {
            m_lastError = errno;
            m_error = true;
            break;
        }

        done += n;
    }

    return done;
}


bool wxFile::Flush()
{
    return !IsOpened() || fsync( m_fd ) == 0 || errno == EINVAL;
}


wxFileOffset wxFile::Seek( wxFileOffset aOffset, wxSeekMode aMode )
{
    int whence = aMode == wxFromStart ? SEEK_SET : aMode == wxFromCurrent ? SEEK_CUR : SEEK_END;
    off_t pos = lseek( m_fd, aOffset, whence );
    return pos < 0 ? wxInvalidOffset : pos;
}


wxFileOffset wxFile::Tell() const
{
    off_t pos = lseek( m_fd, 0, SEEK_CUR );
    return pos < 0 ? wxInvalidOffset : pos;
}


wxFileOffset wxFile::Length() const
{
    struct stat st;
    return fstat( m_fd, &st ) == 0 ? st.st_size : wxInvalidOffset;
}


bool wxTempFile::Open( const wxString& aName )
{
    wxFileName fn( aName );

    if( !fn.IsAbsolute() )
        fn.MakeAbsolute();

    m_name = fn.GetFullPath();
    m_tempName = wxFileName::CreateTempFileName( m_name, &m_file );

    if( m_tempName.empty() )
        return false;

    // Keep the permissions of the file being replaced.
    struct stat st;

    if( stat( fsPath( m_name ).c_str(), &st ) == 0 )
        fchmod( m_file.fd(), st.st_mode & 07777 );
    else
        fchmod( m_file.fd(), 0666 & ~[]() { mode_t m = umask( 0 ); umask( m ); return m; }() );

    return true;
}


bool wxFFile::Open( const wxString& aFileName, const wxString& aMode )
{
    Close();

    m_fp = std::fopen( fsPath( aFileName ).c_str(), fsPath( aMode ).c_str() );

    if( !m_fp )
    {
        wxLogSysError( _( "can't open file '%s'" ), aFileName );
        return false;
    }

    m_name = aFileName;
    return true;
}


bool wxFFile::Seek( wxFileOffset aOffset, wxSeekMode aMode )
{
    int whence = aMode == wxFromStart ? SEEK_SET : aMode == wxFromCurrent ? SEEK_CUR : SEEK_END;
    return fseeko( m_fp, aOffset, whence ) == 0;
}


wxFileOffset wxFFile::Tell() const
{
    return ftello( m_fp );
}


wxFileOffset wxFFile::Length() const
{
    struct stat st;
    return fstat( fileno( m_fp ), &st ) == 0 ? st.st_size : wxInvalidOffset;
}


bool wxDir::Open( const wxString& aDir )
{
    Close();

    m_dirName = aDir;

    // "/dir/" -> "/dir" (but "/" stays)
    while( m_dirName.length() > 1 && m_dirName.Last() == wxT( '/' ) )
        m_dirName.RemoveLast();

    // Like wxWidgets on Unix, a directory that cannot be opened is not reported
    m_dir = opendir( fsPath( m_dirName ).c_str() );

    return m_dir != nullptr;
}


void wxDir::Close()
{
    if( m_dir )
    {
        closedir( static_cast<DIR*>( m_dir ) );
        m_dir = nullptr;
    }
}


bool wxDir::read( wxString* aFileName ) const
{
    DIR*     dir = static_cast<DIR*>( m_dir );
    wxString path = m_dirName + wxT( '/' );

    for( ;; )
    {
        struct dirent* de = readdir( dir );

        if( !de )
            return false;

        wxString name( de->d_name );

        if( de->d_name[0] == '.'
            && ( de->d_name[1] == '\0' || ( de->d_name[1] == '.' && de->d_name[2] == '\0' ) ) )
        {
            if( !( m_flags & wxDIR_DOTDOT ) )
                continue;

            *aFileName = name;
            return true;
        }

        // The type of the entry itself (symlinks are followed unless wxDIR_NO_FOLLOW).
        struct stat st;
        std::string full = fsPath( path + name );
        int         rc = ( m_flags & wxDIR_NO_FOLLOW ) ? lstat( full.c_str(), &st )
                                                       : stat( full.c_str(), &st );
        bool        isDir = rc == 0 && S_ISDIR( st.st_mode );

        if( !( m_flags & wxDIR_DIRS ) && isDir )
            continue;

        if( !( m_flags & wxDIR_FILES ) && !isDir )
            continue;

        bool matches;

        if( m_fileSpec.empty() )
            matches = ( m_flags & wxDIR_HIDDEN ) ? true : de->d_name[0] != '.';
        else
            matches = wxMatchWild( m_fileSpec, name, !( m_flags & wxDIR_HIDDEN ) );

        if( matches )
        {
            *aFileName = name;
            return true;
        }
    }
}


bool wxDir::GetFirst( wxString* aFileName, const wxString& aFileSpec, int aFlags ) const
{
    if( !m_dir )
        return false;

    rewinddir( static_cast<DIR*>( m_dir ) );
    m_fileSpec = aFileSpec;
    m_flags = aFlags;
    return read( aFileName );
}


bool kiwx::RenameReplacing( const wxString& aFrom, const wxString& aTo )
{
    return std::rename( fsPath( aFrom ).c_str(), fsPath( aTo ).c_str() ) == 0;
}
