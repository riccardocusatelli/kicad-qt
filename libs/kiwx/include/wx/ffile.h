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

#include <wx/filefn.h>
#include <wx/strconv.h>
#include <wx/string.h>

#include <cstdio>


/// A C stream (fopen(3)) with the wxFFile API.
class wxFFile
{
public:
    wxFFile() = default;
    explicit wxFFile( FILE* aFp ) : m_fp( aFp ) {}
    wxFFile( const wxString& aFileName, const wxString& aMode = wxT( "r" ) ) { Open( aFileName, aMode ); }
    ~wxFFile() { Close(); }

    wxFFile( const wxFFile& ) = delete;
    wxFFile& operator=( const wxFFile& ) = delete;

    bool Open( const wxString& aFileName, const wxString& aMode = wxT( "r" ) );
    bool Close();
    void Attach( FILE* aFp, const wxString& aName = wxString() ) { Close(); m_fp = aFp; m_name = aName; }
    FILE* Detach() { FILE* fp = m_fp; m_fp = nullptr; return fp; }
    FILE* fp() const { return m_fp; }

    bool IsOpened() const { return m_fp != nullptr; }

    size_t Read( void* aBuf, size_t aCount );
    size_t Write( const void* aBuf, size_t aCount );
    bool   Write( const wxString& aStr, const wxMBConv& aConv = wxConvUTF8 );
    bool   ReadAll( wxString* aStr, const wxMBConv& aConv = wxConvAuto() );
    bool   Flush();

    bool         Seek( wxFileOffset aOffset, wxSeekMode aMode = wxFromStart );
    bool         SeekEnd( wxFileOffset aOffset = 0 ) { return Seek( aOffset, wxFromEnd ); }
    wxFileOffset Tell() const;
    wxFileOffset Length() const;
    bool         Eof() const;
    bool         Error() const;

    const wxString& GetName() const { return m_name; }
    wxFileKind      GetKind() const { return wxGetFileKind( m_fp ); }

private:
    FILE*    m_fp = nullptr;
    wxString m_name;
};

#ifndef _WIN32
#pragma GCC visibility pop
#endif
