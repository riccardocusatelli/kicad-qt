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

#include <wx/arrstr.h>
#include <wx/longlong.h>
#include <wx/string.h>

#include <ctime>
#include <sys/stat.h>

#define wxFILE_SEP_EXT        wxT( '.' )
#define wxFILE_SEP_DSK        wxT( ':' )
#define wxFILE_SEP_PATH_DOS   wxT( '\\' )
#define wxFILE_SEP_PATH_UNIX  wxT( '/' )
#define wxFILE_SEP_PATH_MAC   wxT( ':' )
#define wxFILE_SEP_PATH_VMS   wxT( '.' )
#define wxPATH_SEP_DOS        wxT( ";" )
#define wxPATH_SEP_UNIX       wxT( ":" )

#ifdef _WIN32
#define wxFILE_SEP_PATH       wxFILE_SEP_PATH_DOS
#define wxPATH_SEP            wxPATH_SEP_DOS
// Windows accepts both separators
#define wxIsPathSeparator( c ) ( ( c ) == wxFILE_SEP_PATH_DOS || ( c ) == wxFILE_SEP_PATH_UNIX )
#define wxARE_FILENAMES_CASE_SENSITIVE false
#else
#define wxFILE_SEP_PATH       wxFILE_SEP_PATH_UNIX
#define wxPATH_SEP            wxPATH_SEP_UNIX
#define wxIsPathSeparator( c ) ( ( c ) == wxFILE_SEP_PATH )
#define wxARE_FILENAMES_CASE_SENSITIVE true
#endif

enum wxPosixPermissions
{
    wxS_IRUSR = 00400,
    wxS_IWUSR = 00200,
    wxS_IXUSR = 00100,
    wxS_IRGRP = 00040,
    wxS_IWGRP = 00020,
    wxS_IXGRP = 00010,
    wxS_IROTH = 00004,
    wxS_IWOTH = 00002,
    wxS_IXOTH = 00001,

    wxPOSIX_USER_READ = wxS_IRUSR,
    wxPOSIX_USER_WRITE = wxS_IWUSR,
    wxPOSIX_USER_EXECUTE = wxS_IXUSR,
    wxPOSIX_GROUP_READ = wxS_IRGRP,
    wxPOSIX_GROUP_WRITE = wxS_IWGRP,
    wxPOSIX_GROUP_EXECUTE = wxS_IXGRP,
    wxPOSIX_OTHERS_READ = wxS_IROTH,
    wxPOSIX_OTHERS_WRITE = wxS_IWOTH,
    wxPOSIX_OTHERS_EXECUTE = wxS_IXOTH,

    wxS_DEFAULT = 0666,
    wxS_DIR_DEFAULT = 0777
};

enum wxFileKind
{
    wxFILE_KIND_UNKNOWN,
    wxFILE_KIND_DISK,
    wxFILE_KIND_TERMINAL,
    wxFILE_KIND_PIPE
};

bool     wxFileExists( const wxString& aFile );
bool     wxDirExists( const wxString& aDir );
bool     wxIsAbsolutePath( const wxString& aPath );
bool     wxEndsWithPathSeparator( const wxString& aPath );
wxString wxPathOnly( const wxString& aPath );
wxString wxFileNameFromPath( const wxString& aPath );
bool     wxMkdir( const wxString& aDir, int aPerm = wxS_DIR_DEFAULT );
bool     wxRmdir( const wxString& aDir, int aFlags = 0 );
bool     wxRemoveFile( const wxString& aFile );
int      wxChmod( const wxString& aPath, int aMode );
bool     wxRenameFile( const wxString& aFrom, const wxString& aTo, bool aOverwrite = true );
bool     wxCopyFile( const wxString& aFrom, const wxString& aTo, bool aOverwrite = true );
wxString wxGetCwd();
bool     wxSetWorkingDirectory( const wxString& aDir );
time_t   wxFileModificationTime( const wxString& aFile );
bool     wxIsWild( const wxString& aPattern );
bool     wxMatchWild( const wxString& aPattern, const wxString& aText, bool aDotSpecial = true );
bool     wxIsWritable( const wxString& aPath );
bool     wxIsReadable( const wxString& aPath );
bool     wxIsExecutable( const wxString& aPath );
wxString wxFindFirstFile( const wxString& aSpec, int aFlags = 0 );
wxString wxFindNextFile();
wxString wxRealPath( const wxString& aPath );
bool     wxGetDiskSpace( const wxString& aPath, wxLongLong* aTotal = nullptr,
                         wxLongLong* aFree = nullptr );
wxFileKind wxGetFileKind( int aFd );
wxFileKind wxGetFileKind( FILE* aFile );

/// The size of a file, or -1.
long long kiwxFileSize( const wxString& aFile );


#ifdef _WIN32
/// No umask on Windows: does nothing, as in wxWidgets.
class wxUmaskChanger
{
public:
    explicit wxUmaskChanger( int ) {}
};
#else
/// Sets the umask in the constructor (unless -1) and restores it in the destructor.
class wxUmaskChanger
{
public:
    explicit wxUmaskChanger( int aUmaskNew ) :
            m_umaskOld( aUmaskNew == -1 ? -1 : static_cast<int>( umask( static_cast<mode_t>( aUmaskNew ) ) ) )
    {
    }

    ~wxUmaskChanger()
    {
        if( m_umaskOld != -1 )
            umask( static_cast<mode_t>( m_umaskOld ) );
    }

private:
    int m_umaskOld;
};
#endif

#define wxMAKE_UNIQUE_NAME_CAT2( a, b ) a##b
#define wxMAKE_UNIQUE_NAME_CAT( a, b ) wxMAKE_UNIQUE_NAME_CAT2( a, b )
#define wxMAKE_UNIQUE_NAME( name ) wxMAKE_UNIQUE_NAME_CAT( name, __LINE__ )
#define wxCHANGE_UMASK( m ) wxUmaskChanger wxMAKE_UNIQUE_NAME( umaskChanger_ )( m )


/// A list of directories to look for files in (wxWidgets filefn.h).
class wxPathList : public wxArrayString
{
public:
    wxPathList() = default;
    wxPathList( const wxArrayString& aArr ) { Add( aArr ); }

    /// Adds the directories of an environment variable (separated by ':' or ';').
    void AddEnvList( const wxString& aEnvVariable );

    bool Add( const wxString& aPath );
    void Add( const wxArrayString& aPaths );

    /// The first path + file name that exists, or empty.
    wxString FindValidPath( const wxString& aFilename ) const;

    /// As FindValidPath(), made absolute.
    wxString FindAbsoluteValidPath( const wxString& aFilename ) const;

    bool EnsureFileAccessible( const wxString& aPath );
};


// flags of the file system search functions (wxFindFirstFile()...)
#define wxFILE 1
#define wxDIR 2

#pragma GCC visibility pop
