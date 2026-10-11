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
 * @file file_msw.cpp
 *
 * wxFile, wxFFile and wxDir on Windows, as in wxMSW: the files through the C runtime with
 * wide names (in binary mode), the directories through FindFirstFile().  The Unix versions
 * are in src/unix/file_unix.cpp.
 */

#include <wx/dir.h>
#include <wx/ffile.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/intl.h>
#include <wx/log.h>

#include "../kiwx_private.h"

#include <cerrno>
#include <climits>
#include <cstdio>
#include <string>

#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#include <windows.h>
#include <shlwapi.h>


namespace
{

/// The state of an open wxDir.
struct DIR_DATA
{
    HANDLE find = INVALID_HANDLE_VALUE;

    ~DIR_DATA() { Close(); }

    void Close()
    {
        if( find != INVALID_HANDLE_VALUE )
        {
            FindClose( find );
            find = INVALID_HANDLE_VALUE;
        }
    }
};


/// Whether the name matches the specification: FindFirstFile() also matches the short (8.3)
/// names, so "*.bar" would find "foo.bar.baz" too (as wxMSW, check again).
bool checkFoundMatch( const WIN32_FIND_DATAW& aData, const wxString& aSpec )
{
    return aSpec.empty() || PathMatchSpecW( aData.cFileName, aSpec.wc_str() );
}

} // namespace


// wxFile

bool wxFile::Access( const wxString& aName, OpenMode aMode )
{
    int how = aMode == read ? 4 : aMode == read_write ? 6 : 2;   // R_OK, W_OK
    return _waccess( aName.wc_str(), how ) == 0;
}


bool wxFile::Create( const wxString& aFileName, bool aOverwrite, int aPerms )
{
    Close();

    int flags = _O_BINARY | _O_WRONLY | _O_CREAT | ( aOverwrite ? _O_TRUNC : _O_EXCL );
    int pmode = ( aPerms & 0222 ) ? _S_IREAD | _S_IWRITE : _S_IREAD;
    m_fd = _wopen( aFileName.wc_str(), flags, pmode );

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

    int flags = _O_RDONLY;

    switch( aMode )
    {
    case read:         flags = _O_RDONLY; break;
    case write:        flags = _O_WRONLY | _O_CREAT | _O_TRUNC; break;
    case write_excl:   flags = _O_WRONLY | _O_CREAT | _O_EXCL; break;
    case write_append: flags = _O_WRONLY | _O_CREAT | _O_APPEND; break;
    case read_write:   flags = _O_RDWR; break;
    }

    int pmode = ( aPerms & 0222 ) ? _S_IREAD | _S_IWRITE : _S_IREAD;
    m_fd = _wopen( aFileName.wc_str(), flags | _O_BINARY, pmode );

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

    int rc = _close( m_fd );
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
    // _read() takes an unsigned int count
    unsigned int count = aCount > INT_MAX ? INT_MAX : static_cast<unsigned int>( aCount );
    int          n = _read( m_fd, aBuf, count );

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
        size_t       left = aCount - done;
        unsigned int count = left > INT_MAX ? INT_MAX : static_cast<unsigned int>( left );
        int          n = _write( m_fd, p + done, count );

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
    return !IsOpened() || _commit( m_fd ) == 0 || errno == EBADF;
}


wxFileOffset wxFile::Seek( wxFileOffset aOffset, wxSeekMode aMode )
{
    int whence = aMode == wxFromStart ? SEEK_SET : aMode == wxFromCurrent ? SEEK_CUR : SEEK_END;
    long long pos = _lseeki64( m_fd, aOffset, whence );
    return pos < 0 ? wxInvalidOffset : pos;
}


wxFileOffset wxFile::Tell() const
{
    long long pos = _telli64( m_fd );
    return pos < 0 ? wxInvalidOffset : pos;
}


wxFileOffset wxFile::Length() const
{
    long long len = _filelengthi64( m_fd );
    return len < 0 ? wxInvalidOffset : len;
}


// wxTempFile

