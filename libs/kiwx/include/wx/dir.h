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
#include <wx/filefn.h>
#include <wx/string.h>

enum wxDirFlags
{
    wxDIR_FILES = 0x0001,
    wxDIR_DIRS = 0x0002,
    wxDIR_HIDDEN = 0x0004,
    wxDIR_DOTDOT = 0x0008,
    wxDIR_NO_FOLLOW = 0x0010,
    wxDIR_DEFAULT = wxDIR_FILES | wxDIR_DIRS | wxDIR_HIDDEN
};

enum wxDirTraverseResult
{
    wxDIR_IGNORE = -1,
    wxDIR_STOP,
    wxDIR_CONTINUE
};


class wxDirTraverser
{
public:
    virtual ~wxDirTraverser() = default;

    virtual wxDirTraverseResult OnFile( const wxString& aFileName ) = 0;
    virtual wxDirTraverseResult OnDir( const wxString& aDirName ) = 0;
    virtual wxDirTraverseResult OnOpenError( const wxString& ) { return wxDIR_IGNORE; }
};


/// A directory listing in readdir() order, with the wxDir API.
class wxDir
{
public:
    wxDir() = default;
    wxDir( const wxString& aDir ) { Open( aDir ); }
    ~wxDir() { Close(); }

    wxDir( const wxDir& ) = delete;
    wxDir& operator=( const wxDir& ) = delete;

    bool Open( const wxString& aDir );
    void Close();
    bool IsOpened() const { return m_dir != nullptr; }

    wxString GetName() const;
    wxString GetNameWithSep() const;

    bool GetFirst( wxString* aFileName, const wxString& aFileSpec = wxString(),
                   int aFlags = wxDIR_DEFAULT ) const;
    bool GetNext( wxString* aFileName ) const;

    bool HasFiles( const wxString& aSpec = wxString() ) const;
    bool HasSubDirs( const wxString& aSpec = wxString() ) const;

    size_t Traverse( wxDirTraverser& aSink, const wxString& aFileSpec = wxString(),
                     int aFlags = wxDIR_DEFAULT ) const;

    static size_t GetAllFiles( const wxString& aDirName, wxArrayString* aFiles,
                               const wxString& aFileSpec = wxString(), int aFlags = wxDIR_DEFAULT );
    static wxString FindFirst( const wxString& aDirName, const wxString& aFileSpec,
                               int aFlags = wxDIR_DEFAULT );
    static bool Exists( const wxString& aDir ) { return wxDirExists( aDir ); }
    static bool Make( const wxString& aDir, int aPerm = wxS_DIR_DEFAULT, int aFlags = 0 );
    static bool Remove( const wxString& aDir, int aFlags = 0 );

private:
    bool read( wxString* aFileName ) const;

    wxString       m_dirName;
    mutable void*  m_dir = nullptr;   ///< DIR*
    mutable wxString m_fileSpec;
    mutable int    m_flags = wxDIR_DEFAULT;
};

#ifndef _WIN32
#pragma GCC visibility pop
#endif
