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
 * @file dynlib_msw.cpp
 *
 * wxDynamicLibrary on Windows: LoadLibrary(), as wxWidgets 3.2.4 (src/msw/dlmsw.cpp).
 */

#include <wx/dynlib.h>

#include <wx/log.h>
#include <wx/translation.h>

#include <system_error>

#include <windows.h>


wxDllType wxDynamicLibrary::GetProgramHandle()
{
    return GetModuleHandleW( nullptr );
}


wxDllType wxDynamicLibrary::RawLoad( const wxString& aLibName, int aFlags )
{
    if( aFlags & wxDL_GET_LOADED )
        return GetModuleHandleW( aLibName.wc_str() );

    return LoadLibraryW( aLibName.wc_str() );
}


void wxDynamicLibrary::Unload( wxDllType aHandle )
{
    if( !FreeLibrary( aHandle ) )
        ReportError( _( "Failed to unload shared library" ) );
}


void* wxDynamicLibrary::RawGetSymbol( wxDllType aHandle, const wxString& aName )
{
    // the symbol names are ASCII
    return reinterpret_cast<void*>( GetProcAddress( aHandle, aName.ToStdString().c_str() ) );
}


void wxDynamicLibrary::ReportError( const wxString& aMessage, const wxString& aName )
{
    wxString msg( aMessage );

    if( aName.IsEmpty() && msg.Find( "%s" ) == wxNOT_FOUND )
        msg += "%s";

    wxLogError( msg + wxT( ": %s" ), aName, wxString( std::system_category().message( static_cast<int>( GetLastError() ) ) ) );
}
