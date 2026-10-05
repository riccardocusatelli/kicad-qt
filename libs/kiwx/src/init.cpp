/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Derived from wxWidgets 3.2.4 (src/common/init.cpp, wxWindows Library Licence 3.1),
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

#include <wx/init.h>

#include <wx/app.h>
#include <wx/log.h>
#include <wx/msgout.h>


namespace
{
int            s_initCount = 0;
wxAppConsole*  s_createdApp = nullptr;      ///< The application created by wxEntryStart()
}


bool wxEntryStart( int& aArgc, char** aArgv )
{
    wxAppConsole* app = wxAppConsole::GetInstance();

    // as wxWidgets: a program without application object gets a console one
    if( !app )
    {
        s_createdApp = new wxAppConsole;
        app = s_createdApp;
    }

    app->argc = aArgc;
    app->argv.Init( aArgc, aArgv );
    return true;
}


void wxEntryCleanup()
{
    // the pending log messages, then the log target, as wxWidgets
    wxLog::FlushActive();
    delete wxLog::SetActiveTarget( nullptr );
    delete wxMessageOutput::Set( nullptr );

    delete s_createdApp;
    s_createdApp = nullptr;
}


bool wxInitialize()
{
    int argc = 0;
    return wxInitialize( argc, nullptr );
}


bool wxInitialize( int& aArgc, char** aArgv )
{
    if( s_initCount++ > 0 )
        return true;

    if( !wxEntryStart( aArgc, aArgv ) )
    {
        s_initCount = 0;
        return false;
    }

    return true;
}


void wxUninitialize()
{
    if( --s_initCount == 0 )
        wxEntryCleanup();
}
