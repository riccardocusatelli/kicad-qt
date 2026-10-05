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
#include <wx/strconv.h>

#include <memory>


/// Reads the UTF-8 bytes of a string.
class wxStringInputStream : public wxInputStream
{
public:
    explicit wxStringInputStream( const wxString& aStr ) : m_data( aStr.utf8_string() ) {}

    wxFileOffset GetLength() const override { return static_cast<wxFileOffset>( m_data.size() ); }
    bool         IsSeekable() const override { return true; }

protected:
    size_t OnSysRead( void* aBuffer, size_t aSize ) override;
    wxFileOffset OnSysSeek( wxFileOffset aPos, wxSeekMode aMode ) override;
    wxFileOffset OnSysTell() const override { return static_cast<wxFileOffset>( m_pos ); }

private:
    std::string m_data;
    size_t      m_pos = 0;
};


/// Collects the bytes written, decoded (UTF-8 by default) into a string.
class wxStringOutputStream : public wxOutputStream
{
public:
    explicit wxStringOutputStream( wxString* aStr = nullptr, const wxMBConv& aConv = wxConvUTF8 );

    const wxString& GetString() const;
    wxFileOffset    GetLength() const override { return static_cast<wxFileOffset>( m_bytes.size() ); }

protected:
    size_t       OnSysWrite( const void* aBuffer, size_t aSize ) override;
    wxFileOffset OnSysTell() const override { return static_cast<wxFileOffset>( m_bytes.size() ); }

private:
    wxString*                 m_str;
    wxString                  m_ownStr;
    std::string               m_bytes;
    std::unique_ptr<wxMBConv> m_conv;
};

#pragma GCC visibility pop
