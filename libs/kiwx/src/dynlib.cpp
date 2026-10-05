/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Derived from wxWidgets 3.2.4 (src/common/dynlib.cpp and src/unix/dlunix.cpp, wxWindows Library Licence 3.1),
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

#include <wx/dynlib.h>

#include <wx/filename.h>
#include <wx/log.h>
#include <wx/translation.h>


bool wxDynamicLibrary::Load( const wxString& aLibNameOrig, int aFlags )
{
    wxASSERT_MSG( m_handle == 0, wxT( "Library already loaded." ) );

    // the extension is added unless told not to, or already there
    wxString libname = aLibNameOrig;

    if( !( aFlags & wxDL_VERBATIM ) )
    {
        wxString ext;
        wxFileName::SplitPath( libname, nullptr, nullptr, &ext );

        if( ext.empty() )
            libname += GetDllExt( wxDL_MODULE );
    }

    m_handle = RawLoad( libname, aFlags );

    if( m_handle == 0 && !( aFlags & wxDL_QUIET ) )
        ReportError( _( "Failed to load shared library '%s'" ), libname );

    return IsLoaded();
}


wxDllType wxDynamicLibrary::RawLoad( const wxString& aLibName, int aFlags )
{
    wxASSERT_MSG( !( aFlags & wxDL_NOW ) || !( aFlags & wxDL_LAZY ),
                  wxT( "wxDL_LAZY and wxDL_NOW are mutually exclusive." ) );

    // glibc fails with no flags: RTLD_NOW unless lazy
    int rtldFlags = aFlags & wxDL_LAZY ? RTLD_LAZY : RTLD_NOW;

    if( aFlags & wxDL_GLOBAL )
        rtldFlags |= RTLD_GLOBAL;

    return dlopen( aLibName.fn_str(), rtldFlags );
}


void wxDynamicLibrary::Unload( wxDllType aHandle )
{
    if( dlclose( aHandle ) != 0 )
        ReportError( _( "Failed to unload shared library" ) );
}


void* wxDynamicLibrary::RawGetSymbol( wxDllType aHandle, const wxString& aName )
{
    return dlsym( aHandle, aName.fn_str() );
}


void* wxDynamicLibrary::DoGetSymbol( const wxString& aName, bool* aSuccess ) const
{
    wxCHECK_MSG( IsLoaded(), nullptr, wxT( "Can't load symbol from unloaded library" ) );

    void* symbol = RawGetSymbol( m_handle, aName );

    if( aSuccess )
        *aSuccess = symbol != nullptr;

    return symbol;
}


void* wxDynamicLibrary::GetSymbol( const wxString& aName, bool* aSuccess ) const
{
    void* symbol = DoGetSymbol( aName, aSuccess );

    if( !symbol )
        ReportError( _( "Couldn't find symbol '%s' in a dynamic library" ), aName );

    return symbol;
}


wxString wxDynamicLibrary::CanonicalizeName( const wxString& aName, wxDynamicLibraryCategory aCat )
{
    // libraries start with "lib" on Unix, modules have no prefix
    wxString nameCanonic = aCat == wxDL_LIBRARY ? wxString( "lib" ) : wxString();
    nameCanonic << aName << GetDllExt( aCat );
    return nameCanonic;
}


void wxDynamicLibrary::ReportError( const wxString& aMessage, const wxString& aName )
{
    wxString msg( aMessage );

    if( aName.IsEmpty() && msg.Find( "%s" ) == wxNOT_FOUND )
        msg += "%s";

    wxASSERT( msg.Find( "%s" ) != wxNOT_FOUND );

    const char* dlerr = dlerror();
    wxString    err( dlerr ? dlerr : "" );

    if( err.empty() )
        err = _( "Unknown dynamic library error" );

    wxLogError( msg + wxT( ": %s" ), aName, err );
}
