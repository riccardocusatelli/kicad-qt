/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Derived from wxWidgets 3.2.4 (src/common/stdpbase.cpp and src/unix/stdpaths.cpp, wxWindows Library Licence 3.1),
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

#include <wx/stdpaths.h>

#include <wx/app.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/textfile.h>
#include <wx/tokenzr.h>
#include <wx/utils.h>



namespace
{

class STANDARD_PATHS_DEFAULT : public wxStandardPaths
{
public:
    STANDARD_PATHS_DEFAULT() = default;
};

STANDARD_PATHS_DEFAULT& stdPaths()
{
    static STANDARD_PATHS_DEFAULT s_paths;
    return s_paths;
}

} // namespace


// wxStandardPathsBase

wxStandardPaths& wxStandardPathsBase::Get()
{
    // the application "traits" of wxWidgets are the application here
    wxAppConsole* const traits = wxTheApp;
    wxCHECK_MSG( traits, stdPaths(), wxT( "create wxApp before calling this" ) );
    return stdPaths();
}


wxString wxStandardPathsBase::GetExecutablePath() const
{
    if( !wxTheApp || !wxTheApp->argv )
        return wxEmptyString;

    wxString argv0 = wxTheApp->argv[0];

    if( wxIsAbsolutePath( argv0 ) )
        return argv0;

    wxPathList pathlist;
    pathlist.AddEnvList( wxT( "PATH" ) );
    wxString path = pathlist.FindAbsoluteValidPath( argv0 );

    if( path.empty() )
        return argv0;

    return wxFileName( path ).GetAbsolutePath();
}


wxString wxStandardPathsBase::GetLocalDataDir() const
{
    return GetDataDir();
}


wxString wxStandardPathsBase::GetUserLocalDataDir() const
{
    return GetUserDataDir();
}


wxString wxStandardPathsBase::GetAppDocumentsDir() const
{
    const wxString docsDir = GetDocumentsDir();
    wxString       appDocsDir = AppendAppInfo( docsDir );

    return wxDirExists( appDocsDir ) ? appDocsDir : docsDir;
}


wxString wxStandardPathsBase::GetTempDir() const
{
    return wxFileName::GetTempDir();
}


wxString wxStandardPathsBase::GetUserDir( Dir ) const
{
    return wxFileName::GetHomeDir();
}


wxString wxStandardPathsBase::AppendPathComponent( const wxString& aDir, const wxString& aComponent )
{
    wxString subdir( aDir );

    if( !subdir.empty() && !aComponent.empty() )
    {
        const wxChar ch = *( subdir.end() - 1 );

        if( !wxFileName::IsPathSeparator( ch ) && ch != wxT( '.' ) )
            subdir += wxFileName::GetPathSeparator();

        subdir += aComponent;
    }

    return subdir;
}


wxString wxStandardPathsBase::AppendAppInfo( const wxString& aDir ) const
{
    wxString subdir( aDir );

    if( UsesAppInfo( AppInfo_VendorName ) )
        subdir = AppendPathComponent( subdir, wxTheApp->GetVendorName() );

    if( UsesAppInfo( AppInfo_AppName ) )
        subdir = AppendPathComponent( subdir, wxTheApp->GetAppName() );

    return subdir;
}
