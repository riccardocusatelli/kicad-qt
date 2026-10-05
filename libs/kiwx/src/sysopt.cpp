/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Derived from wxWidgets 3.2.4 (src/common/sysopt.cpp, wxWindows Library Licence 3.1),
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

#include <wx/sysopt.h>

#include <wx/app.h>
#include <wx/arrstr.h>
#include <wx/crt.h>

namespace
{
wxArrayString& optionNames()
{
    static wxArrayString s_names;
    return s_names;
}

wxArrayString& optionValues()
{
    static wxArrayString s_values;
    return s_values;
}
} // namespace


void wxSystemOptions::SetOption( const wxString& aName, const wxString& aValue )
{
    int idx = optionNames().Index( aName, false );

    if( idx == wxNOT_FOUND )
    {
        optionNames().Add( aName );
        optionValues().Add( aValue );
    }
    else
    {
        optionNames()[idx] = aName;
        optionValues()[idx] = aValue;
    }
}


void wxSystemOptions::SetOption( const wxString& aName, int aValue )
{
    SetOption( aName, wxString::Format( wxT( "%d" ), aValue ) );
}


wxString wxSystemOptions::GetOption( const wxString& aName )
{
    wxString val;
    int      idx = optionNames().Index( aName, false );

    if( idx != wxNOT_FOUND )
    {
        val = optionValues()[idx];
    }
    else
    {
        // wx_appname_name, then wx_name in the environment
        wxString var( aName );
        var.Replace( wxT( "." ), wxT( "_" ) );
        var.Replace( wxT( "-" ), wxT( "_" ) );

        wxString appname;

        if( wxTheApp )
            appname = wxTheApp->GetAppName();

        if( !appname.empty() )
            val = wxGetenv( wxT( "wx_" ) + appname + wxT( '_' ) + var );

        if( val.empty() )
            val = wxGetenv( wxT( "wx_" ) + var );
    }

    return val;
}


int wxSystemOptions::GetOptionInt( const wxString& aName )
{
    return wxAtoi( GetOption( aName ) );
}


bool wxSystemOptions::HasOption( const wxString& aName )
{
    return !GetOption( aName ).empty();
}
