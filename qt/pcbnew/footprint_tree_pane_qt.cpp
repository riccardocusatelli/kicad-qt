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
 * @file footprint_tree_pane_qt.cpp
 * FOOTPRINT_TREE_PANE with Qt (see pcbnew/footprint_tree_pane.cpp): the tree gives the chosen
 * items to its handler, and the menus of Qt don't need to block the previews of the tree.
 */

#include <QTimer>
#include <QVBoxLayout>

#include "footprint_tree_pane.h"
#include "fp_tree_synchronizing_adapter.h"
#include <widgets/lib_tree.h>
#include <footprint_edit_frame.h>
#include <ui_hooks.h>


FOOTPRINT_TREE_PANE::FOOTPRINT_TREE_PANE( FOOTPRINT_EDIT_FRAME* aParent )
        : wxPanel( aParent ),
          m_frame( aParent ),
          m_tree( nullptr )
{
    // Create widgets
    QVBoxLayout* layout = new QVBoxLayout( this );
    layout->setContentsMargins( 0, 0, 0, 0 );

    m_tree = new LIB_TREE( this, wxT( "footprints" ), m_frame->GetLibTreeAdapter(),
                           LIB_TREE::SEARCH );
    layout->addWidget( m_tree, 1 );

    m_frame->GetLibTreeAdapter()->FinishTreeInitialization();

    // Event handlers
    m_tree->SetChosenHandler( [this]() { onFootprintChosen(); } );

    KIQT::RegisterUpdateUI( this,
                            [this]()
                            {
                                // Don't allow a selected item in the tree when the canvas has
                                // focus: it's too easy to confuse the selected-highlighting with
                                // the being-edited-on-canvas-highlighting.
                                if( m_frame->GetCanvas() && m_frame->GetCanvas()->HasFocus() )
                                    m_tree->Unselect();
                            } );
}


void FOOTPRINT_TREE_PANE::FocusSearchFieldIfExists()
{
    if( m_tree )
        m_tree->FocusSearchFieldIfExists();
}


void FOOTPRINT_TREE_PANE::PostFootprintChosen()
{
    QTimer::singleShot( 0, this, [this]() { onFootprintChosen(); } );
}


FOOTPRINT_TREE_PANE::~FOOTPRINT_TREE_PANE()
{
    m_tree->SetChosenHandler( nullptr );
}


void FOOTPRINT_TREE_PANE::onFootprintChosen()
{
    m_frame->LoadFootprintFromLibrary( GetLibTree()->GetSelectedLibId() );
    // Make sure current-part highlighting doesn't get lost in seleciton highlighting
    m_tree->Unselect();
}
