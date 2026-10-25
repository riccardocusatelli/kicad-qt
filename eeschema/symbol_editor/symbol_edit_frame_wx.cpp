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
 * @file symbol_edit_frame_wx.cpp
 * The parts of SYMBOL_EDIT_FRAME with wxWidgets: the layout of its panes.
 */

#include <sch_pane_info.h>
#include <symbol_edit_frame.h>
#include <symbol_editor/symbol_editor_settings.h>
#include <tool/action_toolbar.h>
#include <tools/sch_selection_tool.h>
#include <widgets/panel_sch_selection_filter.h>
#include <widgets/sch_properties_panel.h>
#include <widgets/symbol_tree_pane.h>
#include <widgets/wx_infobar.h>


void SYMBOL_EDIT_FRAME::setupLayout()
{
    m_auimgr.SetManagedWindow( this );

    CreateInfoBar();

    // Rows; layers 4 - 6
    m_auimgr.AddPane( m_tbTopMain, EDA_PANE().HToolbar().Name( "TopMainToolbar" )
                      .Top().Layer( 6 ) );

    m_auimgr.AddPane( m_messagePanel, EDA_PANE().Messages().Name( "MsgPanel" )
                      .Bottom().Layer( 6 ) );

    m_auimgr.AddPane( m_tbLeft, EDA_PANE().VToolbar().Name( "LeftToolbar" )
                      .Left().Layer( 2 ) );

    m_auimgr.AddPane( m_tbRight, EDA_PANE().VToolbar().Name( "RightToolbar" )
                      .Right().Layer( 2 ) );

    // Center
    m_auimgr.AddPane( GetCanvas(), wxAuiPaneInfo().Name( "DrawFrame" )
                      .CentrePane() );

    // Columns; layers 1 - 3
    m_auimgr.AddPane( m_treePane, EDA_PANE().Palette().Name( "LibraryTree" )
                      .Left().Layer( 3 )
                      .TopDockable( false ).BottomDockable( false )
                      .Caption( _( "Libraries" ) )
                      // Don't use -1 for don't-change-height on a growable panel; it has side-effects.
                      .MinSize( FromDIP( 250 ), FromDIP( 80 ) )
                      .BestSize( FromDIP( 250 ), -1 ) );

    m_auimgr.AddPane( m_propertiesPanel, defaultPropertiesPaneInfo( this ) );
    m_auimgr.AddPane( m_selectionFilterPanel, defaultSchSelectionFilterPaneInfo( this ) );

    // Can be called only when all panes are created, because (at least on Windows) when items
    // managed by m_auimgr are not the same as those existing when saved by Perspective()
    // in config, broken settings can happen.
    RestoreAuiLayout();

    // Protect against broken saved Perspective() due to bugs in previous version
    // This is currently a workaround.
    m_auimgr.GetPane( "TopMainToolbar" ).Top().Layer( 6 ).Position(0).Show( true );
    m_auimgr.GetPane( "LeftToolbar" ).Position(0).Show( true );
    m_auimgr.GetPane( "RightToolbar" ).Show( true );

    // Show or hide m_propertiesPanel depending on current settings:
    wxAuiPaneInfo& propertiesPaneInfo = m_auimgr.GetPane( PropertiesPaneName() );
    wxAuiPaneInfo& selectionFilterPane = m_auimgr.GetPane( wxS( "SelectionFilter" ) );
    // The selection filter doesn't need to grow in the vertical direction when docked
    selectionFilterPane.dock_proportion = 0;

    propertiesPaneInfo.Show( m_settings->m_AuiPanels.show_properties );
    updateSelectionFilterVisbility();

    FinishAUIInitialization();
}
