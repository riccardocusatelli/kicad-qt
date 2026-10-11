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

#include <wx/arrstr.h>
#include <wx/datetime.h>
#include <wx/filefn.h>
#include <wx/longlong.h>
#include <wx/string.h>
#include <wx/translation.h>

// as wxWidgets
#include <wx/file.h>
#include <wx/intl.h>

class wxFile;
class wxFFile;

enum wxSizeConvention
{
    wxSIZE_CONV_TRADITIONAL,
    wxSIZE_CONV_IEC,
    wxSIZE_CONV_SI
};

enum
{
    wxPATH_NORM_ENV_VARS = 0x0001,
    wxPATH_NORM_DOTS = 0x0002,
    wxPATH_NORM_TILDE = 0x0004,
    wxPATH_NORM_CASE = 0x0008,
    wxPATH_NORM_ABSOLUTE = 0x0010,
    wxPATH_NORM_LONG = 0x0020,
    wxPATH_NORM_SHORTCUT = 0x0040,
    wxPATH_NORM_DEPR_OLD_DEFAULT = 0x00ff & ~wxPATH_NORM_CASE,
    wxPATH_NORM_ALL = wxPATH_NORM_DEPR_OLD_DEFAULT
};

enum
{
    wxPATH_NO_SEPARATOR = 0x0000,
    wxPATH_GET_VOLUME = 0x0001,
    wxPATH_GET_SEPARATOR = 0x0002
};

enum
{
    wxPATH_MKDIR_FULL = 0x0001
};

enum
{
    wxPATH_RMDIR_FULL = 0x0001,
    wxPATH_RMDIR_RECURSIVE = 0x0002
};

enum
{
    wxFILE_EXISTS_REGULAR = 0x0001,
    wxFILE_EXISTS_DIR = 0x0002,
    wxFILE_EXISTS_SYMLINK = 0x1004,
    wxFILE_EXISTS_DEVICE = 0x0008,
    wxFILE_EXISTS_FIFO = 0x0010,
    wxFILE_EXISTS_SOCKET = 0x0020,
    wxFILE_EXISTS_NO_FOLLOW = 0x1000,
    wxFILE_EXISTS_ANY = 0x1FFF
};


/**
 * A file name split in volume, directories, name and extension, with the semantics of
 * wxFileName for the Unix and the DOS (Windows) paths.
 */
class wxFileName
{
public:
    wxFileName() { Clear(); }
    wxFileName( const wxFileName& aOther ) = default;
    wxFileName& operator=( const wxFileName& aOther ) = default;
    wxFileName& operator=( const wxString& aFilename )
    {
        Assign( aFilename );
        return *this;
    }
    wxFileName( const wxString& aFullPath, wxPathFormat aFormat = wxPATH_NATIVE ) { Assign( aFullPath, aFormat ); }
    wxFileName( const wxString& aPath, const wxString& aName, wxPathFormat aFormat = wxPATH_NATIVE )
    {
        Assign( aPath, aName, aFormat );
    }
    wxFileName( const wxString& aPath, const wxString& aName, const wxString& aExt,
                wxPathFormat aFormat = wxPATH_NATIVE )
    {
        Assign( aPath, aName, aExt, aFormat );
    }
    wxFileName( const wxString& aVolume, const wxString& aPath, const wxString& aName,
                const wxString& aExt, wxPathFormat aFormat = wxPATH_NATIVE )
    {
        Assign( aVolume, aPath, aName, aExt, aFormat );
    }

