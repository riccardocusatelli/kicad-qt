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

/**
 * @file stdpaths_unix.cpp
 *
 * wxStandardPaths on Unix (the classic and the XDG layouts).
 */

#include <wx/stdpaths.h>

#include <wx/app.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/textfile.h>
#include <wx/tokenzr.h>
#include <wx/utils.h>

#include <cstdlib>
#include <cstring>
#include <unistd.h>


namespace
{

wxString getXDGConfigHome()
{
    wxString dir;

    if( !wxGetEnv( wxS( "XDG_CONFIG_HOME" ), &dir ) || dir.empty() )
        dir = wxFileName::GetHomeDir() + wxS( "/.config" );

    return dir;
}

} // namespace


// wxStandardPaths

wxString wxStandardPaths::GetUserConfigDir() const
{
    wxString dir;

    switch( GetFileLayout() )
    {
    case FileLayout_Classic: dir = wxFileName::GetHomeDir(); break;
    case FileLayout_XDG:     dir = getXDGConfigHome(); break;
    }

    wxASSERT_MSG( !dir.empty(), wxS( "unsupported file layout" ) );
    return dir;
}


wxString wxStandardPaths::GetExecutablePath() const
{
    wxString exeStr;
    char     buf[4096];
    int      result = readlink( "/proc/self/exe", buf, sizeof( buf ) - 1 );

    if( result != -1 )
    {
        buf[result] = '\0';

        if( std::strcmp( buf, "(deleted)" ) != 0 )
            exeStr = wxString( buf, wxConvLibc );
    }

    // UPX keeps the executable path there
    if( exeStr.empty() )
        wxGetEnv( wxT( "   " ), &exeStr );

    if( !exeStr.empty() )
        return exeStr;

    return wxStandardPathsBase::GetExecutablePath();
}


void wxStandardPaths::DetectPrefix()
{
    wxString exeStr = GetExecutablePath();

    if( !exeStr.empty() )
    {
        // the executable is in the last "bin" directory of the prefix
        size_t pos = exeStr.rfind( wxT( "/bin/" ) );

        if( pos != wxString::npos )
            m_prefix.assign( exeStr, 0, pos );
    }

    if( m_prefix.empty() )
        m_prefix = wxT( "/usr/local" );
}


wxString wxStandardPaths::GetInstallPrefix() const
{
    if( m_prefix.empty() )
        const_cast<wxStandardPaths*>( this )->DetectPrefix();

    return m_prefix;
}


wxString wxStandardPaths::GetConfigDir() const
{
    return wxT( "/etc" );
}


wxString wxStandardPaths::GetDataDir() const
{
    // WX_APPNAME_DATA_DIR overrides it (read once, as wxWidgets)
    static const wxString envOverride(
            std::getenv( ( "WX_" + wxTheApp->GetAppName().Upper() + "_DATA_DIR" ).c_str() ) );

    if( !envOverride.empty() )
        return envOverride;

    return AppendAppInfo( GetInstallPrefix() + wxT( "/share" ) );
}


wxString wxStandardPaths::GetLocalDataDir() const
{
    return AppendAppInfo( wxT( "/etc" ) );
}


wxString wxStandardPaths::GetUserDataDir() const
{
    return AppendAppInfo( wxFileName::GetHomeDir() + wxT( "/." ) );
}


wxString wxStandardPaths::GetPluginsDir() const
{
    return AppendAppInfo( GetInstallPrefix() + wxT( "/lib" ) );
}


wxString wxStandardPaths::GetLocalizedResourcesDir( const wxString& aLang,
                                                    ResourceCat aCategory ) const
{
    if( aCategory != ResourceCat_Messages )
        return wxStandardPathsBase::GetLocalizedResourcesDir( aLang, aCategory );

    return GetInstallPrefix() + wxT( "/share/locale/" ) + aLang + wxT( "/LC_MESSAGES" );
}


wxString wxStandardPaths::GetUserDir( Dir aUserDir ) const
{
    // the XDG convention is followed whatever the file layout
    wxLogNull      logNull;
    const wxString homeDir = wxFileName::GetHomeDir();

    if( aUserDir == Dir_Cache )
    {
        wxString cacheDir;

        if( !wxGetEnv( wxS( "XDG_CACHE_HOME" ), &cacheDir ) )
            cacheDir = homeDir + wxS( "/.cache" );

        return cacheDir;
    }

    const wxFileName dirsFile( getXDGConfigHome(), wxS( "user-dirs.dirs" ) );

    if( dirsFile.FileExists() )
    {
        wxString userDirId;

        switch( aUserDir )
        {
        case Dir_Desktop:   userDirId = "XDG_DESKTOP_DIR"; break;
        case Dir_Downloads: userDirId = "XDG_DOWNLOAD_DIR"; break;
        case Dir_Music:     userDirId = "XDG_MUSIC_DIR"; break;
        case Dir_Pictures:  userDirId = "XDG_PICTURES_DIR"; break;
        case Dir_Videos:    userDirId = "XDG_VIDEOS_DIR"; break;
        default:            userDirId = "XDG_DOCUMENTS_DIR"; break;
        }

        wxTextFile textFile;

        if( textFile.Open( dirsFile.GetFullPath() ) )
        {
            for( wxString line = textFile.GetFirstLine(); !textFile.Eof();
                 line = textFile.GetNextLine() )
            {
                int pos = line.Find( userDirId );

                if( pos != wxNOT_FOUND )
                {
                    wxString value = line.AfterFirst( wxT( '=' ) );
                    value.Replace( wxT( "$HOME" ), homeDir );
                    value.Trim( true );
                    value.Trim( false );
                    value.Replace( "\"", "", true );

                    if( !value.IsEmpty() && wxDirExists( value ) )
                        return value;
                    else
                        break;
                }
            }
        }
    }

    return wxStandardPathsBase::GetUserDir( aUserDir );
}


wxString wxStandardPaths::MakeConfigFileName( const wxString& aBasename, ConfigFileConv aConv ) const
{
    wxFileName fn( wxEmptyString, aBasename );
    bool       addExt = false;

    switch( GetFileLayout() )
    {
    case FileLayout_Classic:
        switch( aConv )
        {
        case ConfigFileConv_Dot: fn.SetName( wxT( '.' ) + fn.GetName() ); break;
        case ConfigFileConv_Ext: addExt = true; break;
        }

        break;

    case FileLayout_XDG:
        addExt = true;
        break;
    }

    if( addExt )
        fn.SetExt( wxS( "conf" ) );

    return fn.GetFullName();
}
