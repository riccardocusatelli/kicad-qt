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

// The parts of PGM_GUI with wxWidgets: the toolkit, the dialogs, the splash screen.

#include <wx/app.h>
#include <wx/dialog.h>
#include <wx/eventfilter.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/propgrid/propgrid.h>
#include <wx/splash.h>
#include <wx/tooltip.h>
#include <wx/utils.h>

#include <advanced_config.h>
#include <bitmap_store.h>
#include <build_version.h>
#include <core/ki_assert.h>
#include <pgm_gui.h>
#include <raster_image_wx.h>
#include <ui_theme.h>

#include <algorithm>


void PGM_GUI::initGuiToolkit()
{
    wxInitAllImageHandlers();
    RegisterWxImageDecoder();

    // Register the bundled UI fonts before any window (and any wxFont) is created.
    UI_THEME::LoadUiFonts();

#if !wxCHECK_VERSION( 3, 3, 0 )
    // Without this the wxPropertyGridManager segfaults on Windows.
    if( !wxPGGlobalVars )
        wxPGInitResourceModule();
#endif
}


void PGM_GUI::configureGuiToolkit( bool aHeadless )
{

#ifdef __WXGTK__
    switch( wxGetDisplayInfo().type )
    {
    case wxDisplayX11:     SetDisplayServerInfo( wxS( "X11" ) );     break;
    case wxDisplayWayland: SetDisplayServerInfo( wxS( "Wayland" ) ); break;
    case wxDisplayNone:    SetDisplayServerInfo( wxS( "None" ) );    break;
    default:               SetDisplayServerInfo( wxS( "Unknown" ) );
    }
#endif

#if defined( __WXGTK__ ) && defined( KICAD_DESKTOP_APP_NAME )
    // On wx >= 3.3.1 wxGTK feeds the class name to gdk_wayland_window_set_application_id(),
    // which lets Wayland compositors resolve the correct window icon and launch feedback. The
    // X11 WM_CLASS is applied per window in EDA_BASE_FRAME instead, because wxGTK derives it
    // from the (human-facing) app display name and would otherwise not match the launcher.
    if( !GetDesktopAppId().IsEmpty() )
        App().SetClassName( GetDesktopAppId() );
#endif

#if defined( __WXMSW__ ) || defined( __WXOSX__ )
    // wx ends a session by closing the top window with an ordinary close event. Calling wx's handler
    // here instead of Skip() keeps that close inside the flag's scope
    if( !aHeadless )
    {
        App().Bind( wxEVT_QUERY_END_SESSION,
                    [this]( wxCloseEvent& aEvent )
                    {
                        SetSessionEnding( true );
                        App().OnQueryEndSession( aEvent );
                        SetSessionEnding( false );
                    } );

        App().Bind( wxEVT_END_SESSION,
                    [this]( wxCloseEvent& aEvent )
                    {
                        SetSessionEnding( true );
                        App().OnEndSession( aEvent );
                        SetSessionEnding( false );
                    } );
    }
#endif

    // This sets the maximum tooltip display duration to 10s (up from 5) but only affects
    // Windows as other platforms display tooltips while the mouse is not moving
    if( !aHeadless )
    {
        wxToolTip::Enable( true );
        wxToolTip::SetAutoPop( 10000 );
    }

    if( ADVANCED_CFG::GetCfg().m_UpdateUIEventInterval != 0 )
        wxUpdateUIEvent::SetUpdateInterval( ADVANCED_CFG::GetCfg().m_UpdateUIEventInterval );
}


bool PGM_GUI::ConfirmAction( const wxString& aTitle, const wxString& aMessage )
{
    return wxMessageBox( aMessage, aTitle, wxYES_NO | wxICON_QUESTION ) == wxYES;
}


void PGM_GUI::SafeYield()
{
    wxSafeYield();
}


wxApp& PGM_GUI::App()
{
    KI_ASSERT( wxTheApp );
    return *wxTheApp;
}


const wxString PGM_GUI::AskUserForPreferredEditor( const wxString& aDefaultEditor )
{
    // Create a mask representing the executable files in the current platform
#ifdef __WINDOWS__
    wxString mask( _( "Executable file" ) + wxT( " (*.exe)|*.exe" ) );
#else
    wxString mask( _( "Executable file" ) + wxT( " (*)|*" ) );
#endif

    // Extract the path, name and extension from the default editor (even if the editor's
    // name was empty, this method will succeed and return empty strings).
    wxString path, name, ext;
    wxFileName::SplitPath( aDefaultEditor, &path, &name, &ext );

    // Show the modal editor and return the file chosen (may be empty if the user cancels
    // the dialog).
    return wxFileSelector( _( "Select Preferred Editor" ), path, name, wxT( "." ) + ext,
                           mask, wxFD_OPEN | wxFD_FILE_MUST_EXIST, nullptr );
}


void PGM_GUI::ShowSplash()
{
    // Disabling until we change to load each DSO at startup rather than lazy-load when needed.
    // Note that once the splash screen is re-enabled, there are some remaining bugs to fix:
    // Any wxWidgets error dialogs that appear during startup are hidden by the splash screen,
    // so we either need to prevent these from happening (probably not feasible) or else change
    // the error-handling path to make sure errors go on top of the splash.
#if 0
    if( m_splash )
        return;

    m_splash = new WX_SPLASH( KiBitmap( BITMAPS::splash ), wxSPLASH_CENTRE_ON_SCREEN, 0,
                              NULL, -1, wxDefaultPosition, wxDefaultSize,
                              wxBORDER_NONE | wxSTAY_ON_TOP );
    wxYield();
#endif
}


void PGM_GUI::HideSplash()
{
    if( !m_splash )
        return;

    m_splash->Close( true );
    m_splash->Destroy();
    m_splash = nullptr;
}


int PGM_GUI::FilterAppEvent( wxEvent& aEvent )
{
    // Windows hear a theme switch in no set order, and each reloads its icons from the store
    if( aEvent.GetEventType() == wxEVT_SYS_COLOUR_CHANGED )
        GetBitmapStore()->ThemeChanged();

    if( aEvent.GetEventType() == wxEVT_SHOW )
    {
        wxShowEvent& event = static_cast<wxShowEvent&>( aEvent );
        wxDialog*    dialog = dynamic_cast<wxDialog*>( event.GetEventObject() );

        if( dialog )
        {
            if( event.IsShown() && dialog->IsModal() )
            {
                m_ModalDialogs.push_back( dialog );
            }
            // Under GTK, sometimes the modal flag is cleared before hiding
            else if( !event.IsShown() && !m_ModalDialogs.empty() )
            {
                // If we close the expected dialog, remove it from our stack
                if( m_ModalDialogs.back() == dialog )
                    m_ModalDialogs.pop_back();
                // If an out-of-order, remove all dialogs added after the closed one
                else if( auto it = std::ranges::find( m_ModalDialogs, dialog ); it != m_ModalDialogs.end() )
                    m_ModalDialogs.erase( it, m_ModalDialogs.end() );
            }
        }
    }

    return wxEventFilter::Event_Skip;
}
