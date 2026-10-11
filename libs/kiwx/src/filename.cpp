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

#include <wx/crt.h>
#include <wx/file.h>
#include <wx/ffile.h>
#include <wx/filename.h>
#include <wx/mstream.h>
#include <wx/process.h>
#include <wx/txtstrm.h>
#include <wx/intl.h>
#include <wx/log.h>
#include <wx/tokenzr.h>
#include <wx/utils.h>

#include "kiwx_private.h"

#include <cwctype>
#include <thread>


namespace
{

/// The length of the "\\?\Volume{guid}\" prefix of the Windows unique volume names.
const size_t MSW_UNIQUE_VOLUME_PREFIX_LENGTH = 49;


bool isDOSPathSep( wxUniChar aCh )
{
    return aCh == wxFILE_SEP_PATH_DOS || aCh == wxFILE_SEP_PATH_UNIX;
}


/// Whether the path looks like a UNC path ("\\server\share").
bool isUNCPath( const wxString& aPath )
{
    return aPath.length() >= 3 && isDOSPathSep( aPath[0] ) && isDOSPathSep( aPath[1] )
           && !isDOSPathSep( aPath[2] );
}


/// Whether the path looks like a volume GUID path ("\\?\Volume{guid}\").
bool isVolumeGUIDPath( const wxString& aPath )
{
    return aPath.length() >= MSW_UNIQUE_VOLUME_PREFIX_LENGTH
           && aPath.StartsWith( wxS( "\\\\?\\Volume{" ) )
           && aPath[MSW_UNIQUE_VOLUME_PREFIX_LENGTH - 1] == wxFILE_SEP_PATH_DOS;
}


/// The volume as it starts a path ("C:", "\\server\share"; nothing for Unix paths).
wxString volumeString( const wxString& aVolume, wxPathFormat aFormat )
{
    wxString path;

    if( !aVolume.empty() && wxFileName::GetFormat( aFormat ) == wxPATH_DOS )
    {
        path = aVolume;

        // no colon after the UNC and volume GUID names, only after the drive letters
        if( aVolume.length() == 1 )
            path += wxFileName::GetVolumeSeparator( aFormat );
    }

    return path;
}

} // namespace


// Free functions on files

bool wxIsAbsolutePath( const wxString& aPath )
{
    if( aPath.empty() )
        return false;

    if( aPath[0] == wxT( '/' ) )
        return true;

#ifdef _WIN32
    // MSDOS like
    if( aPath[0] == wxT( '\\' ) || ( aPath.size() > 1 && std::iswalpha( aPath[0] ) && aPath[1] == wxT( ':' ) ) )
        return true;
#endif

    return false;
}


bool wxEndsWithPathSeparator( const wxString& aPath )
{
    return !aPath.empty() && wxFileName::IsPathSeparator( aPath.Last() );
}


wxString wxPathOnly( const wxString& aPath )
{
    if( aPath.empty() )
        return wxString();

    // Unix or Windows separators, on all the systems (as wxWidgets)
    size_t pos = aPath.find_last_of( wxT( "/\\" ) );

    if( pos != wxString::npos )
        return aPath.Left( pos == 0 ? 1 : pos );

#ifdef _WIN32
    // A drive: "A:junk" gives "A:." (since "A:.\\junk" is not "A:\\junk")
    if( aPath.length() >= 2 && std::iswalpha( aPath[0] ) && aPath[1] == wxT( ':' ) )
        return aPath.Left( 2 ) + wxT( "." );
#endif

    return wxString();
}


wxString wxFileNameFromPath( const wxString& aPath )
{
    return wxFileName( aPath ).GetFullName();
}


bool wxIsWild( const wxString& aPattern )
{
    return aPattern.find_first_of( wxT( "*?[" ) ) != wxString::npos;
}


bool wxMatchWild( const wxString& aPattern, const wxString& aText, bool aDotSpecial )
{
    if( aText.empty() )
        return aPattern.empty() || aPattern == wxT( "*" );

    if( aDotSpecial && aText[0] == wxT( '.' ) && ( aPattern.empty() || aPattern[0] != wxT( '.' ) ) )
        return false;

    return aText.Matches( aPattern );
}


