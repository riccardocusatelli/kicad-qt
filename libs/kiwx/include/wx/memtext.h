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

#include <wx/textbuf.h>

/// A wxTextBuffer only in memory.
class wxMemoryText : public wxTextBuffer
{
public:
    wxMemoryText() = default;
    explicit wxMemoryText( const wxString& aName ) : wxTextBuffer( aName ) {}

protected:
    bool OnExists() const override { return false; }
    bool OnOpen( const wxString&, int ) override { return true; }
    bool OnClose() override { return true; }
    bool OnRead( const wxMBConv& ) override { return true; }
    bool OnWrite( wxTextFileType, const wxMBConv& ) override { return true; }
};

#ifndef _WIN32
#pragma GCC visibility pop
#endif
