/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

// The parts of PCB_BASE_EDIT_FRAME with wxWidgets: the events, the layer selector of the
// toolbars and the vertex editor pane.

#include <pcb_base_edit_frame.h>

#include <wx/app.h>

#include <background_jobs_monitor.h>
#include <pgm_gui.h>
#include <pcb_layer_box_selector.h>
#include <tool/action_toolbar.h>
#include <tool/tool_manager.h>
#include <tools/pcb_selection_tool.h>
#include <widgets/kistatusbar.h>
#include <widgets/vertex_editor_pane.h>
#include <id.h>


void PCB_BASE_EDIT_FRAME::setupToolkitEvents()
{
    // Do not register the idle event handler if we are running in headless mode.
    if( !wxApp::GetGUIInstance() )
        return;

    Bind( wxEVT_IDLE,
          [this]( wxIdleEvent& aEvent )
          {
              // Handle cursor adjustments.  While we can get motion and key events through
              // wxWidgets, we can't get modifier-key-up events.
              if( m_toolManager )
              {
                  PCB_SELECTION_TOOL* selTool = m_toolManager->GetTool<PCB_SELECTION_TOOL>();

                  if( selTool )
                      selTool->OnIdle();
              }
          } );

    PgmGui().GetBackgroundJobMonitor().RegisterStatusBar( static_cast<KISTATUSBAR*>( GetStatusBar() ) );
}


void PCB_BASE_EDIT_FRAME::releaseToolkitEvents()
{
    PgmGui().GetBackgroundJobMonitor().UnregisterStatusBar( static_cast<KISTATUSBAR*>( GetStatusBar() ) );
}


void PCB_BASE_EDIT_FRAME::addLayerSelector( ACTION_TOOLBAR* aToolbar )
{
    if( !m_SelLayerBox )
    {
        m_SelLayerBox = new PCB_LAYER_BOX_SELECTOR( aToolbar, ID_ON_LAYER_SELECT );
        m_SelLayerBox->SetBoardFrame( this );
    }

    m_SelLayerBox->SetToolTip( _( "+/- to switch" ) );
    m_SelLayerBox->Resync();

    aToolbar->Add( m_SelLayerBox );

    // UI update handler for the control
    aToolbar->Bind( wxEVT_UPDATE_UI,
                    [this]( wxUpdateUIEvent& aEvent )
                        {
                            if( m_SelLayerBox->GetCount()
                                && ( m_SelLayerBox->GetLayerSelection() != GetActiveLayer() ) )
                            {
                                m_SelLayerBox->SetLayerSelection( GetActiveLayer() );
                            }
                        },
                    m_SelLayerBox->GetId() );

    // Event handler to respond to the user interacting with the control
    aToolbar->Bind( wxEVT_COMBOBOX,
                    [this]( wxCommandEvent& aEvent )
                        {
                            SetActiveLayer( ToLAYER_ID( m_SelLayerBox->GetLayerSelection() ) );

                            if( GetDisplayOptions().m_ContrastModeDisplay != HIGH_CONTRAST_MODE::NORMAL )
                                GetCanvas()->Refresh();
                        },
                    m_SelLayerBox->GetId() );
}


void PCB_BASE_EDIT_FRAME::OpenVertexEditor( BOARD_ITEM* aItem )
{
    if( !m_vertexEditorPane )
    {
        m_vertexEditorPane = new PCB_VERTEX_EDITOR_PANE( this );

        wxAuiPaneInfo paneInfo = EDA_PANE().Name( VertexEditorPaneName() )
                                       .Float()
                                       .Caption( _( "Edit Vertices" ) )
                                       .PaneBorder( true )
                                       .CloseButton( true )
                                       .DestroyOnClose( true )
                                       .Resizable( true )
                                       .MinSize( FromDIP( wxSize( 260, 200 ) ) )
                                       .BestSize( FromDIP( wxSize( 260, 320 ) ) )
                                       .FloatingSize( FromDIP( wxSize( 320, 360 ) ) );

        m_auimgr.AddPane( m_vertexEditorPane, paneInfo );
        m_auimgr.Update();
    }

    m_vertexEditorPane->SetItem( aItem );
    m_vertexEditorPane->SetFocus();
}
