/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2015-2016 Wayne Stambaugh <stambaughw@gmail.com>
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
 * @file footprint_edit_frame_wx.cpp
 * The parts of FOOTPRINT_EDIT_FRAME with wxWidgets: the layout of its panes and its events.
 */

#include <footprint_edit_frame.h>
#include <footprint_editor_settings.h>
#include <id.h>
#include <pcbnew_id.h>
#include <tool/action_toolbar.h>
#include <tool/tool_dispatcher.h>
#include <widgets/appearance_controls.h>
#include <widgets/panel_selection_filter.h>
#include <widgets/pcb_properties_panel.h>
#include <widgets/wx_aui_utils.h>


void FOOTPRINT_EDIT_FRAME::setupLayout()
{
    m_auimgr.SetManagedWindow( this );

    unsigned int auiFlags = wxAUI_MGR_DEFAULT;
#if !defined( _WIN32 )
    // Windows cannot redraw the UI fast enough during a live resize and may lead to all kinds
    // of graphical glitches
    auiFlags |= wxAUI_MGR_LIVE_RESIZE;
#endif
    m_auimgr.SetFlags( auiFlags );

    // Rows; layers 4 - 6
    m_auimgr.AddPane( m_tbTopMain, EDA_PANE().HToolbar().Name( "TopMainToolbar" )
                      .Top().Layer( 6 ) );

    m_auimgr.AddPane( m_messagePanel, EDA_PANE().Messages().Name( "MsgPanel" )
                      .Bottom().Layer( 6 ) );

    // Columns; layers 1 - 3
    m_auimgr.AddPane( m_treePane, EDA_PANE().Palette().Name( "Footprints" )
                      .Left().Layer( 4 )
                      .Caption( _( "Libraries" ) )
                      // Don't use -1 for don't-change-height on a growable panel; it has side-effects.
                      .MinSize( FromDIP( 250 ), FromDIP( 80 ) )
                      .BestSize( FromDIP( 250 ), -1 ) );
    m_auimgr.AddPane( m_propertiesPanel, EDA_PANE().Name( PropertiesPaneName() )
                      .Left().Layer( 3 )
                      .Caption( _( "Properties" ) ).PaneBorder( false )
                      .MinSize( FromDIP( wxSize( 240, 60 ) ) ).BestSize( FromDIP( wxSize( 300, 200 ) ) ) );
    m_auimgr.AddPane( m_tbLeft, EDA_PANE().VToolbar().Name( "LeftToolbar" )
                      .Left().Layer( 2 ) );

    m_auimgr.AddPane( m_tbRight, EDA_PANE().VToolbar().Name( "RightToolbar" )
                      .Right().Layer(2) );
    m_auimgr.AddPane( m_appearancePanel, EDA_PANE().Name( "LayersManager" )
                      .Right().Layer( 3 )
                      .Caption( _( "Appearance" ) ).PaneBorder( false )
                      // Don't use -1 for don't-change-height on a growable panel; it has side-effects.
                      .MinSize( FromDIP( 180 ), FromDIP( 80 ) )
                      .BestSize( FromDIP( 180 ), -1 ) );
    m_auimgr.AddPane( m_selectionFilterPanel, EDA_PANE().Palette().Name( "SelectionFilter" )
                      .Right().Layer( 3 ).Position( 2 )
                      .Caption( _( "Selection Filter" ) ).PaneBorder( false )
                      // Fixed-size pane; -1 for MinSize height is required
                      .MinSize( FromDIP( 180 ), -1 )
                      .BestSize( FromDIP( 180 ), -1 ) );

    // Center
    m_auimgr.AddPane( GetCanvas(), EDA_PANE().Canvas().Name( "DrawFrame" )
                      .Center() );

    RestoreAuiLayout();

    m_auimgr.GetPane( "LayersManager" ).Show( m_show_layer_manager_tools );
    m_auimgr.GetPane( "SelectionFilter" ).Show( m_show_layer_manager_tools );
    m_auimgr.GetPane( PropertiesPaneName() ).Show( GetSettings()->m_AuiPanels.show_properties );

    // The selection filter doesn't need to grow in the vertical direction when docked
    m_auimgr.GetPane( "SelectionFilter" ).dock_proportion = 0;
    FinishAUIInitialization();
}


void FOOTPRINT_EDIT_FRAME::setupToolkitEvents()
{
    Bind( wxEVT_MENU, [this]( wxCommandEvent& ) { CloseFootprintEditor(); }, wxID_CLOSE );
    Bind( wxEVT_MENU, [this]( wxCommandEvent& ) { OnExitKiCad(); }, wxID_EXIT );

    Bind( wxEVT_SIZE, &FOOTPRINT_EDIT_FRAME::OnSize, this );

    Bind( wxEVT_CHOICE, &FOOTPRINT_EDIT_FRAME::OnSelectZoom, this, ID_ON_ZOOM_SELECT );
    Bind( wxEVT_CHOICE, &FOOTPRINT_EDIT_FRAME::OnSelectGrid, this, ID_ON_GRID_SELECT );

    Bind( wxEVT_TOOL, [this]( wxCommandEvent& ) { OnSaveFootprintAsPng(); }, ID_FPEDIT_SAVE_PNG );

    // Drop files event
    Bind( wxEVT_DROP_FILES, &FOOTPRINT_EDIT_FRAME::OnDropFiles, this );

    // Catch unhandled accelerator command characters that were no handled by the library tree
    // panel.
    Bind( wxEVT_CHAR, &TOOL_DISPATCHER::DispatchWxEvent, m_toolDispatcher );
    Bind( wxEVT_CHAR_HOOK, &TOOL_DISPATCHER::DispatchWxEvent, m_toolDispatcher );
}
