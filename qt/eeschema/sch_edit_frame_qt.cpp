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
 * @file sch_edit_frame_qt.cpp
 * The parts of SCH_EDIT_FRAME with Qt: the layout of its panes and its events (see
 * eeschema/sch_edit_frame_wx.cpp).
 *
 * The remote commands of the standalone editors are received by KIWAY_PLAYER::CreateServer(),
 * the dropped files by EDA_BASE_FRAME::dropEvent() and the choices of the zoom and of the grid
 * by EDA_DRAW_FRAME: the frame has no events of its own for them.
 */

#include <QDockWidget>

#include <eeschema_settings.h>
#include <id.h>
#include <sch_edit_frame.h>
#include <tools/sch_selection_tool.h>
#include <widgets/hierarchy_pane.h>
#include <widgets/panel_remote_symbol.h>
#include <widgets/panel_sch_selection_filter.h>
#include <widgets/sch_design_block_pane.h>
#include <widgets/sch_properties_panel.h>
#include <widgets/sch_search_pane.h>


void SCH_EDIT_FRAME::setupLayout()
{
    CreateInfoBar();

    // The canvas, the info bar and the message panel
    setupCentralArea();

    // Fetch a COPY of the config as a lot of these initializations are going to overwrite our
    // data.
    EESCHEMA_SETTINGS::AUI_PANELS aui_cfg = eeconfig()->m_AuiPanels;

    // The panels docked on the left side, one above the other: the net navigator, the
    // hierarchy, the properties and the selection filter
    QDockWidget* netNavigator = addPane( createHighlightedNetNavigator()->GetHandle(),
                                         NetNavigatorPaneName(), _( "Net Navigator" ),
                                         PANE_DOCK::LEFT, aui_cfg.show_net_nav_panel );
    QDockWidget* hierarchy = addPane( m_hierarchy, SchematicHierarchyPaneName(),
                                      _( "Schematic Hierarchy" ), PANE_DOCK::LEFT,
                                      aui_cfg.show_schematic_hierarchy );
    QDockWidget* properties = addPane( m_propertiesPanel, PropertiesPaneName(), _( "Properties" ),
                                       PANE_DOCK::LEFT, aui_cfg.show_properties );
    QDockWidget* selectionFilter = addPane( m_selectionFilterPanel, wxS( "SelectionFilter" ),
                                            _( "Selection Filter" ), PANE_DOCK::LEFT );

    splitDockWidget( netNavigator, hierarchy, Qt::Vertical );
    splitDockWidget( hierarchy, properties, Qt::Vertical );
    splitDockWidget( properties, selectionFilter, Qt::Vertical );

    // The design blocks and the remote symbols on the right side
    QDockWidget* designBlocks = addPane( m_designBlocksPane, DesignBlocksPaneName(),
                                         _( "Design Blocks" ), PANE_DOCK::RIGHT,
                                         aui_cfg.design_blocks_show );
    QDockWidget* remoteSymbols = addPane( m_remoteSymbolPane, RemoteSymbolPaneName(),
                                          _( "Remote Symbols" ), PANE_DOCK::RIGHT,
                                          aui_cfg.remote_symbol_show
                                                  && m_remoteSymbolPane->HasDataSources() );

    splitDockWidget( designBlocks, remoteSymbols, Qt::Vertical );

    // The search on the bottom side (or on the side of the settings)
    QDockWidget* search = addPane( m_searchPane, SearchPaneName(), _( "Search" ),
                                   PANE_DOCK::BOTTOM, m_show_search );

    // None of them can be docked on the top and on the bottom sides, except the search
    for( QDockWidget* dock : { netNavigator, hierarchy, properties, selectionFilter, designBlocks,
                               remoteSymbols } )
    {
        dock->setAllowedAreas( Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea );
    }

    m_hierarchy->setMinimumSize( 120, 60 );
    m_propertiesPanel->setMinimumSize( 240, 60 );
    m_designBlocksPane->setMinimumSize( 240, 60 );
    m_remoteSymbolPane->setMinimumSize( 240, 60 );
    m_searchPane->setMinimumSize( 180, 60 );

    updateSelectionFilterVisbility();

    // The best sizes of the panels (as the wxWidgets build; the sizes of Qt are independent of
    // the DPI)
    resizeDocks( { hierarchy, designBlocks }, { 200, 300 }, Qt::Horizontal );
    resizeDocks( { search }, { 100 }, Qt::Vertical );

    // The selection filter doesn't need to grow in the vertical direction when docked
    resizeDocks( { selectionFilter }, { m_selectionFilterPanel->sizeHint().height() },
                 Qt::Vertical );

    // The sizes and the places of the settings
    if( aui_cfg.hierarchy_panel_float_width > 0 && aui_cfg.hierarchy_panel_float_height > 0 )
    {
        SetPaneFloatingSize( SchematicHierarchyPaneName(),
                             wxSize( aui_cfg.hierarchy_panel_float_width,
                                     aui_cfg.hierarchy_panel_float_height ) );
    }

    if( aui_cfg.net_nav_panel_float_size.GetWidth() > 0
        && aui_cfg.net_nav_panel_float_size.GetHeight() > 0 )
    {
        SetPaneFloatingSize( NetNavigatorPaneName(), aui_cfg.net_nav_panel_float_size );
        SetPaneFloatingPosition( NetNavigatorPaneName(), aui_cfg.net_nav_panel_float_pos );
    }

    if( aui_cfg.properties_panel_width > 0 )
        SetPaneSize( PropertiesPaneName(), aui_cfg.properties_panel_width, -1 );

    if( aui_cfg.schematic_hierarchy_float )
        hierarchy->setFloating( true );

    PANE_DOCK searchDock = static_cast<PANE_DOCK>( aui_cfg.search_panel_dock_direction );

    if( aui_cfg.search_panel_height > 0
        && ( searchDock == PANE_DOCK::TOP || searchDock == PANE_DOCK::BOTTOM ) )
    {
        SetPaneDockDirection( SearchPaneName(), aui_cfg.search_panel_dock_direction );
        SetPaneSize( SearchPaneName(), -1, aui_cfg.search_panel_height );
    }
    else if( aui_cfg.search_panel_width > 0
             && ( searchDock == PANE_DOCK::LEFT || searchDock == PANE_DOCK::RIGHT ) )
    {
        SetPaneDockDirection( SearchPaneName(), aui_cfg.search_panel_dock_direction );
        SetPaneSize( SearchPaneName(), aui_cfg.search_panel_width, -1 );
    }

    if( aui_cfg.float_net_nav_panel )
        netNavigator->setFloating( true );

    if( aui_cfg.design_blocks_show )
        SetPaneSize( DesignBlocksPaneName(), aui_cfg.design_blocks_panel_docked_width, -1 );

    if( aui_cfg.remote_symbol_show )
        SetPaneSize( RemoteSymbolPaneName(), aui_cfg.remote_symbol_panel_docked_width, -1 );

    if( aui_cfg.hierarchy_panel_docked_width > 0 )
    {
        // If the net navigator is not show, let the hierarchy navigator take all of the vertical
        // space.
        if( !aui_cfg.show_net_nav_panel )
        {
            SetPaneSize( SchematicHierarchyPaneName(), aui_cfg.hierarchy_panel_docked_width, -1 );
        }
        else
        {
            SetPaneSize( SchematicHierarchyPaneName(), aui_cfg.hierarchy_panel_docked_width,
                         aui_cfg.hierarchy_panel_docked_height );

            SetPaneSize( NetNavigatorPaneName(), aui_cfg.net_nav_panel_docked_size.GetWidth(),
                         aui_cfg.net_nav_panel_docked_size.GetHeight() );
        }
    }
}


void SCH_EDIT_FRAME::setupToolkitEvents()
{
    // The handlers take the event of the menu item
    auto bindCommand =
            [this]( int aId, void ( SCH_EDIT_FRAME::*aHandler )( wxCommandEvent& ) )
            {
                BindMenuCommand( aId,
                                 [this, aId, aHandler]()
                                 {
                                     wxCommandEvent event( wxEVT_MENU, aId );
                                     event.SetEventObject( this );
                                     ( this->*aHandler )( event );
                                 } );
            };

    // Menu Files:
    for( int id = ID_FILE1; id <= ID_FILEMAX; ++id )
        bindCommand( id, &SCH_EDIT_FRAME::OnLoadFile );

    bindCommand( ID_FILE_LIST_CLEAR, &SCH_EDIT_FRAME::OnClearFileHistory );

    for( int id : { wxID_EXIT, wxID_CLOSE } )
        bindCommand( id, &SCH_EDIT_FRAME::OnExit );
}