wxFileKind wxGetFileKind( FILE* aFile )
{
    return aFile ? wxGetFileKind( fileno( aFile ) ) : wxFILE_KIND_UNKNOWN;
}


// Environment and system

wxString wxExpandEnvVars( const wxString& aStr )
{
    // The rules of wxWidgets: $VAR, ${VAR}, $(VAR) and, on Windows, %VAR%; unknown variables
    // are kept as they are; a backslash before $ or % suppresses the expansion.
    const std::wstring& str = aStr.impl();
    wxString            result;
    result.reserve( str.size() );

    for( size_t n = 0; n < str.size(); n++ )
    {
        wchar_t c = str[n];

#ifdef _WIN32
        const bool windowsBracket = c == L'%';
#else
        const bool windowsBracket = false;
#endif

        if( c == L'$' || windowsBracket )
        {
            wchar_t bracket = 0;

            if( windowsBracket )
            {
                bracket = L'%';
            }
            else if( n + 1 < str.size() )
            {
                if( str[n + 1] == L'(' )
                {
                    bracket = L')';
                    n++;
                }
                else if( str[n + 1] == L'{' )
                {
                    bracket = L'}';
                    n++;
                }
            }

            size_t m = n + 1;

            while( m < str.size() && ( std::iswalnum( str[m] ) || str[m] == L'_' ) )
                m++;

            wxString varName( str.substr( n + 1, m - n - 1 ) );
            wxString value;
            bool     found = !varName.empty() && wxGetEnv( varName, &value );

            if( found )
            {
                result += value;
            }
            else
            {
                if( bracket && !windowsBracket )
                    result += str[n - 1];

                result += str[n];
                result += varName;
            }

            if( bracket )
            {
                if( m == str.size() || str[m] != bracket )
                {
                    // not on Windows, where '%' is common in the values (as wxWidgets)
#ifndef _WIN32
                    wxLogWarning( _( "Environment variables expansion failed: missing '%c' at "
                                     "position %u in '%s'." ),
                                  (char) bracket, (unsigned int) ( m + 1 ), aStr );
#endif
                }
                else
                {
                    if( !found )
                        result += bracket;

                    m++;
                }
            }

            n = m - 1;
            continue;
        }

        if( c == L'\\' && n + 1 < str.size() && ( str[n + 1] == L'%' || str[n + 1] == L'$' ) )
        {
            result += str[++n];
            continue;
        }

        result += c;
    }

    return result;
}


bool wxGetUserId( wxChar* aBuf, int aSize )
{
    wxString id = wxGetUserId();

    if( id.empty() || aSize <= 0 )
        return false;

    std::wcsncpy( aBuf, id.wc_str(), aSize - 1 );
    aBuf[aSize - 1] = 0;
    return true;
}


wxString wxGetNativeCpuArchitectureName()
{
    return wxGetCpuArchitectureName();
}


bool wxIsPlatform64Bit()
{
    return sizeof( void* ) == 8;
}


void wxMilliSleep( unsigned long aMs )
{
    std::this_thread::sleep_for( std::chrono::milliseconds( aMs ) );
}


void wxMicroSleep( unsigned long aUs )
{
    std::this_thread::sleep_for( std::chrono::microseconds( aUs ) );
}


void wxSleep( int aSeconds )
{
    std::this_thread::sleep_for( std::chrono::seconds( aSeconds ) );
}


// wxPathList

bool wxPathList::Add( const wxString& aPath )
{
    // as a directory, normalized without wxPATH_NORM_DOTS (paths starting with "..")
    wxFileName fn( aPath + wxFileName::GetPathSeparator() );

    if( !fn.Normalize( wxPATH_NORM_TILDE | wxPATH_NORM_LONG | wxPATH_NORM_ENV_VARS ) )
        return false;

    wxString toadd = fn.GetPath();

    if( Index( toadd ) == wxNOT_FOUND )
        wxArrayString::Add( toadd );

    return true;
}