    void Assign( const wxFileName& aOther ) { *this = aOther; }
    void Assign( const wxString& aFullPath, wxPathFormat aFormat = wxPATH_NATIVE );
    void Assign( const wxString& aVolume, const wxString& aPath, const wxString& aName,
                 const wxString& aExt, bool aHasExt, wxPathFormat aFormat = wxPATH_NATIVE );
    void Assign( const wxString& aVolume, const wxString& aPath, const wxString& aName,
                 const wxString& aExt, wxPathFormat aFormat = wxPATH_NATIVE )
    {
        Assign( aVolume, aPath, aName, aExt, !aExt.empty(), aFormat );
    }
    void Assign( const wxString& aPath, const wxString& aName, wxPathFormat aFormat = wxPATH_NATIVE );
    void Assign( const wxString& aPath, const wxString& aName, const wxString& aExt,
                 wxPathFormat aFormat = wxPATH_NATIVE );
    void AssignDir( const wxString& aDir, wxPathFormat aFormat = wxPATH_NATIVE ) { Assign( aDir, wxString(), aFormat ); }
    void AssignCwd( const wxString& aVolume = wxString() );
    void AssignHomeDir();
    void AssignTempFileName( const wxString& aPrefix );
    void AssignTempFileName( const wxString& aPrefix, wxFile* aFileTemp );
    void AssignTempFileName( const wxString& aPrefix, wxFFile* aFileTemp );

    void Clear();

    static wxFileName FileName( const wxString& aFile, wxPathFormat aFormat = wxPATH_NATIVE )
    {
        return wxFileName( aFile, aFormat );
    }

    static wxFileName DirName( const wxString& aDir, wxPathFormat aFormat = wxPATH_NATIVE )
    {
        wxFileName fn;
        fn.AssignDir( aDir, aFormat );
        return fn;
    }

    bool IsOk() const
    {
        return m_dirs.size() != 0 || !m_name.empty() || !m_relative || !m_ext.empty() || m_hasExt;
    }

    bool FileExists() const { return FileExists( GetFullPath() ); }
    bool DirExists() const { return DirExists( GetPath() ); }
    bool Exists( int aFlags = wxFILE_EXISTS_ANY ) const { return Exists( GetFullPath(), aFlags ); }
    static bool FileExists( const wxString& aFile );
    static bool DirExists( const wxString& aDir );
    static bool Exists( const wxString& aPath, int aFlags = wxFILE_EXISTS_ANY );

    bool IsDirWritable() const { return wxIsWritable( GetPath() ); }
    static bool IsDirWritable( const wxString& aDir ) { return wxIsWritable( aDir ); }
    bool IsDirReadable() const { return wxIsReadable( GetPath() ); }
    static bool IsDirReadable( const wxString& aDir ) { return wxIsReadable( aDir ); }
    bool IsFileWritable() const { return wxIsWritable( GetFullPath() ); }
    static bool IsFileWritable( const wxString& aFile ) { return wxIsWritable( aFile ); }
    bool IsFileReadable() const { return wxIsReadable( GetFullPath() ); }
    static bool IsFileReadable( const wxString& aFile ) { return wxIsReadable( aFile ); }
    bool IsFileExecutable() const { return wxIsExecutable( GetFullPath() ); }
    static bool IsFileExecutable( const wxString& aFile ) { return wxIsExecutable( aFile ); }

    bool GetTimes( wxDateTime* aAccess, wxDateTime* aMod, wxDateTime* aCreate ) const;
    bool SetTimes( const wxDateTime* aAccess, const wxDateTime* aMod, const wxDateTime* aCreate ) const;
    bool Touch() const;
    wxDateTime GetModificationTime() const;
    wxDateTime GetAccessTime() const;

    static wxString GetCwd( const wxString& aVolume = wxString() );
    static bool     SetCwd( const wxString& aCwd );
    bool            SetCwd() const { return SetCwd( GetPath() ); }
    static wxString GetHomeDir();
    static wxString GetTempDir();

    static wxString CreateTempFileName( const wxString& aPrefix, wxFile* aFileTemp = nullptr );
    static wxString CreateTempFileName( const wxString& aPrefix, wxFFile* aFileTemp );

    bool Mkdir( int aPerm = wxS_DIR_DEFAULT, int aFlags = 0 ) const { return Mkdir( GetPath(), aPerm, aFlags ); }
    static bool Mkdir( const wxString& aDir, int aPerm = wxS_DIR_DEFAULT, int aFlags = 0 );
    bool Rmdir( int aFlags = 0 ) const { return Rmdir( GetPath(), aFlags ); }
    static bool Rmdir( const wxString& aDir, int aFlags = 0 );

