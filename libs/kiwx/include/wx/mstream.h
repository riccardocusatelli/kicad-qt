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

#include <wx/stream.h>

#include <memory>

class wxMemoryOutputStream;


class wxMemoryInputStream : public wxInputStream
{
public:
    wxMemoryInputStream( const void* aData, size_t aLen );
    explicit wxMemoryInputStream( const wxMemoryOutputStream& aStream );
    explicit wxMemoryInputStream( wxInputStream& aStream, wxFileOffset aLen = wxInvalidOffset );

    wxFileOffset    GetLength() const override { return static_cast<wxFileOffset>( m_data.size() ); }
    bool            IsSeekable() const override { return true; }
    bool            CanRead() const override { return m_pos != m_data.size(); }
    char            Peek() override;
    wxStreamBuffer* GetInputStreamBuffer() const { return m_buffer.get(); }

protected:
    size_t       OnSysRead( void* aBuffer, size_t aSize ) override;
    wxFileOffset OnSysSeek( wxFileOffset aPos, wxSeekMode aMode ) override;
    wxFileOffset OnSysTell() const override { return static_cast<wxFileOffset>( m_pos ); }

private:
    std::vector<char>               m_data;
    size_t                          m_pos = 0;
    std::unique_ptr<wxStreamBuffer> m_buffer;
};


class wxMemoryOutputStream : public wxOutputStream
{
public:
    explicit wxMemoryOutputStream( void* aData = nullptr, size_t aLen = 0 );

    wxFileOffset    GetLength() const override { return static_cast<wxFileOffset>( m_data.size() ); }
    bool            IsSeekable() const override { return true; }
    size_t          CopyTo( void* aBuffer, size_t aLen ) const;
    wxStreamBuffer* GetOutputStreamBuffer() const { return m_buffer.get(); }

    /// Internal: the data written.
    const std::vector<char>& GetData() const { return m_data; }

protected:
    size_t       OnSysWrite( const void* aBuffer, size_t aSize ) override;
    wxFileOffset OnSysSeek( wxFileOffset aPos, wxSeekMode aMode ) override;
    wxFileOffset OnSysTell() const override { return static_cast<wxFileOffset>( m_pos ); }

private:
    std::vector<char>               m_data;
    size_t                          m_pos = 0;
    std::unique_ptr<wxStreamBuffer> m_buffer;
};

#pragma GCC visibility pop