void wxPathList::Add( const wxArrayString& aArr )
{
    for( size_t j = 0; j < aArr.GetCount(); j++ )
        Add( aArr[j] );
}


void wxPathList::AddEnvList( const wxString& aEnvVariable )
{
    wxString val;

    if( wxGetEnv( aEnvVariable, &val ) )
    {
        wxArrayString arr = wxStringTokenize( val, wxT( ":;" ) );

        for( const wxString& item : arr )
            wxArrayString::Add( item );
    }
}


bool wxPathList::EnsureFileAccessible( const wxString& aPath )
{
    return Add( wxPathOnly( aPath ) );
}


wxString wxPathList::FindValidPath( const wxString& aFile ) const
{
    wxFileName fn( aFile );
    wxString   strend;

    if( !fn.Normalize( wxPATH_NORM_TILDE | wxPATH_NORM_LONG | wxPATH_NORM_ENV_VARS ) )
        return wxEmptyString;

    wxASSERT_MSG( !fn.IsDir(), wxT( "Cannot search for directories; only for files" ) );

    // an absolute path: only its name is searched
    if( fn.IsAbsolute() )
        strend = fn.GetFullName();
    else
        strend = fn.GetFullPath();

    for( size_t i = 0; i < GetCount(); i++ )
    {
        wxString strstart = Item( i );

        if( !strstart.IsEmpty() && strstart.Last() != wxFileName::GetPathSeparator() )
            strstart += wxFileName::GetPathSeparator();

        if( wxFileExists( strstart + strend ) )
            return strstart + strend;
    }

    return wxEmptyString;
}


wxString wxPathList::FindAbsoluteValidPath( const wxString& aFile ) const
{
    wxString f = FindValidPath( aFile );

    if( f.empty() || wxIsAbsolutePath( f ) )
        return f;

    wxString buf = ::wxGetCwd();

    if( !wxEndsWithPathSeparator( buf ) )
        buf += wxFILE_SEP_PATH;

    return buf + f;
}


wxKillError wxProcess::Kill( int aPid, wxSignal aSig, int aFlags )
{
    wxKillError rc;
    wxKill( aPid, aSig, &rc, aFlags );
    return rc;
}


bool wxProcess::Exists( int aPid )
{
    switch( Kill( aPid, wxSIGNONE ) )
    {
    case wxKILL_OK:
    case wxKILL_ACCESS_DENIED: return true;
    default:                   return false;
    }
}


// wxFileName

void wxFileName::Clear()
{
    m_dirs.clear();
    m_volume.clear();
    m_name.clear();
    m_ext.clear();
    m_relative = true;
    m_hasExt = false;
    m_dontFollowLinks = false;
}


wxPathFormat wxFileName::GetFormat( wxPathFormat aFormat )
{
    if( aFormat == wxPATH_NATIVE )
    {
#ifdef _WIN32
        aFormat = wxPATH_DOS;
#else
        aFormat = wxPATH_UNIX;
#endif
    }

    return aFormat;
}


bool wxFileName::IsCaseSensitive( wxPathFormat aFormat )
{
    // only Unix file names are truly case-sensitive
    return GetFormat( aFormat ) == wxPATH_UNIX;
}


wxString wxFileName::GetVolumeSeparator( wxPathFormat aFormat )
{
    return GetFormat( aFormat ) == wxPATH_DOS ? wxString( wxFILE_SEP_DSK ) : wxString();
}


bool wxFileName::IsMSWUniqueVolumeNamePath( const wxString& aPath, wxPathFormat aFormat )
{
    return GetFormat( aFormat ) == wxPATH_DOS && isVolumeGUIDPath( aPath );
}


wxUniChar wxFileName::GetPathSeparator( wxPathFormat aFormat )
{
    return GetFormat( aFormat ) == wxPATH_DOS ? wxT( '\\' ) : wxT( '/' );
}


