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
 * @file symbol_edit_frame_qt.cpp
 * The parts of SYMBOL_EDIT_FRAME with Qt: the layout of its panes and its events (see
 * eeschema/symbol_editor/symbol_edit_frame_wx.cpp).
 *
 * The dropped files are received by EDA_BASE_FRAME::dropEvent() and the choices of the zoom and
 * of the grid by EDA_DRAW_FRAME: the frame has no events of its own for them.
 */

#include <QBoxLayout>
#include <QDockWidget>

#include <kiway.h>
#include <symbol_edit_frame.h>
#include <symbol_editor/symbol_editor_settings.h>
#include <symbol_editor/symbol_editor_tab_context.h>
#include <tools/sch_selection_tool.h>
#include <widgets/editor_tabs_panel.h>
#include <widgets/panel_sch_selection_filter.h>
#include <widgets/sch_properties_panel.h>
#include <widgets/symbol_tree_pane.h>


void SYMBOL_EDIT_FRAME::setupLayout()
{
    CreateInfoBar();

    // The canvas, the info bar and the message panel
    setupCentralArea();

    // The tabs of the open symbols, over the canvas
    m_tabsPanel = new EDITOR_TABS_PANEL( centralWidget(), GetCanvas() );

    if( QBoxLayout* central = qobject_cast<QBoxLayout*>( centralWidget()->layout() ) )
        central->insertWidget( central->indexOf( GetCanvas() ), m_tabsPanel );

    m_tabsPanel->onActivateTab =
            [this]( int aIdx )
            {
                if( SYMBOL_EDITOR_TAB_CONTEXT* ctx = symbolTabContextForIndex( aIdx ) )
                    activateSymbolTab( ctx );
            };

    m_tabsPanel->onCloseTabRequested =
            [this]( int aIdx ) -> bool
            {
                return promptAndCloseSymbolTab( aIdx );
            };

    m_tabsPanel->onQueryVisualState =
            [this]( int aIdx ) -> TAB_VISUAL_STATE
            {
                // The panel model owns preview. Read modified from the live context.
                const std::vector<EDITOR_TABS_MODEL::ENTRY>& entries = m_tabsPanel->Model().Entries();

                if( aIdx < 0 || aIdx >= static_cast<int>( entries.size() ) )
                    return TAB_VISUAL_STATE{};

                bool modified = entries[aIdx].modified;

                if( SYMBOL_EDITOR_TAB_CONTEXT* ctx = symbolTabContextForIndex( aIdx ) )
                    modified = ctx->IsModified();

                return ResolveTabVisualState( entries[aIdx].preview, modified );
            };

    // The libraries, the properties and the selection filter on the left side, one above the
    // other
    QDockWidget* tree = addPane( m_treePane, wxS( "LibraryTree" ), _( "Libraries" ),
                                 PANE_DOCK::LEFT );
    QDockWidget* properties = addPane( m_propertiesPanel, PropertiesPaneName(), _( "Properties" ),
                                       PANE_DOCK::LEFT, m_settings->m_AuiPanels.show_properties );
    QDockWidget* selectionFilter = addPane( m_selectionFilterPanel, wxS( "SelectionFilter" ),
                                            _( "Selection Filter" ), PANE_DOCK::LEFT );

    splitDockWidget( tree, properties, Qt::Vertical );
    splitDockWidget( properties, selectionFilter, Qt::Vertical );

    // The libraries can't be docked on the top and on the bottom sides
    for( QDockWidget* dock : { tree, properties, selectionFilter } )
        dock->setAllowedAreas( Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea );

    m_treePane->setMinimumSize( 250, 80 );
    m_propertiesPanel->setMinimumSize( 240, 60 );

    updateSelectionFilterVisbility();

    // The best size of the libraries (as the wxWidgets build; the sizes of Qt are independent of
    // the DPI)
    resizeDocks( { tree }, { 250 }, Qt::Horizontal );

    // The selection filter doesn't need to grow in the vertical direction when docked
    resizeDocks( { selectionFilter }, { m_selectionFilterPanel->sizeHint().height() },
                 Qt::Vertical );
}


void SYMBOL_EDIT_FRAME::setupToolkitEvents()
{
    BindMenuCommand( wxID_EXIT, [this]() { Kiway().OnKiCadExit(); } );
    BindMenuCommand( wxID_CLOSE, [this]() { Close( false ); } );
}
