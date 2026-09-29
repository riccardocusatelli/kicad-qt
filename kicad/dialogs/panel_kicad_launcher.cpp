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

#include <bitmaps.h>
#include <bitmap_store.h>
#include <kicad_manager_frame.h>
#include <kiplatform/policy.h>
#include <policy_keys.h>
#include <tool/tool_manager.h>
#include <tools/kicad_manager_actions.h>
#include <tools/kicad_manager_control.h>
#include <widgets/bitmap_button.h>
#include <wx/settings.h>
#include <wx/stattext.h>

#include "panel_kicad_launcher.h"
#include "../widgets/home_widgets.h"

#include <ui_theme.h>


PANEL_KICAD_LAUNCHER::PANEL_KICAD_LAUNCHER( wxWindow* aParent ) :
        PANEL_KICAD_LAUNCHER_BASE( aParent ),
        m_frame( static_cast<KICAD_MANAGER_FRAME*>( aParent->GetParent() ) )
{
    CreateLaunchers();

    Bind( wxEVT_SYS_COLOUR_CHANGED, wxSysColourChangedEventHandler( PANEL_KICAD_LAUNCHER::onThemeChanged ), this );
}


PANEL_KICAD_LAUNCHER::~PANEL_KICAD_LAUNCHER()
{
    m_frame->SetPcmButton( nullptr );

    for( wxWindow* window : m_scrolledWindow->GetChildren() )
    {
        if( dynamic_cast<BITMAP_BUTTON*>( window ) != nullptr )
            window->Unbind( wxEVT_BUTTON, &PANEL_KICAD_LAUNCHER::onLauncherButtonClick, this );
    }

    Unbind( wxEVT_SYS_COLOUR_CHANGED, wxSysColourChangedEventHandler( PANEL_KICAD_LAUNCHER::onThemeChanged ), this );
}


void PANEL_KICAD_LAUNCHER::onLauncherButtonClick( wxCommandEvent& aEvent )
{
    // Don't accept clicks processed during wxProgressReporter updating.  In particular, the wxSafeYield()
    // call below will puke.
    if( m_frame->GetToolManager()->GetTool<KICAD_MANAGER_CONTROL>()->InShowPlayer() )
        return;

    // Defocus the button because leaving the large buttons focused after a click looks out of place in
    // the launcher
    m_frame->SetFocus();

    // Gives a slice of time to update the button state (mandatory on GTK, useful on MSW to avoid some
    // cosmetic issues).
    wxSafeYield();

    BITMAP_BUTTON*     button = (BITMAP_BUTTON*) aEvent.GetEventObject();
    const TOOL_ACTION* action = static_cast<const TOOL_ACTION*>( button->GetClientData() );

    if( action == nullptr )
        return;

    OPT_TOOL_EVENT evt = action->MakeEvent();
    evt->SetHasPosition( false );
    m_frame->GetToolManager()->ProcessEvent( *evt );
}