wxString wxFileName::GetPathSeparators( wxPathFormat aFormat )
{
    // DOS: both, as the Windows API, the native one first (the one of GetFullPath())
    return GetFormat( aFormat ) == wxPATH_DOS ? wxT( "\\/" ) : wxT( "/" );
}


bool wxFileName::IsPathSeparator( wxUniChar aCh, wxPathFormat aFormat )
{
    return aCh != 0 && GetPathSeparators( aFormat ).Find( aCh ) != wxNOT_FOUND;
}


wxString wxFileName::GetForbiddenChars( wxPathFormat aFormat )
{
    wxString chars = wxT( "*?" );

    if( GetFormat( aFormat ) == wxPATH_DOS )
        chars += wxT( "\\/:\"<>|" );

    return chars;
}


void wxFileName::SplitVolume( const wxString& aFullPath, wxString* aVolume, wxString* aPath,
                              wxPathFormat aFormat )
{
    wxString pathOnly = aFullPath;

    if( aVolume )
        aVolume->clear();

    // No volumes in the Unix paths
    if( GetFormat( aFormat ) == wxPATH_DOS )
    {
        if( isVolumeGUIDPath( aFullPath ) )
        {
            if( aVolume )
                *aVolume = aFullPath.Left( MSW_UNIQUE_VOLUME_PREFIX_LENGTH - 1 );

            // the first slash stays in the path
            pathOnly = aFullPath.Mid( MSW_UNIQUE_VOLUME_PREFIX_LENGTH - 1 );
        }
        else if( isUNCPath( aFullPath ) )
        {
            // isUNCPath() checks that the third character is not a separator
            size_t posFirstSlash = aFullPath.find_first_of( GetPathTerminators( aFormat ), 3 );

            if( posFirstSlash != wxString::npos )
            {
                if( aVolume )
                    *aVolume = aFullPath.Left( posFirstSlash );

                pathOnly = aFullPath.Mid( posFirstSlash );
            }
            else if( aVolume )
            {
                // the root of the share ("\\\\share")
                *aVolume = aFullPath;
            }

            // backslashes are the canonical separators of the UNC paths
            if( aVolume )
            {
                ( *aVolume )[0] = wxT( '\\' );
                ( *aVolume )[1] = wxT( '\\' );
            }

            pathOnly = posFirstSlash != wxString::npos ? pathOnly : wxString();
        }
        else
        {
            // a colon at the very start is not a volume separator
            size_t posFirstColon = aFullPath.find_first_of( GetVolumeSeparator( aFormat ) );

            if( posFirstColon && posFirstColon != wxString::npos )
            {
                if( aVolume )
                    *aVolume = aFullPath.Left( posFirstColon );

                pathOnly = aFullPath.Mid( posFirstColon + 1 );
            }
        }
    }

    if( aPath )
        *aPath = pathOnly;
}


void wxFileName::SplitPath( const wxString& aFullPathWithVolume, wxString* aVolume, wxString* aPath,
                            wxString* aName, wxString* aExt, bool* aHasExt, wxPathFormat aFormat )
{
    wxString fullPath;
    SplitVolume( aFullPathWithVolume, aVolume, &fullPath, aFormat );

    size_t posLastSlash = fullPath.find_last_of( GetPathTerminators( aFormat ) );
    size_t posLastDot = fullPath.find_last_of( wxFILE_SEP_EXT );

    // A dot at the start of the name is not an extension (hidden files).
    if( posLastDot != wxString::npos
        && ( posLastDot == 0 || IsPathSeparator( fullPath[posLastDot - 1], aFormat ) ) )
    {
        posLastDot = wxString::npos;
    }

    // The dot must be in the name, not in the path.
    if( posLastDot != wxString::npos && posLastSlash != wxString::npos && posLastDot < posLastSlash )
        posLastDot = wxString::npos;

    if( aPath )
    {
        if( posLastSlash == wxString::npos )
        {
            aPath->clear();
        }
        else
        {
            // The files of the root directory have the path "/", not "".
            size_t len = posLastSlash;

            if( !len )
                len++;

            *aPath = fullPath.Left( len );
        }
    }

    if( aName )
    {
        size_t nStart = posLastSlash == wxString::npos ? 0 : posLastSlash + 1;
        size_t count = posLastDot == wxString::npos ? wxString::npos : posLastDot - nStart;

        *aName = fullPath.Mid( nStart, count );
    }

    if( posLastDot == wxString::npos )
    {
        if( aExt )
            aExt->clear();

        if( aHasExt )
            *aHasExt = false;
    }
    else
    {
        if( aExt )
            *aExt = fullPath.Mid( posLastDot + 1 );

        if( aHasExt )
            *aHasExt = true;
    }
}


