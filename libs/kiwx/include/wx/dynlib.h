/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Derived from wxWidgets 3.2.4 (include/wx/dynlib.h, wxWindows Library Licence 3.1),
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

#pragma once

// exported from the kiwx library, as the wxWidgets API (WXDLLIMPEXP_BASE)
#ifndef _WIN32
#pragma GCC visibility push( default )
#endif

#include <wx/defs.h>
#include <wx/string.h>

#ifdef _WIN32
// HMODULE, without windows.h
typedef struct HINSTANCE__* wxDllType;
#else
typedef void* wxDllType;
#endif

enum wxDLFlags
{
    wxDL_LAZY = 0x00000001,       ///< resolve undefined symbols at first use
    wxDL_NOW = 0x00000002,        ///< resolve undefined symbols on load
    wxDL_GLOBAL = 0x00000004,     ///< export the symbols to the libraries loaded later
    wxDL_VERBATIM = 0x00000008,   ///< no extension added to the name
    wxDL_NOSHARE = 0x00000010,
    wxDL_QUIET = 0x00000020,      ///< no error logged if loading fails
    wxDL_GET_LOADED = 0x00000040,
    wxDL_DEFAULT = wxDL_NOW
};

enum wxDynamicLibraryCategory
{
    wxDL_LIBRARY,
    wxDL_MODULE
};

#define wxDYNLIB_FUNCTION( type, name, dynlib )                                                  \
    type pfn##name = (type) ( dynlib ).GetSymbol( wxT( #name ) )
#define wxDL_INIT_FUNC( pfx, name, dynlib ) pfx##name = (name##_t) ( dynlib ).RawGetSymbol( #name )


class wxDynamicLibrary
{
public:
    static wxDllType GetProgramHandle();
#ifdef _WIN32
    static wxString  GetDllExt( wxDynamicLibraryCategory = wxDL_LIBRARY ) { return ".dll"; }
#else
    static wxString  GetDllExt( wxDynamicLibraryCategory = wxDL_LIBRARY ) { return ".so"; }
#endif

    wxDynamicLibrary() = default;
    wxDynamicLibrary( const wxString& aLibName, int aFlags = wxDL_DEFAULT )
    {
        Load( aLibName, aFlags );
    }
    ~wxDynamicLibrary() { Unload(); }

    wxDynamicLibrary( const wxDynamicLibrary& ) = delete;
    wxDynamicLibrary& operator=( const wxDynamicLibrary& ) = delete;

    bool             IsLoaded() const { return m_handle != nullptr; }
    bool             Load( const wxString& aLibName, int aFlags = wxDL_DEFAULT );
    static wxDllType RawLoad( const wxString& aLibName, int aFlags = wxDL_DEFAULT );

    void      Attach( wxDllType aHandle )
    {
        Unload();
        m_handle = aHandle;
    }
    wxDllType Detach()
    {
        wxDllType h = m_handle;
        m_handle = nullptr;
        return h;
    }

    static void Unload( wxDllType aHandle );
    void        Unload()
    {
        if( IsLoaded() )
        {
            Unload( m_handle );
            m_handle = nullptr;
        }
    }

    wxDllType GetLibHandle() const { return m_handle; }

    bool HasSymbol( const wxString& aName ) const
    {
        bool ok;
        DoGetSymbol( aName, &ok );
        return ok;
    }

    void*        GetSymbol( const wxString& aName, bool* aSuccess = nullptr ) const;
    static void* RawGetSymbol( wxDllType aHandle, const wxString& aName );
    void*        RawGetSymbol( const wxString& aName ) const { return RawGetSymbol( m_handle, aName ); }

    static wxString CanonicalizeName( const wxString& aName,
                                      wxDynamicLibraryCategory aCat = wxDL_LIBRARY );

protected:
    void*       DoGetSymbol( const wxString& aName, bool* aSuccess = nullptr ) const;
    static void ReportError( const wxString& aMsg, const wxString& aName = wxString() );

    wxDllType m_handle = nullptr;
};

#ifndef _WIN32
#pragma GCC visibility pop
#endif
