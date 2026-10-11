/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Derived from wxWidgets 3.2.4 (include/wx/init.h, wxWindows Library Licence 3.1),
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

/**
 * Initialises the library for a program that does not use wxEntry(): the application object
 * (a plain wxAppConsole when the program has none) and its command line.  The calls nest:
 * only the last wxUninitialize() cleans up.
 */
bool wxEntryStart( int& aArgc, char** aArgv );
void wxEntryCleanup();

bool wxInitialize();
bool wxInitialize( int& aArgc, char** aArgv );
void wxUninitialize();


/// wxInitialize() for the lifetime of the object.
class wxInitializer
{
public:
    wxInitializer() : m_ok( wxInitialize() ) {}
    wxInitializer( int& aArgc, char** aArgv ) : m_ok( wxInitialize( aArgc, aArgv ) ) {}
    ~wxInitializer()
    {
        if( m_ok )
            wxUninitialize();
    }

    wxInitializer( const wxInitializer& ) = delete;
    wxInitializer& operator=( const wxInitializer& ) = delete;

    bool IsOk() const { return m_ok; }
    operator bool() const { return m_ok; }

private:
    bool m_ok;
};

#ifndef _WIN32
#pragma GCC visibility pop
#endif