void wxFileName::SplitPath( const wxString& aFullPath, wxString* aPath, wxString* aName,
                            wxString* aExt, wxPathFormat aFormat )
{
    wxString volume;
    SplitPath( aFullPath, &volume, aPath, aName, aExt, nullptr, aFormat );

    if( aPath )
        aPath->Prepend( volumeString( volume, aFormat ) );
}


wxString wxFileName::StripExtension( const wxString& aFullPath )
{
    wxFileName fn( aFullPath );
    fn.ClearExt();
    return fn.GetFullPath();
}


void wxFileName::Assign( const wxString& aFullPath, wxPathFormat aFormat )
{
    wxString volume, path, name, ext;
    bool     hasExt;
    SplitPath( aFullPath, &volume, &path, &name, &ext, &hasExt, aFormat );
    Assign( volume, path, name, ext, hasExt, aFormat );
}


void wxFileName::Assign( const wxString& aVolume, const wxString& aPath, const wxString& aName,
                         const wxString& aExt, bool aHasExt, wxPathFormat aFormat )
{
    SetPath( aPath, aFormat );

    m_volume = aVolume;
    m_ext = aExt;
    m_name = aName;
    m_hasExt = aHasExt;
}


void wxFileName::Assign( const wxString& aPathOrig, const wxString& aFullName, wxPathFormat aFormat )
{
    // The path is a directory even without a trailing separator.
    wxString fullPath = aPathOrig;

    if( !fullPath.empty() && !wxEndsWithPathSeparator( fullPath ) )
        fullPath += GetPathSeparator( aFormat );

    wxString volume, path, name, ext;
    wxString volDummy, pathDummy, nameDummy, extDummy;
    bool     hasExt;

    SplitPath( aFullName, &volDummy, &pathDummy, &name, &ext, &hasExt, aFormat );
    SplitPath( fullPath, &volume, &path, &nameDummy, &extDummy, nullptr, aFormat );

    Assign( volume, path, name, ext, hasExt, aFormat );
}


void wxFileName::Assign( const wxString& aPathOrig, const wxString& aName, const wxString& aExt,
                         wxPathFormat aFormat )
{
    wxString volume, path;
    SplitVolume( aPathOrig, &volume, &path, aFormat );
    Assign( volume, path, aName, aExt, aFormat );
}


void wxFileName::AssignCwd( const wxString& aVolume )
{
    AssignDir( GetCwd( aVolume ) );
}


void wxFileName::AssignHomeDir()
{
    AssignDir( GetHomeDir() );
}


void wxFileName::AssignTempFileName( const wxString& aPrefix )
{
    wxString tempName = CreateTempFileName( aPrefix );

    if( tempName.empty() )
        Clear();
    else
        Assign( tempName );
}


void wxFileName::AssignTempFileName( const wxString& aPrefix, wxFile* aFileTemp )
{
    wxString tempName = CreateTempFileName( aPrefix, aFileTemp );

    if( tempName.empty() )
        Clear();
    else
        Assign( tempName );
}


void wxFileName::AssignTempFileName( const wxString& aPrefix, wxFFile* aFileTemp )
{
    wxString tempName = CreateTempFileName( aPrefix, aFileTemp );

    if( tempName.empty() )
        Clear();
    else
        Assign( tempName );
}


