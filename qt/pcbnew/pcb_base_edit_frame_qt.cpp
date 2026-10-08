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

/**
 * @file pcb_base_edit_frame_qt.cpp
 * The parts of PCB_BASE_EDIT_FRAME with Qt: the update of the tools after the events, the
 * layer selector of the toolbars and the vertex editor pane (see
 * pcbnew/pcb_base_edit_frame_wx.cpp).
 */

#include <pcb_base_edit_frame.h>

#include <QApplication>
#include <QComboBox>
#include <QDockWidget>

#include <background_jobs_monitor.h>
#include <class_draw_panel_gal.h>
#include <pcb_layer_box_selector.h>
#include <pcbnew_settings.h>
#include <pgm_gui.h>
#include <qt_strings.h>
#include <tool/action_toolbar.h>
#include <tool/tool_manager.h>
#include <tools/pcb_selection_tool.h>
#include <ui_hooks.h>
#include <widgets/kistatusbar.h>
#include <widgets/vertex_editor_pane.h>
#include <id.h>


void PCB_BASE_EDIT_FRAME::setupToolkitEvents()
{
    // Nothing to follow without the GUI (headless mode)
    if( !qobject_cast<QApplication*>( QCoreApplication::instance() ) )
        return;

    // Handle cursor adjustments.  The modifier keys released outside of the canvas don't make
    // events of the canvas: the selection tool checks them after the events.
    KIQT::RegisterUpdateUI( this,
                            [this]()
                            {
                                if( m_toolManager )
                                {
                                    if( PCB_SELECTION_TOOL* selTool =
                                                m_toolManager->GetTool<PCB_SELECTION_TOOL>() )
                                    {
                                        selTool->OnIdle();
                                    }
                                }
                            } );

    if( GetStatusBar() )
        PgmGui().GetBackgroundJobMonitor().RegisterStatusBar( GetStatusBar() );
}


void PCB_BASE_EDIT_FRAME::releaseToolkitEvents()
{
    if( GetStatusBar() )
        PgmGui().GetBackgroundJobMonitor().UnregisterStatusBar( GetStatusBar() );
}


void PCB_BASE_EDIT_FRAME::addLayerSelector( ACTION_TOOLBAR* aToolbar )
{
    if( !m_SelLayerBox )
    {
        m_SelLayerBox = new PCB_LAYER_BOX_SELECTOR( aToolbar, ID_ON_LAYER_SELECT );
        m_SelLayerBox->SetBoardFrame( this );

        // The selection follows the active layer of the frame
        KIQT::RegisterUpdateUI( m_SelLayerBox,
                                [this]()
                                {
                                    if( m_SelLayerBox->count()
                                        && m_SelLayerBox->GetLayerSelection() != GetActiveLayer() )
                                    {
                                        m_SelLayerBox->SetLayerSelection( GetActiveLayer() );
                                    }
                                } );

        // The user chose a layer in the list
        QObject::connect( m_SelLayerBox, &QComboBox::activated, m_SelLayerBox,
                          [this]( int )
                          {
                              SetActiveLayer( ToLAYER_ID( m_SelLayerBox->GetLayerSelection() ) );

                              if( GetDisplayOptions().m_ContrastModeDisplay
                                  != HIGH_CONTRAST_MODE::NORMAL )
                              {
                                  GetCanvas()->Refresh();
                              }
                          } );
    }

    m_SelLayerBox->setToolTip( ToQString( _( "+/- to switch" ) ) );
    m_SelLayerBox->Resync();

    aToolbar->Add( m_SelLayerBox );
}


void PCB_BASE_EDIT_FRAME::OpenVertexEditor( BOARD_ITEM* aItem )
{
    if( !m_vertexEditorPane )
    {
        m_vertexEditorPane = new PCB_VERTEX_EDITOR_PANE( this );

        QDockWidget* dock = addPane( m_vertexEditorPane, VertexEditorPaneName(),
                                     _( "Edit Vertices" ), PANE_DOCK::NONE, true );

        // The pane is deleted when it is closed (as with the DestroyOnClose() of wxAUI); the
        // pane tells the frame (OnVertexEditorPaneClosed())
        dock->setAttribute( Qt::WA_DeleteOnClose );
        dock->setFloating( true );
        m_vertexEditorPane->setMinimumSize( 260, 200 );
        dock->resize( 320, 360 );
        dock->show();
    }

    m_vertexEditorPane->SetItem( aItem );
    m_vertexEditorPane->SetFocus();
}
