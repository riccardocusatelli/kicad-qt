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

/**
 * @file stdpaths_msw.cpp
 *
 * wxStandardPaths on Windows, as wxWidgets 3.2.4 (src/msw/stdpaths.cpp): the known folders of
 * the shell, and the directory of the program for its data and plugins.
 */

#include <wx/stdpaths.h>

#include <wx/filename.h>

#include <windows.h>
#include <knownfolders.h>
#include <shlobj.h>


namespace
{

wxString knownFolder( REFKNOWNFOLDERID aId )
{
    wxString dir;
    PWSTR    path = nullptr;

    if( SUCCEEDED( SHGetKnownFolderPath( aId, KF_FLAG_DONT_VERIFY, nullptr, &path ) ) )
        dir = path;

    CoTaskMemFree( path );
    return dir;
}

} // namespace


wxStandardPaths::wxStandardPaths()
{
    // the programs run from their build directory too
    IgnoreAppBuildSubDirs();
}


wxString wxStandardPaths::GetAppDir() const
{
    if( m_appDir.empty() )
        m_appDir = wxFileName( GetExecutablePath() ).GetPath();

    return m_appDir;
}


void wxStandardPaths::IgnoreAppSubDir( const wxString& aSubdirPattern )
{
    wxFileName fn = wxFileName::DirName( GetAppDir() );

    if( !fn.GetDirCount() )
        return;

    if( fn.GetDirs().Last().Lower().Matches( aSubdirPattern.Lower() ) )
    {
        fn.RemoveLastDir();
        m_appDir = fn.GetPath();
    }
}


void wxStandardPaths::IgnoreAppBuildSubDirs()
{
    IgnoreAppSubDir( wxT( "debug" ) );
    IgnoreAppSubDir( wxT( "release" ) );

    // the architecture directories
#ifdef _WIN64
    IgnoreAppSubDir( wxT( "Win64" ) );
    IgnoreAppSubDir( wxT( "x64" ) );
    IgnoreAppSubDir( wxT( "x86_64" ) );
    IgnoreAppSubDir( wxT( "ARM64" ) );
#else
    IgnoreAppSubDir( wxT( "Win32" ) );
    IgnoreAppSubDir( wxT( "x86" ) );
#endif
}


void wxStandardPaths::DontIgnoreAppSubDir()
{
    m_appDir.clear();
}


wxString wxStandardPaths::GetExecutablePath() const
{
    std::wstring buf( MAX_PATH, L'\0' );

    for( ;; )
    {
        DWORD len = GetModuleFileNameW( nullptr, buf.data(), static_cast<DWORD>( buf.size() ) );

        if( len < buf.size() )
        {
            buf.resize( len );
            return wxString( buf );
        }

        buf.resize( buf.size() * 2 );
    }
}


wxString wxStandardPaths::GetConfigDir() const
{
    return AppendAppInfo( knownFolder( FOLDERID_ProgramData ) );
}


wxString wxStandardPaths::GetUserConfigDir() const
{
    return knownFolder( FOLDERID_RoamingAppData );
}


wxString wxStandardPaths::GetDataDir() const
{
    // each program is installed in its own directory, with its data
    return GetAppDir();
}


wxString wxStandardPaths::GetUserDataDir() const
{
    return AppendAppInfo( GetUserConfigDir() );
}


wxString wxStandardPaths::GetUserLocalDataDir() const
{
    return AppendAppInfo( knownFolder( FOLDERID_LocalAppData ) );
}


wxString wxStandardPaths::GetPluginsDir() const
{
    return GetAppDir();
}


wxString wxStandardPaths::GetUserDir( Dir aUserDir ) const
{
    switch( aUserDir )
    {
    case Dir_Cache:     return knownFolder( FOLDERID_LocalAppData );
    case Dir_Desktop:   return knownFolder( FOLDERID_Desktop );
    case Dir_Downloads: return knownFolder( FOLDERID_Downloads );
    case Dir_Music:     return knownFolder( FOLDERID_Music );
    case Dir_Pictures:  return knownFolder( FOLDERID_Pictures );
    case Dir_Videos:    return knownFolder( FOLDERID_Videos );
    default:            return knownFolder( FOLDERID_Documents );
    }
}


wxString wxStandardPaths::MakeConfigFileName( const wxString& aBasename, ConfigFileConv ) const
{
    wxFileName fn( wxEmptyString, aBasename );
    fn.SetExt( wxT( "ini" ) );
    return fn.GetFullName();
}