void wxFileName::SetPath( const wxString& aPathOrig, wxPathFormat aFormat )
{
    m_dirs.clear();

    if( aPathOrig.empty() )
    {
        m_relative = true;
        return;
    }

    wxString volume, path;
    SplitVolume( aPathOrig, &volume, &path, aFormat );

    if( !volume.empty() )
    {
        m_relative = false;
        SetVolume( volume );
    }

    if( path.empty() )
        return;

    if( GetFormat( aFormat ) == wxPATH_DOS )
        m_relative = !IsPathSeparator( path[0], aFormat );
    else
        m_relative = path[0] != wxT( '/' );

    // Empty components (a//b) are ignored; "/" alone gives no components and m_relative false.
    wxStringTokenizer tn( path, GetPathSeparators( aFormat ) );

    while( tn.HasMoreTokens() )
    {
        wxString token = tn.GetNextToken();

        if( !token.empty() )
            m_dirs.Add( token );
    }
}


wxString wxFileName::GetPath( int aFlags, wxPathFormat aFormat ) const
{
    wxString fullPath;

    if( aFlags & wxPATH_GET_VOLUME )
        fullPath += volumeString( m_volume, aFormat );

    if( !m_relative )
        fullPath += GetPathSeparator( aFormat );

    const size_t dirCount = m_dirs.size();

    for( size_t i = 0; i < dirCount; i++ )
    {
        fullPath += m_dirs[i];

        if( ( aFlags & wxPATH_GET_SEPARATOR ) || i != dirCount - 1 )
            fullPath += GetPathSeparator( aFormat );
    }

    return fullPath;
}


wxString wxFileName::GetFullPath( wxPathFormat aFormat ) const
{
    return GetPath( wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR, aFormat ) + GetFullName();
}


void wxFileName::SetFullName( const wxString& aFullName )
{
    SplitPath( aFullName, nullptr, nullptr, &m_name, &m_ext, &m_hasExt );
}


wxString wxFileName::GetFullName() const
{
    wxString fullName = m_name;

    if( m_hasExt )
        fullName << wxFILE_SEP_EXT << m_ext;

    return fullName;
}


bool wxFileName::IsAbsolute( wxPathFormat aFormat ) const
{
    // the Unix paths starting with ~ are absolute (only with wxPATH_UNIX given explicitly, as
    // in wxWidgets)
    if( aFormat == wxPATH_UNIX && !m_dirs.empty() && !m_dirs[0].empty()
        && m_dirs[0][0] == wxT( '~' ) )
    {
        return true;
    }

    if( m_relative )
        return false;

    // where there are volumes an absolute path has one
    if( !GetVolumeSeparator( aFormat ).empty() && GetVolume().empty() )
        return false;

    return true;
}


bool wxFileName::IsValidDirComponent( const wxString& aDir )
{
    if( aDir.empty() )
        return false;

    for( wxUniChar c : aDir )
    {
        if( IsPathSeparator( c ) || ( c == wxFILE_SEP_DSK && !GetVolumeSeparator().empty() ) )
            return false;
    }

    return true;
}


bool wxFileName::AppendDir( const wxString& aDir )
{
    if( !IsValidDirComponent( aDir ) )
        return false;

    m_dirs.Add( aDir );
    return true;
}


bool wxFileName::InsertDir( size_t aBefore, const wxString& aDir )
{
    if( !IsValidDirComponent( aDir ) )
        return false;

    m_dirs.Insert( aDir, aBefore );
    return true;
}


