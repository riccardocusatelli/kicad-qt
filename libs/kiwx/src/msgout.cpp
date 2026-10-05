/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Derived from wxWidgets 3.2.4 (src/common/msgout.cpp, wxWindows Library Licence 3.1),
 * Copyright (C) the wxWidgets authors.
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

#include <wx/msgout.h>

#include <wx/app.h>

#include <memory>


wxMessageOutput* wxMessageOutput::ms_msgOut = nullptr;


wxMessageOutput* wxMessageOutput::Get()
{
    // as wxWidgets: created on demand while there is an application (a console one here)
    if( !ms_msgOut && wxTheApp )
        ms_msgOut = new wxMessageOutputStderr;

    return ms_msgOut;
}


wxMessageOutput* wxMessageOutput::Set( wxMessageOutput* aOutput )
{
    wxMessageOutput* old = ms_msgOut;
    ms_msgOut = aOutput;
    return old;
}


void wxMessageOutputStderr::Output( const wxString& aStr )
{
    wxString str = aStr;

    if( str.empty() || str.Last() != wxT( '\n' ) )
        str += wxT( '\n' );

    const wxScopedCharBuffer buf = str.utf8_str();
    fwrite( buf.data(), buf.length(), 1, m_fp );
    fflush( m_fp );
}
