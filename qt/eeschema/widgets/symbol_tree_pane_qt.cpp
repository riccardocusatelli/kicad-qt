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
 * @file symbol_tree_pane_qt.cpp
 * SYMBOL_TREE_PANE with Qt (see eeschema/widgets/symbol_tree_pane.cpp): the tree gives the
 * chosen items to its handler, and the menus of Qt don't need to block the previews of the tree.
 */

#include <QVBoxLayout>

#include <widgets/symbol_tree_pane.h>
#include <widgets/lib_tree.h>
#include <lib_symbol_library_manager.h>
#include <symbol_edit_frame.h>
#include <tool/tool_manager.h>
#include <tools/sch_actions.h>
#include <ui_hooks.h>


SYMBOL_TREE_PANE::SYMBOL_TREE_PANE( SYMBOL_EDIT_FRAME* aParent,
                                    LIB_SYMBOL_LIBRARY_MANAGER* aLibMgr )
        : wxPanel( aParent ),
          m_symbolEditFrame( aParent ),
          m_tree( nullptr ),
          m_libMgr( aLibMgr )
{
    // Create widgets
    QVBoxLayout* layout = new QVBoxLayout( this );
    layout->setContentsMargins( 0, 0, 0, 0 );

    m_tree = new LIB_TREE( this, wxT( "symbols" ), m_libMgr->GetAdapter(),
                           LIB_TREE::SEARCH | LIB_TREE::MULTISELECT );
    layout->addWidget( m_tree, 1 );

    m_libMgr->GetAdapter()->FinishTreeInitialization();

    // Event handlers
    m_tree->SetChosenHandler(
            [this]()
            {
                wxCommandEvent dummy;
                onSymbolSelected( dummy );
            } );

    KIQT::RegisterUpdateUI( this,
                            [this]()
                            {
                                wxUpdateUIEvent dummy;
                                onUpdateUI( dummy );
                            } );
}


SYMBOL_TREE_PANE::~SYMBOL_TREE_PANE()
{
    m_tree->SetChosenHandler( nullptr );
}


void SYMBOL_TREE_PANE::onSymbolSelected( wxCommandEvent& aEvent )
{
    m_symbolEditFrame->GetToolManager()->RunAction( SCH_ACTIONS::editSymbol );

    // Make sure current-part highlighting doesn't get lost in selection highlighting
    m_tree->Unselect();

    // Turn off any previous current-part highlighting
    m_tree->RefreshLibTree();
}


void SYMBOL_TREE_PANE::onUpdateUI( wxUpdateUIEvent& aEvent )
{
    if( m_symbolEditFrame->GetCanvas() && m_symbolEditFrame->GetCanvas()->HasFocus() )
    {
        // Don't allow a selected item in the tree when the canvas has focus: it's too easy
        // to confuse the selected-highlighting with the being-edited-on-canvas-highlighting.
        if( m_symbolEditFrame->GetCurSymbol() != nullptr )
        {
            m_tree->Unselect();
        }
    }
}