bool wxFileName::Normalize( int aFlags, const wxString& aCwd, wxPathFormat aFormat )
{
    // Environment variables first: they may change the whole path.
    if( aFlags & wxPATH_NORM_ENV_VARS )
    {
        wxString pathOrig = GetFullPath( aFormat );
        wxString path = wxExpandEnvVars( pathOrig );

        if( path != pathOrig )
            Assign( path );
    }

    wxArrayString dirs = GetDirs();
    wxFileName    curDir;

    if( ( aFlags & wxPATH_NORM_ABSOLUTE ) && !IsAbsolute( aFormat ) )
    {
        if( aCwd.empty() )
            curDir.AssignCwd( GetVolume() );
        else
            curDir.AssignDir( aCwd );
    }

    aFormat = GetFormat( aFormat );

    if( aFormat == wxPATH_UNIX && ( aFlags & wxPATH_NORM_TILDE ) && m_relative && !dirs.empty() )
    {
        wxString dir = dirs[0];

        if( !dir.empty() && dir[0] == wxT( '~' ) )
        {
            curDir.AssignDir( wxGetUserHome( dir.Mid( 1 ) ) );
            dirs.RemoveAt( 0 );
        }
    }

    if( curDir.IsOk() )
    {
        if( !HasVolume() && curDir.HasVolume() )
        {
            SetVolume( curDir.GetVolume() );

            if( !m_relative )
                curDir.Clear();
        }

        wxArrayString dirsNew = curDir.GetDirs();
        dirs.insert( dirs.begin(), dirsNew.begin(), dirsNew.end() );

        if( !curDir.m_relative )
            m_relative = false;
    }

    m_dirs.clear();

    for( const wxString& dir : dirs )
    {
        if( aFlags & wxPATH_NORM_DOTS )
        {
            if( dir == wxT( "." ) )
                continue;

            if( dir == wxT( ".." ) )
            {
                if( m_dirs.empty() )
                {
                    // More ".." than components: dropped for absolute paths ("/.." is "/"),
                    // kept for relative ones.
                    if( !m_relative )
                        continue;
                }
                else if( m_dirs.Last() != wxT( ".." ) )
                {
                    m_dirs.pop_back();
                    continue;
                }
            }
        }

        m_dirs.Add( dir );
    }

#ifdef _WIN32
    if( ( aFlags & wxPATH_NORM_LONG ) && aFormat == wxPATH_DOS )
        Assign( GetLongPath() );
#endif

    // The case last, once the path does not change any more
    if( ( aFlags & wxPATH_NORM_CASE ) && !IsCaseSensitive( aFormat ) )
    {
        m_volume.MakeLower();
        m_name.MakeLower();
        m_ext.MakeLower();

        for( wxString& dir : m_dirs )
            dir.MakeLower();
    }

    return true;
}


bool wxFileName::MakeRelativeTo( const wxString& aPathBase, wxPathFormat aFormat )
{
    wxFileName fnBase = wxFileName::DirName( aPathBase, aFormat );
    wxString   cwd = wxGetCwd();

    const int normFlags = wxPATH_NORM_ALL & ~( wxPATH_NORM_CASE | wxPATH_NORM_SHORTCUT );
    Normalize( normFlags, cwd, aFormat );
    fnBase.Normalize( normFlags, cwd, aFormat );

    bool withCase = IsCaseSensitive( aFormat );

    if( !GetVolume().IsSameAs( fnBase.GetVolume(), withCase ) )
        return false;

    m_volume.clear();

    while( !m_dirs.empty() && !fnBase.m_dirs.empty()
           && m_dirs[0].IsSameAs( fnBase.m_dirs[0], withCase ) )
    {
        m_dirs.RemoveAt( 0 );
        fnBase.m_dirs.RemoveAt( 0 );
    }

    for( size_t i = 0; i < fnBase.m_dirs.size(); i++ )
        m_dirs.Insert( wxT( ".." ), 0 );

    // A directory relative to itself is ".".
    if( m_dirs.empty() && IsDir() )
        m_dirs.Add( wxT( "." ) );

    m_relative = true;
    return true;
}


bool wxFileName::ReplaceEnvVariable( const wxString& aEnvName, const wxString& aReplacementFmt,
                                     wxPathFormat aFormat )
{
    wxString value;

    if( !wxGetEnv( aEnvName, &value ) || value.empty() )
        return false;

    wxString replacement = wxString::Format( aReplacementFmt, aEnvName );
    wxString path = GetPath( wxPATH_GET_VOLUME, aFormat );

    if( !path.Replace( value, replacement, false ) )
        return false;

    SetPath( path, aFormat );
    return true;
}


