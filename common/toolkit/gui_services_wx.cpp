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


#include <toolkit/gui_services.h>

#include <wx/app.h>
#include <wx/evtloop.h>
#include <wx/frame.h>
#include <wx/utils.h>
#include <wx/window.h>


namespace TOOLKIT
{

void Bell()
{
    wxBell();
}


bool IsKeyDown( int aKeyCode )
{
    return wxGetKeyState( static_cast<wxKeyCode>( aKeyCode ) );
}


void YieldEvents()
{
    wxYield();
}


void ProcessPendingEvents()
{
    if( wxTheApp )
        wxTheApp->ProcessPendingEvents();
}


bool IsYielding()
{
    wxEventLoopBase* loop = wxEventLoopBase::GetActive();
    return loop && loop->IsYielding();
}


TOOLKIT_WINDOW* FindWindowByName( const wxString& aName, TOOLKIT_WINDOW* aParent )
{
    return wxWindow::FindWindowByName( aName, aParent );
}


TOOLKIT_WINDOW* FindFocus()
{
    return wxWindow::FindFocus();
}


TOOLKIT_WINDOW* GetParent( const TOOLKIT_WINDOW* aWindow )
{
    return aWindow ? aWindow->GetParent() : nullptr;
}


TOOLKIT_WINDOW* GetTopLevelParent( TOOLKIT_WINDOW* aWindow )
{
    return aWindow ? wxGetTopLevelParent( aWindow ) : nullptr;
}


bool IsShownOnScreen( const TOOLKIT_WINDOW* aWindow )
{
    return aWindow && aWindow->IsShownOnScreen();
}


std::unique_ptr<TOOLKIT_WINDOW> CreateHiddenWindow( TOOLKIT_WINDOW* aParent, const wxSize& aSize )
{
    auto window = std::make_unique<wxFrame>( aParent, wxID_ANY, wxEmptyString, wxDefaultPosition,
                                             aSize, wxFRAME_NO_TASKBAR );
    window->Hide();
    return window;
}

} // namespace TOOLKIT
