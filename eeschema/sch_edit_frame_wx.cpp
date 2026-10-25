/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file sch_edit_frame_wx.cpp
 * The parts of SCH_EDIT_FRAME with wxWidgets: the layout of its panes and its events.
 */

#include <wx/socket.h>

#include <eeschema_settings.h>
#include <id.h>
#include <sch_edit_frame.h>
#include <sch_pane_info.h>
#include <tool/action_toolbar.h>
#include <tools/sch_selection_tool.h>
#include <widgets/hierarchy_pane.h>
#include <widgets/panel_remote_symbol.h>
#include <widgets/panel_sch_selection_filter.h>
#include <widgets/sch_design_block_pane.h>
#include <widgets/sch_properties_panel.h>
#include <widgets/sch_search_pane.h>
#include <widgets/wx_aui_utils.h>


void SCH_EDIT_FRAME::setupLayout()
{
    m_auimgr.SetManagedWindow( this );

    CreateInfoBar();

    // Fetch a COPY of the config as a lot of these initializations are going to overwrite our
    // data.
    EESCHEMA_SETTINGS::AUI_PANELS aui_cfg = eeconfig()->m_AuiPanels;

    // Rows; layers 4 - 6
    m_auimgr.AddPane( m_tbTopMain, EDA_PANE().HToolbar().Name( wxS( "TopMainToolbar" ) )
                      .Top().Layer( 6 ) );

    m_auimgr.AddPane( m_messagePanel, EDA_PANE().Messages().Name( wxS( "MsgPanel" ) )
                      .Bottom().Layer( 6 ) );

    // Columns; layers 1 - 3
    m_auimgr.AddPane( m_hierarchy, EDA_PANE().Palette().Name( SchematicHierarchyPaneName() )
                      .Caption( _( "Schematic Hierarchy" ) )
                      .Left().Layer( 3 ).Position( 1 )
                      .TopDockable( false )
                      .BottomDockable( false )
                      .CloseButton( true )
                      .MinSize( FromDIP( wxSize( 120, 60 ) ) )
                      .BestSize( FromDIP( wxSize( 200, 200 ) ) )
                      .FloatingSize( FromDIP( wxSize( 200, 200 ) ) )
                      .FloatingPosition( FromDIP( wxPoint( 50, 50 ) ) )
                      .Show( false ) );

    m_auimgr.AddPane( m_propertiesPanel, defaultPropertiesPaneInfo( this ) );
    m_auimgr.AddPane( m_selectionFilterPanel, defaultSchSelectionFilterPaneInfo( this ) );

    m_auimgr.AddPane( m_designBlocksPane, defaultDesignBlocksPaneInfo( this ) );
    m_auimgr.AddPane( m_remoteSymbolPane, defaultRemoteSymbolPaneInfo( this ) );

    m_auimgr.AddPane( createHighlightedNetNavigator(), defaultNetNavigatorPaneInfo() );

    m_auimgr.AddPane( m_tbLeft, EDA_PANE().VToolbar().Name( wxS( "LeftToolbar" ) )
                      .Left().Layer( 2 ) );

    m_auimgr.AddPane( m_tbRight, EDA_PANE().VToolbar().Name( wxS( "RightToolbar" ) )
                      .Right().Layer( 2 ) );

    // Center
    m_auimgr.AddPane( GetCanvas(), EDA_PANE().Canvas().Name( wxS( "DrawFrame" ) )
                      .Center() );

    m_auimgr.AddPane( m_searchPane, EDA_PANE()
                      .Name( SearchPaneName() )
                      .Bottom()
                      .Caption( _( "Search" ) )
                      .PaneBorder( false )
                      .MinSize( FromDIP( wxSize( 180, 60 ) ) )
                      .BestSize( FromDIP( wxSize( 180, 100 ) ) )
                      .FloatingSize( FromDIP( wxSize( 480, 200 ) ) )
                      .CloseButton( true )
                      .DestroyOnClose( false )
                      .Show( m_show_search ) );

    RestoreAuiLayout();
    FinishAUIInitialization();

    wxAuiPaneInfo& hierarchy_pane = m_auimgr.GetPane( SchematicHierarchyPaneName() );
    wxAuiPaneInfo& netNavigatorPane = m_auimgr.GetPane( NetNavigatorPaneName() );
    wxAuiPaneInfo& propertiesPane = m_auimgr.GetPane( PropertiesPaneName() );
    wxAuiPaneInfo& selectionFilterPane = m_auimgr.GetPane( wxS( "SelectionFilter" ) );
    wxAuiPaneInfo& designBlocksPane = m_auimgr.GetPane( DesignBlocksPaneName() );
    wxAuiPaneInfo& remoteSymbolPane = m_auimgr.GetPane( RemoteSymbolPaneName() );

    hierarchy_pane.Show( aui_cfg.show_schematic_hierarchy );
    netNavigatorPane.Show( aui_cfg.show_net_nav_panel );
    propertiesPane.Show( aui_cfg.show_properties );
    designBlocksPane.Show( aui_cfg.design_blocks_show );

    if( m_remoteSymbolPane && !m_remoteSymbolPane->HasDataSources() )
        remoteSymbolPane.Show( false );
    else
        remoteSymbolPane.Show( aui_cfg.remote_symbol_show );

    updateSelectionFilterVisbility();

    // The selection filter doesn't need to grow in the vertical direction when docked
    selectionFilterPane.dock_proportion = 0;

    if( aui_cfg.hierarchy_panel_float_width > 0 && aui_cfg.hierarchy_panel_float_height > 0 )
    {
        // Show at end, after positioning
        hierarchy_pane.FloatingSize( aui_cfg.hierarchy_panel_float_width,
                                     aui_cfg.hierarchy_panel_float_height );
    }

    if( aui_cfg.net_nav_panel_float_size.GetWidth() > 0
      && aui_cfg.net_nav_panel_float_size.GetHeight() > 0 )
    {
        netNavigatorPane.FloatingSize( aui_cfg.net_nav_panel_float_size );
        netNavigatorPane.FloatingPosition( aui_cfg.net_nav_panel_float_pos );
    }

    if( aui_cfg.properties_panel_width > 0 )
        SetAuiPaneSize( m_auimgr, propertiesPane, aui_cfg.properties_panel_width, -1 );

    if( aui_cfg.schematic_hierarchy_float )
        hierarchy_pane.Float();

    if( aui_cfg.search_panel_height > 0 && (   aui_cfg.search_panel_dock_direction == wxAUI_DOCK_TOP
                                            || aui_cfg.search_panel_dock_direction == wxAUI_DOCK_BOTTOM ) )
    {
        wxAuiPaneInfo& searchPane = m_auimgr.GetPane( SearchPaneName() );
        searchPane.Direction( aui_cfg.search_panel_dock_direction );
        SetAuiPaneSize( m_auimgr, searchPane, -1, aui_cfg.search_panel_height );
    }

    else if( aui_cfg.search_panel_width > 0 && (   aui_cfg.search_panel_dock_direction == wxAUI_DOCK_LEFT
                                                || aui_cfg.search_panel_dock_direction == wxAUI_DOCK_RIGHT ) )
    {
        wxAuiPaneInfo& searchPane = m_auimgr.GetPane( SearchPaneName() );
        searchPane.Direction( aui_cfg.search_panel_dock_direction );
        SetAuiPaneSize( m_auimgr, searchPane, aui_cfg.search_panel_width, -1 );
    }

    if( aui_cfg.float_net_nav_panel )
        netNavigatorPane.Float();

    if( aui_cfg.design_blocks_show )
        SetAuiPaneSize( m_auimgr, designBlocksPane, aui_cfg.design_blocks_panel_docked_width, -1 );

    if( aui_cfg.remote_symbol_show )
    {
        SetAuiPaneSize( m_auimgr, remoteSymbolPane, aui_cfg.remote_symbol_panel_docked_width, -1 );
    }

    if( aui_cfg.hierarchy_panel_docked_width > 0 )
    {
        // If the net navigator is not show, let the hierarchy navigator take all of the vertical
        // space.
        if( !aui_cfg.show_net_nav_panel )
        {
            SetAuiPaneSize( m_auimgr, hierarchy_pane, aui_cfg.hierarchy_panel_docked_width, -1 );
        }
        else
        {
            SetAuiPaneSize( m_auimgr, hierarchy_pane,
                            aui_cfg.hierarchy_panel_docked_width,
                            aui_cfg.hierarchy_panel_docked_height );

            SetAuiPaneSize( m_auimgr, netNavigatorPane,
                            aui_cfg.net_nav_panel_docked_size.GetWidth(),
                            aui_cfg.net_nav_panel_docked_size.GetHeight() );
        }

        // wxAUI hack: force width by setting MinSize() and then Fixed()
        // thanks to ZenJu https://github.com/wxWidgets/wxWidgets/issues/13180
        hierarchy_pane.MinSize( aui_cfg.hierarchy_panel_docked_width, 60 );
        hierarchy_pane.Fixed();
        netNavigatorPane.MinSize( aui_cfg.net_nav_panel_docked_size.GetWidth(), 60 );
        netNavigatorPane.Fixed();
        m_auimgr.Update();

        // now make it resizable again
        hierarchy_pane.Resizable();
        netNavigatorPane.Resizable();
        m_auimgr.Update();

        // Note: DO NOT call m_auimgr.Update() anywhere after this; it will nuke the size
        // back to minimum.
        hierarchy_pane.MinSize( FromDIP( wxSize( 120, 60 ) ) );
        netNavigatorPane.MinSize( FromDIP( wxSize( 120, 60 ) ) );
    }
    else
    {
        m_auimgr.Update();
    }
}