bool wxFileName::ReplaceHomeDir( wxPathFormat aFormat )
{
    wxString home = wxGetHomeDir();
    wxString path = GetPath( wxPATH_GET_VOLUME, aFormat );

    if( home.empty() || !path.StartsWith( home ) )
        return false;

    path = wxT( "~" ) + path.Mid( home.length() );
    SetPath( path, aFormat );
    return true;
}


bool wxFileName::SameAs( const wxFileName& aOther, wxPathFormat aFormat ) const
{
    wxFileName fn1 = *this;
    wxFileName fn2 = aOther;

    wxString cwd = wxGetCwd();
    // the case only matters where the file names are case sensitive
    const int flags = ( wxPATH_NORM_ALL | wxPATH_NORM_CASE ) & ~wxPATH_NORM_SHORTCUT;
    fn1.Normalize( flags, cwd, aFormat );
    fn2.Normalize( flags, cwd, aFormat );

    return fn1.GetFullPath( aFormat ) == fn2.GetFullPath( aFormat );
}


bool wxFileName::FileExists( const wxString& aFile )
{
    return wxFileExists( aFile );
}


bool wxFileName::DirExists( const wxString& aDir )
{
    return wxDirExists( aDir );
}


wxDateTime wxFileName::GetModificationTime() const
{
    wxDateTime mod;
    GetTimes( nullptr, &mod, nullptr );
    return mod;
}


wxDateTime wxFileName::GetAccessTime() const
{
    wxDateTime access;
    GetTimes( &access, nullptr, nullptr );
    return access;
}


wxString wxFileName::GetCwd( const wxString& )
{
    return wxGetCwd();
}


bool wxFileName::SetCwd( const wxString& aCwd )
{
    return wxSetWorkingDirectory( aCwd );
}


wxString wxFileName::GetHomeDir()
{
    return wxGetHomeDir();
}


bool wxFileName::Mkdir( const wxString& aDir, int aPerm, int aFlags )
{
    if( aFlags & wxPATH_MKDIR_FULL )
    {
        wxFileName fn;
        fn.AssignDir( aDir );

        wxString currPath = volumeString( fn.GetVolume(), wxPATH_NATIVE );
        const wxArrayString& dirs = fn.GetDirs();

        for( size_t i = 0; i < dirs.size(); i++ )
        {
            if( i > 0 || fn.IsAbsolute() )
                currPath += wxFILE_SEP_PATH;

            currPath += dirs[i];

            if( !wxDirExists( currPath ) && !wxMkdir( currPath, aPerm ) )
                return false;
        }

        return true;
    }

    return wxMkdir( aDir, aPerm );
}


wxULongLong wxFileName::GetSize( const wxString& aFile )
{
    long long size = kiwxFileSize( aFile );
    return size < 0 ? wxInvalidSize : wxULongLong( static_cast<unsigned long long>( size ) );
}


wxString wxFileName::GetHumanReadableSize( const wxULongLong& aSize, const wxString& aNullSize,
                                           int aPrecision, wxSizeConvention aConv )
{
    if( aSize.GetValue() == 0 || aSize.GetValue() == wxInvalidSize.GetValue() )
        return aNullSize;

    double   multiplier = 1024.0;
    wxString infix;

    if( aConv == wxSIZE_CONV_IEC )
        infix = wxT( "i" );
    else if( aConv == wxSIZE_CONV_SI )
        multiplier = 1000.0;

    double         size = aSize.ToDouble();
    const wchar_t* units[] = { L"K", L"M", L"G", L"T", L"P", L"E", L"Z", L"Y" };

    if( size < multiplier )
        return wxString::Format( wxT( "%s B" ), aSize.ToString() );

    double scale = multiplier;
    int    unit = 0;

    while( unit < 7 && size >= scale * multiplier )
    {
        scale *= multiplier;
        unit++;
    }

    return wxString::Format( wxT( "%.*f %s%sB" ), aPrecision, size / scale, units[unit], infix );
}
