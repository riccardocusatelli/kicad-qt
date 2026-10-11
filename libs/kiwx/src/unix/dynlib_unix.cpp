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
 * @file dynlib_unix.cpp
 *
 * wxDynamicLibrary on Unix: dlopen().  Derived from wxWidgets 3.2.4 (src/unix/dlunix.cpp,
 * wxWindows Library Licence 3.1).
 */

#include <wx/dynlib.h>

#include <wx/log.h>
#include <wx/translation.h>

#include <dlfcn.h>


wxDllType wxDynamicLibrary::GetProgramHandle()
{
    return dlopen( nullptr, RTLD_LAZY );
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
