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
 * @file pcb_design_block_preview_widget_wx.cpp
 * The widgets of PCB_DESIGN_BLOCK_PREVIEW_WIDGET with wxWidgets: the footprint preview canvas and
 * the status panel (the Qt build has its own version).  What the widget does is in
 * pcb_design_block_preview_widget.cpp.
 */

#include <board.h>
#include <confirm.h>
#include <pcb_view.h>
#include <pcb_screen.h>
#include <gal/gal_display_options.h>
#include <gal/graphics_abstraction_layer.h>
#include <math/vector2wx.h>
#include <design_block.h>
#include <footprint_preview_panel.h>
#include <pgm_base.h>
#include <pcb_painter.h>
#include <pcb_edit_frame.h>
#include <pcb_field.h>
#include <pcb_io/pcb_io.h>
#include <pcb_io/pcb_io_mgr.h>
#include <project_pcb.h>
#include <pcbnew_settings.h>
//#include <pcb_helpers.h>
#include <settings/settings_manager.h>
#include <widgets/pcb_design_block_preview_widget.h>
#include <widgets/wx_progress_reporters.h>
#include <wx/log.h>
#include <wx/stattext.h>
#include <wx/panel.h>
#include <core/ki_assert.h>



PCB_DESIGN_BLOCK_PREVIEW_WIDGET::PCB_DESIGN_BLOCK_PREVIEW_WIDGET( wxWindow* aParent, PCB_EDIT_FRAME* aFrame ) :
        DESIGN_BLOCK_PREVIEW_WIDGET( aParent ), m_preview( nullptr ), m_status( nullptr ), m_statusPanel( nullptr ),
        m_statusSizer( nullptr ), m_previewItem( nullptr )
{
    COMMON_SETTINGS* common_settings = Pgm().GetCommonSettings();
    PCBNEW_SETTINGS* cfg = GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" );

    m_galDisplayOptions.ReadConfig( *common_settings, cfg->m_Window, this );
    m_galDisplayOptions.m_forceDisplayCursor = false;

    m_preview = FOOTPRINT_PREVIEW_PANEL::New( &aFrame->Kiway(), this, aFrame );
    m_preview->SetStealsFocus( false );
    m_preview->ShowScrollbars( wxSHOW_SB_NEVER, wxSHOW_SB_NEVER );
    m_preview->GetGAL()->SetAxesEnabled( false );

    // Do not display the grid: the look is not good for a small canvas area.
    // But mainly, due to some strange bug I (JPC) was unable to fix, the grid creates
    // strange artifacts on Windows when Pcb is run from KiCad manager (but not in
    // stand alone...).
    m_preview->GetGAL()->SetGridVisibility( true );

    // Early initialization of the canvas background color,
    // before any OnPaint event is fired for the canvas using a wrong bg color
    KIGFX::VIEW* view = m_preview->GetView();
    auto         settings = static_cast<KIGFX::PCB_RENDER_SETTINGS*>( view->GetPainter()->GetSettings() );

    if( COLOR_SETTINGS* cs = ::GetColorSettings( cfg ? cfg->m_ColorTheme : DEFAULT_THEME ) )
        settings->LoadColors( cs );

    const COLOR4D& backgroundColor = settings->GetBackgroundColor();
    const COLOR4D& foregroundColor = settings->GetCursorColor();

    m_preview->GetGAL()->SetClearColor( backgroundColor );

    m_outerSizer = new wxBoxSizer( wxVERTICAL );

    m_statusPanel = new wxPanel( this );
    m_statusPanel->SetBackgroundColour( backgroundColor.ToColour() );
    m_status = new wxStaticText( m_statusPanel, wxID_ANY, wxEmptyString );
    m_status->SetForegroundColour( settings->GetLayerColor( LAYER_REFERENCEPART ).ToColour() );
    m_statusSizer = new wxBoxSizer( wxVERTICAL );
    m_statusSizer->Add( 0, 0, 1 ); // add a spacer
    m_statusSizer->Add( m_status, 0, wxALIGN_CENTER );
    m_statusSizer->Add( 0, 0, 1 ); // add a spacer
    m_statusPanel->SetSizer( m_statusSizer );

    // Give the status panel the same color scheme as the canvas so it isn't jarring when
    // switched to.
    m_statusPanel->SetBackgroundColour( backgroundColor.ToColour() );
    m_statusPanel->SetForegroundColour( foregroundColor.ToColour() );

    // Give the preview panel a small top border to align its top with the status panel,
    // and give the status panel a small bottom border to align its bottom with the preview
    // panel.
    m_outerSizer->Add( m_preview, 1, wxTOP | wxEXPAND, 5 );
    m_outerSizer->Add( m_statusPanel, 1, wxBOTTOM | wxEXPAND, 5 );

    // Hide the status panel to start
    m_statusPanel->Hide();

    SetSizer( m_outerSizer );
    Layout();

    Connect( wxEVT_SIZE, wxSizeEventHandler( PCB_DESIGN_BLOCK_PREVIEW_WIDGET::onSize ), nullptr, this );
}


void PCB_DESIGN_BLOCK_PREVIEW_WIDGET::SetStatusText( wxString const& aText )
{
    KI_CHECK( m_statusPanel, /* void */ );

    m_status->SetLabel( aText );
    m_preview->Hide();
    m_statusPanel->Show();
    Layout();
}


void PCB_DESIGN_BLOCK_PREVIEW_WIDGET::onSize( wxSizeEvent& aEvent )
{
    if( m_previewItem )
    {
        fitOnDrawArea();
        m_preview->ForceRefresh();
    }

    aEvent.Skip();
}


BOARD* PCB_DESIGN_BLOCK_PREVIEW_WIDGET::loadBoard( const wxString& aFileName )
{
    IO_RELEASER<PCB_IO>  pi( PCB_IO_MGR::FindPlugin( PCB_IO_MGR::KICAD_SEXP ) );
    WX_PROGRESS_REPORTER progressReporter( this, _( "Load PCB" ), 1, PR_CAN_ABORT );

    pi->SetProgressReporter( &progressReporter );

    return pi->LoadBoard( aFileName, nullptr );
}


void PCB_DESIGN_BLOCK_PREVIEW_WIDGET::showCanvas()
{
    if( !m_preview->IsShownOnScreen() )
    {
        m_preview->Show();

        if( m_statusPanel )
            m_statusPanel->Hide();

        Layout(); // Ensure panel size is up to date.
    }
}