void SCH_EDIT_FRAME::setupToolkitEvents()
{
    Bind( wxEVT_SOCKET, &EDA_DRAW_FRAME::OnSockRequestServer, this, ID_EDA_SOCKET_EVENT_SERV );
    Bind( wxEVT_SOCKET, &EDA_DRAW_FRAME::OnSockRequest, this, ID_EDA_SOCKET_EVENT );

    Bind( wxEVT_SIZE, &SCH_EDIT_FRAME::OnSize, this );

    Bind( wxEVT_MENU, &SCH_EDIT_FRAME::OnLoadFile, this, ID_FILE1, ID_FILEMAX );
    Bind( wxEVT_MENU, &SCH_EDIT_FRAME::OnClearFileHistory, this, ID_FILE_LIST_CLEAR );

    Bind( wxEVT_MENU, &SCH_EDIT_FRAME::OnExit, this, wxID_EXIT );
    Bind( wxEVT_MENU, &SCH_EDIT_FRAME::OnExit, this, wxID_CLOSE );

    Bind( wxEVT_CHOICE, &SCH_EDIT_FRAME::OnSelectZoom, this, ID_ON_ZOOM_SELECT );
    Bind( wxEVT_CHOICE, &SCH_EDIT_FRAME::OnSelectGrid, this, ID_ON_GRID_SELECT );

    // Drop files event
    Bind( wxEVT_DROP_FILES, &SCH_EDIT_FRAME::OnDropFiles, this );
}
