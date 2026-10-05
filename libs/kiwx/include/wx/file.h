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

#pragma once

// exported from the kiwx library, as the wxWidgets API (WXDLLIMPEXP_BASE)
#pragma GCC visibility push( default )

#include <wx/filefn.h>
#include <wx/strconv.h>
#include <wx/string.h>

#include <sys/types.h>


/// A file descriptor (open(2)) with the wxFile API.
class wxFile
{
public:
    enum OpenMode
    {
        read,
        write,
        read_write,
        write_append,
        write_excl
    };

    enum
    {
        fd_invalid = -1,
        fd_stdin,
        fd_stdout,
        fd_stderr
    };

    wxFile() = default;
    explicit wxFile( const wxString& aFileName, OpenMode aMode = read ) { Open( aFileName, aMode ); }
    explicit wxFile( int aFd ) : m_fd( aFd ) {}
    ~wxFile() { Close(); }

    wxFile( const wxFile& ) = delete;
    wxFile& operator=( const wxFile& ) = delete;

    static bool Exists( const wxString& aName ) { return wxFileExists( aName ); }
    static bool Access( const wxString& aName, OpenMode aMode );

    bool Create( const wxString& aFileName, bool aOverwrite = false, int aPerms = wxS_DEFAULT );
    bool Open( const wxString& aFileName, OpenMode aMode = read, int aPerms = wxS_DEFAULT );
    bool Close();
    void Attach( int aFd ) { Close(); m_fd = aFd; }
    int  Detach() { int fd = m_fd; m_fd = fd_invalid; return fd; }
    int  fd() const { return m_fd; }

    bool IsOpened() const { return m_fd != fd_invalid; }

    ssize_t Read( void* aBuf, size_t aCount );
    size_t  Write( const void* aBuf, size_t aCount );
    bool    Write( const wxString& aStr, const wxMBConv& aConv = wxConvUTF8 );
    bool    ReadAll( wxString* aStr, const wxMBConv& aConv = wxConvAuto() );
    bool    Flush();

    wxFileOffset Seek( wxFileOffset aOffset, wxSeekMode aMode = wxFromStart );
    wxFileOffset SeekEnd( wxFileOffset aOffset = 0 ) { return Seek( aOffset, wxFromEnd ); }
    wxFileOffset Tell() const;
    wxFileOffset Length() const;
    bool         Eof() const;
    bool         Error() const { return m_error; }
    int          GetLastError() const { return m_lastError; }
    void         ClearLastError() { m_lastError = 0; }
    wxFileKind   GetKind() const { return wxGetFileKind( m_fd ); }

private:
    int  m_fd = fd_invalid;
    bool m_error = false;
    int  m_lastError = 0;
};


/// Writes to a temporary file, renamed to the target by Commit().
class wxTempFile
{
public:
    wxTempFile() = default;
    explicit wxTempFile( const wxString& aName ) { Open( aName ); }
    ~wxTempFile();

    bool Open( const wxString& aName );
    bool IsOpened() const { return m_file.IsOpened(); }
    bool Write( const void* aBuf, size_t aCount ) { return m_file.Write( aBuf, aCount ) == aCount; }
    bool Write( const wxString& aStr, const wxMBConv& aConv = wxConvUTF8 ) { return m_file.Write( aStr, aConv ); }
    bool Flush() { return m_file.Flush(); }
    wxFileOffset Length() const { return m_file.Length(); }
    wxFileOffset Tell() const { return m_file.Tell(); }
    bool Commit();
    void Discard();

private:
    wxString m_name;
    wxString m_tempName;
    wxFile   m_file;
};

#pragma GCC visibility pop