void PANEL_KICAD_LAUNCHER::CreateLaunchers()
{
    m_frame->SetPcmButton( nullptr );

    if( m_toolsSizer->GetEffectiveRowsCount() > 0 )
    {
        m_toolsSizer->Clear( true );
        m_toolsSizer->SetRows( 0 );
    }

    const UI_THEME::PALETTE& theme = UI_THEME::Get();

    SetBackgroundColour( theme.bg );
    m_scrolledWindow->SetBackgroundColour( theme.bg );

    // Comfortable outer margin around the whole column.
    if( wxSizerItem* item = m_scrolledWindow->GetSizer()->GetItem( m_toolsSizer ) )
        item->SetBorder( FromDIP( 16 ) );

    // One column of full-width rows (section labels, cards, the utilities grid).
    m_toolsSizer->SetCols( 1 );

    if( !m_toolsSizer->IsColGrowable( 0 ) )
        m_toolsSizer->AddGrowableCol( 0 );

    auto runAction =
            [this]( const TOOL_ACTION* aAction )
            {
                // Don't accept clicks processed during wxProgressReporter updating.
                if( m_frame->GetToolManager()->GetTool<KICAD_MANAGER_CONTROL>()->InShowPlayer() )
                    return;

                m_frame->SetFocus();

                OPT_TOOL_EVENT evt = aAction->MakeEvent();
                evt->SetHasPosition( false );
                m_frame->GetToolManager()->ProcessEvent( *evt );
            };

    auto makeCard =
            [&]( wxWindow* aParent, const TOOL_ACTION& aAction, const wxString& aMonogram,
                 const wxString& aHelp, bool aCompact, bool aEnabled = true ) -> LAUNCHER_CARD*
            {
                const TOOL_ACTION* action = &aAction;
                LAUNCHER_CARD*     card = new LAUNCHER_CARD( aParent, aMonogram, aAction.GetFriendlyName(),
                                                             aHelp, [runAction, action]() { runAction( action ); },
                                                             aCompact );
                card->Enable( aEnabled );
                return card;
            };

    const int gap = FromDIP( 8 );

    // Column header, same grammar as the recent projects column.
    m_toolsSizer->Add( CreateSectionLabel( m_scrolledWindow, _( "Tools" ) ), 0, wxTOP, FromDIP( 4 ) );
    m_toolsSizer->Add( CreateHomeTitle( m_scrolledWindow, _( "Open an Editor" ) ), 0, wxTOP, FromDIP( 14 ) );

    auto addSection =
            [&]( const wxString& aTitle )
            {
                m_toolsSizer->Add( CreateSectionLabel( m_scrolledWindow, aTitle ), 0,
                                   wxTOP | wxBOTTOM, FromDIP( 16 ) );
            };

    auto addRow =
            [&]( const TOOL_ACTION& aAction, const wxString& aMonogram, const wxString& aHelp )
            {
                m_toolsSizer->Add( makeCard( m_scrolledWindow, aAction, aMonogram, aHelp, false ), 0,
                                   wxEXPAND | wxBOTTOM, gap );
            };

    addSection( _( "Design" ) );
    addRow( KICAD_MANAGER_ACTIONS::editSchematic, _( "SC" ), _( "Edit the project schematic" ) );
    addRow( KICAD_MANAGER_ACTIONS::editPCB, _( "PC" ), _( "Edit the project PCB design" ) );

    addSection( _( "Libraries" ) );
    addRow( KICAD_MANAGER_ACTIONS::editSymbols, _( "SY" ),
            _( "Edit global and/or project schematic symbol libraries" ) );
    addRow( KICAD_MANAGER_ACTIONS::editFootprints, _( "FP" ),
            _( "Edit global and/or project PCB footprint libraries" ) );
    addRow( KICAD_MANAGER_ACTIONS::editDrawingSheet, _( "DS" ),
            _( "Edit drawing sheet borders and title blocks" ) );

    addSection( _( "Manufacturing and Utilities" ) );

    // Utilities: two-column grid of compact cards
    wxGridSizer* grid = new wxGridSizer( 2, gap, gap );

    grid->Add( makeCard( m_scrolledWindow, KICAD_MANAGER_ACTIONS::viewGerbers, _( "GB" ),
                         _( "Preview Gerber files" ), true ), 1, wxEXPAND );
    grid->Add( makeCard( m_scrolledWindow, KICAD_MANAGER_ACTIONS::showCalculator, _( "CA" ),
                         _( "Resistance, current capacity, etc." ), true ), 1, wxEXPAND );
    grid->Add( makeCard( m_scrolledWindow, KICAD_MANAGER_ACTIONS::convertImage, _( "IM" ),
                         _( "Bitmaps to symbols or footprints" ), true ), 1, wxEXPAND );

    LAUNCHER_CARD* pcm = makeCard( m_scrolledWindow, KICAD_MANAGER_ACTIONS::showPluginManager, _( "PM" ),
                                   _( "KiCad and third-party packages" ), true,
                                   KIPLATFORM::POLICY::GetPolicyBool( POLICY_KEY_PCM )
                                           != KIPLATFORM::POLICY::PBOOL::DISABLED );
    grid->Add( pcm, 1, wxEXPAND );

    m_toolsSizer->Add( grid, 0, wxEXPAND | wxBOTTOM, gap );

    m_frame->SetPcmButton( pcm );

    m_scrolledWindow->FitInside();
    Layout();
}


void PANEL_KICAD_LAUNCHER::onThemeChanged( wxSysColourChangedEvent& aEvent )
{
    GetBitmapStore()->ThemeChanged();
    CreateLaunchers();

    aEvent.Skip();
}
