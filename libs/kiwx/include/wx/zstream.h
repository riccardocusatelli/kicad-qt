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

#include <wx/stream.h>

#include <memory>

enum
{
    wxZ_DEFAULT_COMPRESSION = -1,
    wxZ_NO_COMPRESSION = 0,
    wxZ_BEST_SPEED = 1,
    wxZ_BEST_COMPRESSION = 9
};

enum wxZLibFlags
{
    wxZLIB_NO_HEADER = 0,   ///< raw deflate
    wxZLIB_ZLIB = 1,        ///< zlib header and checksum
    wxZLIB_GZIP = 2,        ///< gzip header and checksum
    wxZLIB_AUTO = 3         ///< zlib or gzip (input only)
};

struct z_stream_s;


/// A port of the wxWidgets zlib streams: same zlib parameters and flushes, so the same bytes.
class wxZlibInputStream : public wxFilterInputStream
{
public:
    explicit wxZlibInputStream( wxInputStream& aStream, int aFlags = wxZLIB_AUTO );
    explicit wxZlibInputStream( wxInputStream* aStream, int aFlags = wxZLIB_AUTO );
    ~wxZlibInputStream() override;

    char         Peek() override { return wxInputStream::Peek(); }
    wxFileOffset GetLength() const override { return wxInvalidOffset; }

    static bool CanHandleGZip() { return true; }
    bool        SetDictionary( const char* aData, size_t aSize );

protected:
    size_t       OnSysRead( void* aBuffer, size_t aSize ) override;
    wxFileOffset OnSysTell() const override { return m_pos; }

private:
    void init( int aFlags );

    std::vector<unsigned char> m_buffer;
    z_stream_s*                m_inflate = nullptr;
    wxFileOffset               m_pos = 0;
};


class wxZlibOutputStream : public wxFilterOutputStream
{
public:
    explicit wxZlibOutputStream( wxOutputStream& aStream, int aLevel = -1, int aFlags = wxZLIB_ZLIB );
    explicit wxZlibOutputStream( wxOutputStream* aStream, int aLevel = -1, int aFlags = wxZLIB_ZLIB );
    ~wxZlibOutputStream() override { Close(); }

    void         Sync() override { doFlush( false ); }
    bool         Close() override;
    wxFileOffset GetLength() const override { return m_pos; }

    static bool CanHandleGZip() { return true; }
    bool        SetDictionary( const char* aData, size_t aSize );

protected:
    size_t       OnSysWrite( const void* aBuffer, size_t aSize ) override;
    wxFileOffset OnSysTell() const override { return m_pos; }

private:
    void init( int aLevel, int aFlags );
    void doFlush( bool aFinal );

    std::vector<unsigned char> m_buffer;
    z_stream_s*                m_deflate = nullptr;
    wxFileOffset               m_pos = 0;
};

#ifndef _WIN32
#pragma GCC visibility pop
#endif
