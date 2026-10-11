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
 * @file panel_design_block_chooser.cpp
 * What PANEL_DESIGN_BLOCK_CHOOSER does without the GUI toolkit: the libraries of the tree, the
 * history, the selection and the preview of the selected design block.  The widgets are in
 * panel_design_block_chooser_wx.cpp (and in qt/common/widgets/panel_design_block_chooser_qt.cpp).
 */

#include <pgm_base.h>
#include <design_block.h>
#include <widgets/design_block_pane.h>
#include <design_block_library_adapter.h>
#include <design_block_tree_model_adapter.h>
#include <widgets/panel_design_block_chooser.h>
#include <widgets/design_block_preview_widget.h>
#include <eda_draw_frame.h>
#include <widgets/lib_tree.h>


wxString PANEL_DESIGN_BLOCK_CHOOSER::g_designBlockSearchString;


void PANEL_DESIGN_BLOCK_CHOOSER::ShowChangedLanguage()
{
    if( m_tree )
        m_tree->ShowChangedLanguage();
}


void PANEL_DESIGN_BLOCK_CHOOSER::RefreshLibs( bool aProgress )
{
    // Unselect before syncing to avoid null reference in the adapter
    // if a selected item is removed during the sync
    LIB_ID savedSelection = GetSelectedLibId();
    m_tree->Unselect();

    DESIGN_BLOCK_TREE_MODEL_ADAPTER* adapter = static_cast<DESIGN_BLOCK_TREE_MODEL_ADAPTER*>( m_adapter.get() );

    // Clear all existing libraries then re-add
    adapter->ClearLibraries();

    rebuildHistoryNode();

    if( !m_historyList.empty() )
        adapter->SetPreselectNode( m_historyList[0], 0 );

    adapter->AddLibraries( m_frame );

    m_tree->Regenerate( true );

    if( savedSelection.IsValid() )
        SelectLibId( savedSelection );

    refreshView();
}


void PANEL_DESIGN_BLOCK_CHOOSER::SetPreselect( const LIB_ID& aPreselect )
{
    m_adapter->SetPreselectNode( aPreselect, 0 );
}


LIB_ID PANEL_DESIGN_BLOCK_CHOOSER::GetSelectedLibId( int* aUnit ) const
{
    return m_tree->GetSelectedLibId( aUnit );
}


void PANEL_DESIGN_BLOCK_CHOOSER::SelectLibId( const LIB_ID& aLibId )
{
    m_tree->CenterLibId( aLibId );
    m_tree->SelectLibId( aLibId );
}


void PANEL_DESIGN_BLOCK_CHOOSER::showSelectedDesignBlock()
{
    if( !m_preview || !m_preview->IsInitialized() )
        return;

    if( GetSelectedLibId().IsValid() )
    {
        std::unique_ptr<DESIGN_BLOCK> designBlock( m_parent->GetDesignBlock( GetSelectedLibId(), true, true ) );
        m_preview->DisplayDesignBlock( designBlock.get() );
    }
}


void PANEL_DESIGN_BLOCK_CHOOSER::addDesignBlockToHistory( const LIB_ID& aLibId )
{
    LIB_ID savedId = GetSelectedLibId();

    m_tree->Unselect();

    // Remove duplicates
    for( int i = (int) m_historyList.size() - 1; i >= 0; --i )
    {
        if( m_historyList[i] == aLibId )
            m_historyList.erase( m_historyList.begin() + i );
    }

    // Add the new name at the beginning of the history list
    m_historyList.insert( m_historyList.begin(), aLibId );

    // Remove extra names
    while( m_historyList.size() >= 8 )
        m_historyList.pop_back();

    rebuildHistoryNode();
    m_tree->Regenerate( true );

    SelectLibId( savedId );
}


void PANEL_DESIGN_BLOCK_CHOOSER::rebuildHistoryNode()
{
    m_adapter->RemoveGroup( true, false );

    // Build the history list
    std::vector<LIB_TREE_ITEM*> historyInfos;

    DESIGN_BLOCK_LIBRARY_ADAPTER* adapter = m_frame->Prj().DesignBlockLibs();

    for( const LIB_ID& lib : m_historyList )
    {
        LIB_TREE_ITEM* info = adapter->LoadDesignBlock( lib.GetLibNickname(), lib.GetLibItemName() );

        // this can be null, for example, if the design block has been deleted from a library.
        if( info != nullptr )
            historyInfos.push_back( info );
    }

    m_adapter->DoAddLibrary( wxT( "-- " ) + _( "Recently Used" ) + wxT( " --" ), wxEmptyString,
                             historyInfos, false, true )
            .m_IsRecentlyUsedGroup = true;
}