    bool Normalize( int aFlags, const wxString& aCwd = wxString(), wxPathFormat aFormat = wxPATH_NATIVE );
    bool Normalize() { return Normalize( wxPATH_NORM_DEPR_OLD_DEFAULT ); }
    bool MakeAbsolute( const wxString& aCwd = wxString(), wxPathFormat aFormat = wxPATH_NATIVE )
    {
        return Normalize( wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE | wxPATH_NORM_TILDE, aCwd, aFormat );
    }
    wxString GetAbsolutePath( const wxString& aCwd = wxString(), wxPathFormat aFormat = wxPATH_NATIVE ) const
    {
        wxFileName fn( *this );
        fn.MakeAbsolute( aCwd, aFormat );
        return fn.GetFullPath();
    }
    bool MakeRelativeTo( const wxString& aPathBase = wxString(), wxPathFormat aFormat = wxPATH_NATIVE );
    bool ReplaceEnvVariable( const wxString& aEnvName, const wxString& aReplacementFmt = wxT( "$%s" ),
                             wxPathFormat aFormat = wxPATH_NATIVE );
    bool ReplaceHomeDir( wxPathFormat aFormat = wxPATH_NATIVE );

    void DontFollowLink() { m_dontFollowLinks = true; }
    bool ShouldFollowLink() const { return !m_dontFollowLinks; }
    wxFileName ResolveLink();

    bool SameAs( const wxFileName& aOther, wxPathFormat aFormat = wxPATH_NATIVE ) const;
    bool operator==( const wxFileName& aOther ) const { return SameAs( aOther ); }
    bool operator!=( const wxFileName& aOther ) const { return !SameAs( aOther ); }
    bool operator==( const wxString& aOther ) const { return SameAs( wxFileName( aOther ) ); }
    bool operator!=( const wxString& aOther ) const { return !SameAs( wxFileName( aOther ) ); }

    /// wxPATH_NATIVE as the format of this system (wxPATH_DOS on Windows, wxPATH_UNIX elsewhere).
    static wxPathFormat GetFormat( wxPathFormat aFormat = wxPATH_NATIVE );

    static bool     IsCaseSensitive( wxPathFormat aFormat = wxPATH_NATIVE );
    static wxString GetForbiddenChars( wxPathFormat aFormat = wxPATH_NATIVE );
    static wxString GetVolumeSeparator( wxPathFormat aFormat = wxPATH_NATIVE );
    static wxUniChar GetPathSeparator( wxPathFormat aFormat = wxPATH_NATIVE );
    static wxString GetPathSeparators( wxPathFormat aFormat = wxPATH_NATIVE );
    static wxString GetPathTerminators( wxPathFormat aFormat = wxPATH_NATIVE ) { return GetPathSeparators( aFormat ); }
    static bool     IsPathSeparator( wxUniChar aCh, wxPathFormat aFormat = wxPATH_NATIVE );
    static bool     IsMSWUniqueVolumeNamePath( const wxString& aPath, wxPathFormat aFormat = wxPATH_NATIVE );

    bool IsAbsolute( wxPathFormat aFormat = wxPATH_NATIVE ) const;
    bool IsRelative( wxPathFormat aFormat = wxPATH_NATIVE ) const { return !IsAbsolute( aFormat ); }
    bool IsDir() const { return m_name.empty() && m_ext.empty(); }

    size_t GetDirCount() const { return m_dirs.size(); }
    bool   AppendDir( const wxString& aDir );
    void   PrependDir( const wxString& aDir ) { InsertDir( 0, aDir ); }
    bool   InsertDir( size_t aBefore, const wxString& aDir );
    void   RemoveDir( size_t aPos ) { m_dirs.RemoveAt( aPos ); }
    void   RemoveLastDir() { RemoveDir( GetDirCount() - 1 ); }
    const wxArrayString& GetDirs() const { return m_dirs; }

