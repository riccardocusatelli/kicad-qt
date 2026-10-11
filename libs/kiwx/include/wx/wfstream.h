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
#ifndef _WIN32
#pragma GCC visibility push( default )
#endif

#include <wx/ffile.h>
#include <wx/file.h>
#include <wx/stream.h>


class wxFileInputStream : public wxInputStream
{
public:
    explicit wxFileInputStream( const wxString& aFileName );
    explicit wxFileInputStream( wxFile& aFile ) : m_file( &aFile ) {}
    explicit wxFileInputStream( int aFd ) : m_file( new wxFile( aFd ) ), m_owns( true ) {}
    ~wxFileInputStream() override;

    wxFileOffset GetLength() const override { return m_file->Length(); }
    bool         IsSeekable() const override { return true; }
    bool         IsOk() const override { return wxStreamBase::IsOk() && m_file->IsOpened(); }
    wxFile*      GetFile() const { return m_file; }

protected:
    size_t       OnSysRead( void* aBuffer, size_t aSize ) override;
    wxFileOffset OnSysSeek( wxFileOffset aPos, wxSeekMode aMode ) override { return m_file->Seek( aPos, aMode ); }
    wxFileOffset OnSysTell() const override { return m_file->Tell(); }

    wxFile* m_file;
    bool    m_owns = false;
};


class wxFileOutputStream : public wxOutputStream
{
public:
    explicit wxFileOutputStream( const wxString& aFileName );
    explicit wxFileOutputStream( wxFile& aFile ) : m_file( &aFile ) {}
    explicit wxFileOutputStream( int aFd ) : m_file( new wxFile( aFd ) ), m_owns( true ) {}
    ~wxFileOutputStream() override;

    wxFileOffset GetLength() const override { return m_file->Length(); }
    bool         IsSeekable() const override { return true; }
    bool         IsOk() const override { return wxStreamBase::IsOk() && m_file->IsOpened(); }
    bool         Close() override { return m_file->Close(); }
    void         Sync() override { m_file->Flush(); }
    wxFile*      GetFile() const { return m_file; }

protected:
    size_t       OnSysWrite( const void* aBuffer, size_t aSize ) override;
    wxFileOffset OnSysSeek( wxFileOffset aPos, wxSeekMode aMode ) override { return m_file->Seek( aPos, aMode ); }
    wxFileOffset OnSysTell() const override { return m_file->Tell(); }

    wxFile* m_file;
    bool    m_owns = false;
};


/// Writes to a temporary file that replaces the target on Commit().
class wxTempFileOutputStream : public wxOutputStream
{
public:
    explicit wxTempFileOutputStream( const wxString& aFileName ) : m_file( aFileName ) {}

    bool IsOk() const override { return wxStreamBase::IsOk() && m_file.IsOpened(); }
    bool Commit() { return m_file.Commit(); }
    void Discard() { m_file.Discard(); }
    bool Close() override { return Commit(); }

    wxFileOffset GetLength() const override { return m_file.Length(); }

protected:
    size_t OnSysWrite( const void* aBuffer, size_t aSize ) override
    {
        return m_file.Write( aBuffer, aSize ) ? aSize : 0;
    }

private:
    wxTempFile m_file;
};


class wxFFileInputStream : public wxInputStream
{
public:
    explicit wxFFileInputStream( const wxString& aFileName, const wxString& aMode = wxT( "rb" ) );
    explicit wxFFileInputStream( wxFFile& aFile ) : m_file( &aFile ) {}
    explicit wxFFileInputStream( FILE* aFp ) : m_file( new wxFFile( aFp ) ), m_owns( true ) {}
    ~wxFFileInputStream() override;

    wxFileOffset GetLength() const override { return m_file->Length(); }
    bool         IsSeekable() const override { return true; }
    bool         IsOk() const override { return wxStreamBase::IsOk() && m_file->IsOpened(); }
    wxFFile*     GetFile() const { return m_file; }

protected:
    size_t       OnSysRead( void* aBuffer, size_t aSize ) override;
    wxFileOffset OnSysSeek( wxFileOffset aPos, wxSeekMode aMode ) override
    {
        return m_file->Seek( aPos, aMode ) ? m_file->Tell() : wxInvalidOffset;
    }
    wxFileOffset OnSysTell() const override { return m_file->Tell(); }

    wxFFile* m_file;
    bool     m_owns = false;
};


class wxFFileOutputStream : public wxOutputStream
{
public:
    explicit wxFFileOutputStream( const wxString& aFileName, const wxString& aMode = wxT( "wb" ) );
    explicit wxFFileOutputStream( wxFFile& aFile ) : m_file( &aFile ) {}
    explicit wxFFileOutputStream( FILE* aFp ) : m_file( new wxFFile( aFp ) ), m_owns( true ) {}
    ~wxFFileOutputStream() override;

    wxFileOffset GetLength() const override { return m_file->Length(); }
    bool         IsSeekable() const override { return true; }
    bool         IsOk() const override { return wxStreamBase::IsOk() && m_file->IsOpened(); }
    bool         Close() override { return m_file->Close(); }
    void         Sync() override { m_file->Flush(); }
    wxFFile*     GetFile() const { return m_file; }

protected:
    size_t       OnSysWrite( const void* aBuffer, size_t aSize ) override;
    wxFileOffset OnSysSeek( wxFileOffset aPos, wxSeekMode aMode ) override
    {
        return m_file->Seek( aPos, aMode ) ? m_file->Tell() : wxInvalidOffset;
    }
    wxFileOffset OnSysTell() const override { return m_file->Tell(); }

    wxFFile* m_file;
    bool     m_owns = false;
};


typedef wxFileInputStream  wxFileStream;
typedef wxFFileInputStream wxFFileStream;

#ifndef _WIN32
#pragma GCC visibility pop
#endif
