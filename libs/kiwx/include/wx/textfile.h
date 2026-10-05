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

#include <wx/file.h>
#include <wx/textbuf.h>


class wxTextFile : public wxTextBuffer
{
public:
    wxTextFile() = default;
    explicit wxTextFile( const wxString& aFileName ) : wxTextBuffer( aFileName ) {}

protected:
    bool OnExists() const override;
    bool OnOpen( const wxString& aName, int aMode ) override;
    bool OnClose() override;
    bool OnRead( const wxMBConv& aConv ) override;
    bool OnWrite( wxTextFileType aType, const wxMBConv& aConv ) override;

private:
    wxFile m_file;
};

#pragma GCC visibility pop