    void     SetExt( const wxString& aExt ) { m_ext = aExt; m_hasExt = !m_ext.empty(); }
    void     ClearExt() { m_ext.clear(); m_hasExt = false; }
    void     SetEmptyExt() { m_ext.clear(); m_hasExt = true; }
    wxString GetExt() const { return m_ext; }
    bool     HasExt() const { return m_hasExt; }

    void     SetName( const wxString& aName ) { m_name = aName; }
    wxString GetName() const { return m_name; }
    bool     HasName() const { return !m_name.empty(); }

    void     SetVolume( const wxString& aVolume ) { m_volume = aVolume; }
    wxString GetVolume() const { return m_volume; }
    bool     HasVolume() const { return !m_volume.empty(); }

    void     SetFullName( const wxString& aFullName );
    wxString GetFullName() const;

    void     SetPath( const wxString& aPath, wxPathFormat aFormat = wxPATH_NATIVE );
    wxString GetPath( int aFlags = wxPATH_GET_VOLUME, wxPathFormat aFormat = wxPATH_NATIVE ) const;
    wxString GetPath( bool aWithSep, wxPathFormat aFormat = wxPATH_NATIVE ) const
    {
        return GetPath( aWithSep ? wxPATH_GET_SEPARATOR : 0, aFormat );
    }
    wxString GetPathWithSep( wxPathFormat aFormat = wxPATH_NATIVE ) const
    {
        return GetPath( wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR, aFormat );
    }
    wxString GetFullPath( wxPathFormat aFormat = wxPATH_NATIVE ) const;
#ifdef _WIN32
    wxString GetShortPath() const;
    wxString GetLongPath() const;
#else
    wxString GetShortPath() const { return GetFullPath(); }
    wxString GetLongPath() const { return GetFullPath(); }
#endif

    static void SplitPath( const wxString& aFullPath, wxString* aVolume, wxString* aPath,
                           wxString* aName, wxString* aExt, bool* aHasExt = nullptr,
                           wxPathFormat aFormat = wxPATH_NATIVE );
    static void SplitPath( const wxString& aFullPath, wxString* aVolume, wxString* aPath,
                           wxString* aName, wxString* aExt, wxPathFormat aFormat )
    {
        SplitPath( aFullPath, aVolume, aPath, aName, aExt, nullptr, aFormat );
    }
    static void SplitPath( const wxString& aFullPath, wxString* aPath, wxString* aName,
                           wxString* aExt, wxPathFormat aFormat = wxPATH_NATIVE );
    static void SplitVolume( const wxString& aFullPath, wxString* aVolume, wxString* aPath,
                             wxPathFormat aFormat = wxPATH_NATIVE );
    static wxString StripExtension( const wxString& aFullPath );

    wxULongLong        GetSize() const { return GetSize( GetFullPath() ); }
    static wxULongLong GetSize( const wxString& aFile );
    wxString GetHumanReadableSize( const wxString& aNullSize = wxGetTranslation( "Not available" ),
                                   int aPrecision = 1, wxSizeConvention aConv = wxSIZE_CONV_TRADITIONAL ) const
    {
        return GetHumanReadableSize( GetSize(), aNullSize, aPrecision, aConv );
    }
    static wxString GetHumanReadableSize( const wxULongLong& aSize,
                                          const wxString& aNullSize = wxGetTranslation( "Not available" ),
                                          int aPrecision = 1,
                                          wxSizeConvention aConv = wxSIZE_CONV_TRADITIONAL );

private:
    /// SetPath(), with or without a volume at the start of @a aPath.
    void doSetPath( const wxString& aPath, wxPathFormat aFormat, bool aMayHaveVolume );

    static bool IsValidDirComponent( const wxString& aDir );

    wxString      m_volume;
    wxArrayString m_dirs;
    wxString      m_name;
    wxString      m_ext;
    bool          m_relative = true;
    bool          m_hasExt = false;
    bool          m_dontFollowLinks = false;
};

#ifndef _WIN32
#pragma GCC visibility pop
#endif