bool wxTempFile::Open( const wxString& aName )
{
    wxFileName fn( aName );

    if( !fn.IsAbsolute() )
        fn.MakeAbsolute();

    m_name = fn.GetFullPath();
    m_tempName = wxFileName::CreateTempFileName( m_name, &m_file );

    // No permissions to keep on Windows
    return !m_tempName.empty();
}


bool kiwx::RenameReplacing( const wxString& aFrom, const wxString& aTo )
{
    return MoveFileExW( aFrom.wc_str(), aTo.wc_str(),
                        MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED ) != 0;
}


// wxFFile

bool wxFFile::Open( const wxString& aFileName, const wxString& aMode )
{
    Close();

    m_fp = _wfopen( aFileName.wc_str(), aMode.wc_str() );

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
    return _fseeki64( m_fp, aOffset, whence ) == 0;
}


wxFileOffset wxFFile::Tell() const
{
    return _ftelli64( m_fp );
}


wxFileOffset wxFFile::Length() const
{
    long long len = _filelengthi64( _fileno( m_fp ) );
    return len < 0 ? wxInvalidOffset : len;
}


// wxDir

bool wxDir::Open( const wxString& aDir )
{
    Close();

    if( !wxDirExists( aDir ) )
        return false;

    m_dirName = aDir;

    // "C:\dir\" -> "C:\dir" (but "\" and "C:\" stay)
    while( m_dirName.length() > 1 && wxIsPathSeparator( m_dirName.Last() )
           && !( m_dirName.length() == 3 && m_dirName[1] == wxT( ':' ) ) )
    {
        m_dirName.RemoveLast();
    }

    m_dir = new DIR_DATA;
    return true;
}


void wxDir::Close()
{
    delete static_cast<DIR_DATA*>( m_dir );
    m_dir = nullptr;
}


bool wxDir::read( wxString* aFileName ) const
{
    DIR_DATA*        data = static_cast<DIR_DATA*>( m_dir );
    WIN32_FIND_DATAW found;
    bool             first = false;

    if( data->find == INVALID_HANDLE_VALUE )
    {
        wxString spec = m_dirName;

        if( !wxEndsWithPathSeparator( spec ) )
            spec += wxT( '\\' );

        spec += m_fileSpec.empty() ? wxString( wxT( "*.*" ) ) : m_fileSpec;

        data->find = FindFirstFileW( spec.wc_str(), &found );

        if( data->find == INVALID_HANDLE_VALUE )
        {
            DWORD err = GetLastError();

            if( err != ERROR_FILE_NOT_FOUND && err != ERROR_NO_MORE_FILES )
                wxLogSysError( _( "Cannot enumerate files in directory '%s'" ), m_dirName );

            return false;
        }

        first = true;
    }

    for( ;; )
    {
        if( first )
            first = false;
        else if( !FindNextFileW( data->find, &found ) )
            return false;

        if( !checkFoundMatch( found, m_fileSpec ) )
            continue;

        const wchar_t* name = found.cFileName;
        DWORD          attr = found.dwFileAttributes;
        bool           isDir = ( attr & FILE_ATTRIBUTE_DIRECTORY ) != 0;

        // "." and ".." only if asked for
        if( name[0] == L'.' && ( name[1] == 0 || ( name[1] == L'.' && name[2] == 0 ) ) )
        {
            if( !( m_flags & wxDIR_DOTDOT ) )
                continue;
        }

        if( !( m_flags & wxDIR_FILES ) && !isDir )
            continue;

        if( !( m_flags & wxDIR_DIRS ) && isDir )
            continue;

        if( !( m_flags & wxDIR_HIDDEN ) && ( attr & ( FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM ) ) )
            continue;

        *aFileName = name;
        return true;
    }
}


bool wxDir::GetFirst( wxString* aFileName, const wxString& aFileSpec, int aFlags ) const
{
    if( !m_dir )
        return false;

    static_cast<DIR_DATA*>( m_dir )->Close();
    m_fileSpec = aFileSpec;
    m_flags = aFlags;
    return read( aFileName );
}
