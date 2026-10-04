/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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
 * @file board_commit_gui.cpp
 * @brief BOARD_COMMIT in an editor: the BOARD_COMMIT_LISTENER that keeps the view, the
 *        selection, the undo list and the frame in step with the board.
 */

#include <board_commit.h>

#include <board.h>
#include <footprint.h>
#include <pcb_base_frame.h>
#include <pcb_board_outline.h>
#include <pcb_group.h>
#include <pcbnew_settings.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <tools/pcb_tool_base.h>
#include <tools/zone_filler_tool.h>
#include <tool/tool_manager.h>
#include <undo_redo_container.h>
#include <pcb_view.h>
#include <widgets/msgpanel_item.h>
#include <settings/settings_manager.h>
#include <pgm_base.h>


namespace
{

/// The listener of the commits made in an editor, through its TOOL_MANAGER.
class TOOL_COMMIT_LISTENER : public BOARD_COMMIT_LISTENER
{
public:
    explicit TOOL_COMMIT_LISTENER( TOOL_MANAGER* aMgr ) :
            m_toolMgr( aMgr )
    {
    }

    BOARD* Board() const override { return static_cast<BOARD*>( m_toolMgr->GetModel() ); }

    PCB_GROUP* EnteredGroup() const override
    {
        PCB_SELECTION_TOOL* selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
        return selTool ? selTool->GetEnteredGroup() : nullptr;
    }

    bool AutoRefillZones() const override
    {
        PCB_BASE_FRAME* frame = this->frame();
        return frame && frame->GetPcbNewSettings()->m_AutoRefillZones;
    }

    void ItemAdded( BOARD_ITEM* aItem ) override
    {
        if( KIGFX::VIEW* view = m_toolMgr->GetView() )
            view->Add( aItem );
    }

    void ItemRemoved( BOARD_ITEM* aItem ) override
    {
        if( KIGFX::VIEW* view = m_toolMgr->GetView() )
            view->Remove( aItem );
    }

    void ItemChanged( BOARD_ITEM* aItem ) override
    {
        if( KIGFX::VIEW* view = m_toolMgr->GetView() )
            view->Update( aItem );
    }

    void Deselect( BOARD_ITEM* aItem ) override
    {
        if( PCB_SELECTION_TOOL* selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>() )
            selTool->RemoveItemFromSel( aItem, true /* quiet mode */ );
    }

    bool PruneSelection( const std::unordered_set<EDA_ITEM*>& aRemoved ) override
    {
        PCB_SELECTION_TOOL* selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
        bool                pruned = false;

        if( !selTool )
            return false;

        for( EDA_ITEM* selectedItem : selTool->GetSelection().GetItems() )
        {
            for( EDA_ITEM* ancestor = selectedItem; ancestor; ancestor = ancestor->GetParent() )
            {
                if( aRemoved.count( ancestor ) )
                {
                    selTool->RemoveItemFromSel( selectedItem, true /* quiet mode */ );
                    pruned = true;
                    break;
                }
            }
        }

        return pruned;
    }

    void RatsnestChanged() override
    {
        if( PCB_BASE_FRAME* frame = this->frame() )
            frame->GetCanvas()->RedrawRatsnest();
    }

    void SolderMaskChanged() override
    {
        if( PCB_BASE_FRAME* frame = this->frame() )
            frame->HideSolderMask();
    }

    void BoardOutlineChanged() override
    {
        KIGFX::VIEW*       view = m_toolMgr->GetView();
        PCB_BOARD_OUTLINE* outline = Board()->BoardOutline();

        if( !view || !outline )
            return;

        if( view->HasItem( outline ) )
            view->Update( outline );
        else
            view->Add( outline );
    }

