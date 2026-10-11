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

#include <wx/defs.h>
#include <wx/string.h>

#include <vector>

enum wxStreamError
{
    wxSTREAM_NO_ERROR = 0,
    wxSTREAM_EOF,
    wxSTREAM_WRITE_ERROR,
    wxSTREAM_READ_ERROR
};

enum wxStreamProtocolType
{
    wxSTREAM_PROTOCOL,
    wxSTREAM_MIMETYPE,
    wxSTREAM_ENCODING,
    wxSTREAM_FILEEXT
};


class wxStreamBase
{
public:
    virtual ~wxStreamBase() = default;

    virtual bool  IsOk() const { return GetLastError() == wxSTREAM_NO_ERROR; }
    bool          operator!() const { return !IsOk(); }
    wxStreamError GetLastError() const { return m_lastError; }
    void          Reset( wxStreamError aError = wxSTREAM_NO_ERROR ) { m_lastError = aError; }

    virtual size_t       GetSize() const;
    virtual wxFileOffset GetLength() const { return wxInvalidOffset; }
    virtual bool         IsSeekable() const { return false; }

protected:
    virtual wxFileOffset OnSysSeek( wxFileOffset, wxSeekMode ) { return wxInvalidOffset; }
    virtual wxFileOffset OnSysTell() const { return wxInvalidOffset; }

    wxStreamError m_lastError = wxSTREAM_NO_ERROR;
};


class wxOutputStream;


class wxInputStream : public wxStreamBase
{
public:
    wxInputStream() = default;

    virtual bool Eof() const;
    virtual bool CanRead() const;

    virtual char Peek();
    int          GetC();
    virtual wxInputStream& Read( void* aBuffer, size_t aSize );
    bool                   ReadAll( void* aBuffer, size_t aSize );
    wxInputStream&         Read( wxOutputStream& aStream );
    virtual size_t         LastRead() const { return m_lastCount; }

    size_t Ungetch( const void* aBuffer, size_t aSize );
    bool   Ungetch( char aCh ) { return Ungetch( &aCh, 1 ) == 1; }

    virtual wxFileOffset SeekI( wxFileOffset aPos, wxSeekMode aMode = wxFromStart );
    virtual wxFileOffset TellI() const;

    wxInputStream& operator>>( wxOutputStream& aOut ) { return Read( aOut ); }

protected:
    virtual size_t OnSysRead( void* aBuffer, size_t aSize ) = 0;

    size_t getWBack( void* aBuffer, size_t aSize );

    std::vector<char> m_wback;        ///< data pushed back by Ungetch() / Peek()
    size_t            m_wbackPos = 0;
    mutable size_t    m_lastCount = 0;
};


class wxOutputStream : public wxStreamBase
{
public:
    wxOutputStream() = default;

    virtual wxOutputStream& Write( const void* aBuffer, size_t aSize );
    bool                    WriteAll( const void* aBuffer, size_t aSize );
    wxOutputStream&         Write( wxInputStream& aStream );
    void                    PutC( char aCh ) { Write( &aCh, 1 ); }
    virtual size_t          LastWrite() const { return m_lastCount; }

    virtual wxFileOffset SeekO( wxFileOffset aPos, wxSeekMode aMode = wxFromStart );
    virtual wxFileOffset TellO() const;

    virtual void Sync() {}
    virtual bool Close() { return true; }

    wxOutputStream& operator<<( wxInputStream& aIn ) { return Write( aIn ); }

protected:
    virtual size_t OnSysWrite( const void* aBuffer, size_t aSize ) = 0;

    size_t m_lastCount = 0;
};


/// A stream of a stream (compression, archives): reads from or writes to the parent.
class wxFilterInputStream : public wxInputStream
{
public:
    wxFilterInputStream() = default;
    explicit wxFilterInputStream( wxInputStream& aStream ) : m_parent( &aStream ) {}
    explicit wxFilterInputStream( wxInputStream* aStream ) : m_parent( aStream ), m_owns( true ) {}
    ~wxFilterInputStream() override
    {
        if( m_owns )
            delete m_parent;
    }

    wxInputStream* GetFilterInputStream() const { return m_parent; }

    char         Peek() override { return m_parent->Peek(); }
    wxFileOffset GetLength() const override { return m_parent->GetLength(); }

protected:
    wxInputStream* m_parent = nullptr;
    bool           m_owns = false;
};


class wxFilterOutputStream : public wxOutputStream
{
public:
    wxFilterOutputStream() = default;
    explicit wxFilterOutputStream( wxOutputStream& aStream ) : m_parent( &aStream ) {}
    explicit wxFilterOutputStream( wxOutputStream* aStream ) : m_parent( aStream ), m_owns( true ) {}
    ~wxFilterOutputStream() override
    {
        if( m_owns )
            delete m_parent;
    }

    wxOutputStream* GetFilterOutputStream() const { return m_parent; }

    wxFileOffset GetLength() const override { return m_parent->GetLength(); }

    bool Close() override { return m_parent && m_owns ? m_parent->Close() : true; }

protected:
    wxOutputStream* m_parent = nullptr;
    bool            m_owns = false;
};


/// The buffer of a memory stream (the parts of wxStreamBuffer that are used).
class wxStreamBuffer
{
public:
    explicit wxStreamBuffer( std::vector<char>& aData, size_t* aPos = nullptr ) :
            m_data( aData ), m_pos( aPos )
    {
    }

    void*  GetBufferStart() const { return m_data.data(); }
    void*  GetBufferEnd() const { return m_data.data() + m_data.size(); }
    void*  GetBufferPos() const { return m_data.data() + GetIntPosition(); }
    size_t GetBufferSize() const { return m_data.size(); }
    size_t GetIntPosition() const { return m_pos ? *m_pos : m_data.size(); }
    size_t GetDataLeft() const { return m_data.size() - GetIntPosition(); }
    size_t GetLastAccess() const { return m_data.size(); }

    /// The current position (no stream is behind the buffers of kiwx).
    wxFileOffset Tell() const { return static_cast<wxFileOffset>( GetIntPosition() ); }

private:
    std::vector<char>& m_data;
    size_t*            m_pos;
};


/// Buffering wrappers: the streams below buffer as needed, so these only pass through.
class wxBufferedInputStream : public wxFilterInputStream
{
public:
    explicit wxBufferedInputStream( wxInputStream& aStream ) : wxFilterInputStream( aStream ) {}

    bool         Eof() const override { return m_parent->Eof() && m_wbackPos >= m_wback.size(); }
    wxFileOffset GetLength() const override { return m_parent->GetLength(); }

protected:
    size_t OnSysRead( void* aBuffer, size_t aSize ) override
    {
        m_parent->Read( aBuffer, aSize );
        m_lastError = m_parent->GetLastError();
        return m_parent->LastRead();
    }
};


class wxBufferedOutputStream : public wxFilterOutputStream
{
public:
    explicit wxBufferedOutputStream( wxOutputStream& aStream ) : wxFilterOutputStream( aStream ) {}

protected:
    size_t OnSysWrite( const void* aBuffer, size_t aSize ) override
    {
        m_parent->Write( aBuffer, aSize );
        m_lastError = m_parent->GetLastError();
        return m_parent->LastWrite();
    }
};

#ifndef _WIN32
#pragma GCC visibility pop
#endif
