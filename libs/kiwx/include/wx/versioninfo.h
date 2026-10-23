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

/**
 * @file versioninfo.h
 * The version of a library (wxVersionInfo of wxWidgets).
 */

#include <wx/string.h>


class wxVersionInfo
{
public:
    wxVersionInfo( const wxString& aName = wxString(), int aMajor = 0, int aMinor = 0,
                   int aMicro = 0, const wxString& aDescription = wxString(),
                   const wxString& aCopyright = wxString() ) :
            m_name( aName ),
            m_description( aDescription ),
            m_copyright( aCopyright ),
            m_major( aMajor ),
            m_minor( aMinor ),
            m_micro( aMicro )
    {
    }

    const wxString& GetName() const { return m_name; }
    int GetMajor() const { return m_major; }
    int GetMinor() const { return m_minor; }
    int GetMicro() const { return m_micro; }

    /// "name major.minor.micro" (the micro version only if not 0).
    wxString ToString() const { return HasDescription() ? GetDescription() : GetVersionString(); }

    wxString GetVersionString() const
    {
        wxString str = m_name + wxS( " " ) + wxString::Format( wxS( "%d.%d" ), m_major, m_minor );

        if( m_micro )
            str += wxString::Format( wxS( ".%d" ), m_micro );

        return str;
    }

    bool HasDescription() const { return !m_description.empty(); }
    const wxString& GetDescription() const { return m_description; }

    bool HasCopyright() const { return !m_copyright.empty(); }
    const wxString& GetCopyright() const { return m_copyright; }

private:
    wxString m_name;
    wxString m_description;
    wxString m_copyright;
    int      m_major;
    int      m_minor;
    int      m_micro;
};