    void RuleAreasChanged( const std::vector<BOX2I>& aAreas ) override
    {
        KIGFX::PCB_VIEW* view = static_cast<KIGFX::PCB_VIEW*>( m_toolMgr->GetView() );

        if( !view )
            return;

        if( PCBNEW_SETTINGS* cfg = GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" ) )
        {
            if( cfg->m_Display.m_TrackClearance == SHOW_WITH_VIA_ALWAYS
                    || cfg->m_Display.m_PadClearance )
            {
                view->UpdateCollidingItems( aAreas, { PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T, PCB_PAD_T } );
            }
        }
    }

    void Pushed( PICKED_ITEMS_LIST& aUndoList, int aCommitFlags, const PUSH_RESULT& aResult ) override
    {
        PCB_BASE_FRAME* frame = this->frame();
        BOARD*          board = Board();

        if( frame )
        {
            if( !( aCommitFlags & SKIP_UNDO ) )
            {
                if( aCommitFlags & APPEND_UNDO )
                    frame->AppendCopyToUndoList( aUndoList, UNDO_REDO::UNSPECIFIED );
                else
                    frame->SaveCopyInUndoList( aUndoList, UNDO_REDO::UNSPECIFIED );
            }
        }

        m_toolMgr->PostEvent( { TC_MESSAGE, TA_MODEL_CHANGE, AS_GLOBAL } );

        if( aResult.itemsDeselected )
            m_toolMgr->PostEvent( EVENTS::UnselectedEvent );

        if( aResult.autofillZones )
        {
            ZONE_FILLER_TOOL* zoneFillerTool = m_toolMgr->GetTool<ZONE_FILLER_TOOL>();

            for( ZONE* zone : aResult.staleZones )
                zoneFillerTool->DirtyZone( zone );

            m_toolMgr->PostAction( PCB_ACTIONS::zoneFillDirty );
        }

        m_toolMgr->PostAction( PCB_ACTIONS::rehatchShapes );

        if( aResult.selectedModified )
            m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );

        if( frame )
        {
            if( !( aCommitFlags & SKIP_SET_DIRTY ) )
                frame->OnModify();
            else
                frame->Update3DView( true, frame->GetPcbNewSettings()->m_Display.m_Live3DRefresh );

            // Ensure the message panel is updated after committing changes.
            // By default (i.e. if no event posted), display the updated board info
            if( !aResult.itemsDeselected && !aResult.autofillZones && !aResult.selectedModified )
            {
                std::vector<MSG_PANEL_ITEM> msg_list;
                board->GetMsgPanelInfo( frame->GetMsgPanelContext(), msg_list );
                frame->SetMsgPanel( msg_list );
            }
        }
    }

    void Reverted() override
    {
        if( PCB_SELECTION_TOOL* selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>() )
            selTool->RebuildSelection();

        // Property panel needs to know about the reselect
        m_toolMgr->PostEvent( EVENTS::SelectedItemsModified );
    }

private:
    PCB_BASE_FRAME* frame() const
    {
        return dynamic_cast<PCB_BASE_FRAME*>( m_toolMgr->GetToolHolder() );
    }

    TOOL_MANAGER* m_toolMgr;
};

} // namespace


BOARD_COMMIT::BOARD_COMMIT( TOOL_BASE* aTool ) :
        BOARD_COMMIT( aTool->GetManager(), false, false )
{
    if( PCB_TOOL_BASE* pcb_tool = dynamic_cast<PCB_TOOL_BASE*>( aTool ) )
    {
        m_isBoardEditor = pcb_tool->IsBoardEditor();
        m_isFootprintEditor = pcb_tool->IsFootprintEditor();
    }
}


BOARD_COMMIT::BOARD_COMMIT( EDA_DRAW_FRAME* aFrame ) :
        BOARD_COMMIT( aFrame->GetToolManager(), aFrame->IsType( FRAME_PCB_EDITOR ),
                      aFrame->IsType( FRAME_FOOTPRINT_EDITOR ) )
{
}


BOARD_COMMIT::BOARD_COMMIT( TOOL_MANAGER* aMgr ) :
        BOARD_COMMIT( aMgr, false, false )
{
    EDA_DRAW_FRAME* frame = dynamic_cast<EDA_DRAW_FRAME*>( aMgr->GetToolHolder() );

    if( frame && frame->IsType( FRAME_PCB_EDITOR ) )
        m_isBoardEditor = true;
    else if( frame && frame->IsType( FRAME_FOOTPRINT_EDITOR ) )
        m_isFootprintEditor = true;
}


BOARD_COMMIT::BOARD_COMMIT( TOOL_MANAGER* aMgr, bool aIsBoardEditor, bool aIsFootprintEditor ) :
        COMMIT(),
        m_board( nullptr ),
        m_listener( std::make_unique<TOOL_COMMIT_LISTENER>( aMgr ) ),
        m_isBoardEditor( aIsBoardEditor ),
        m_isFootprintEditor( aIsFootprintEditor )
{
}
