/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2016 CERN
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <functional>
#include <core/ki_assert.h>
#include <pcb_edit_frame.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <tools/pcb_control.h>
#include <tools/board_editor_control.h>
#include <board_commit.h>
#include <board_undo_redo.h>
#include <drawing_sheet/ds_proxy_undo_item.h>
#include <wx/msgdlg.h>
#include <pcb_board_outline.h>
#include <pcb_drill_map.h>

/* Undo and redo of the board editors: the undo and redo lists of the frame hold the entries
 * made by BOARD_COMMIT::Push(), which BOARD_UNDO_REDO (board_undo_redo.cpp) applies.
 */


namespace
{

/// The board editor, for BOARD_UNDO_REDO::Apply().
class FRAME_UNDO_REDO_LISTENER : public BOARD_UNDO_REDO::LISTENER
{
public:
    explicit FRAME_UNDO_REDO_LISTENER( PCB_BASE_EDIT_FRAME* aFrame ) :
            m_frame( aFrame )
    {
    }

    void ItemRemoved( EDA_ITEM* aItem ) override
    {
        m_frame->GetCanvas()->GetView()->Remove( aItem );
    }

    void ItemAdded( EDA_ITEM* aItem ) override
    {
        KIGFX::PCB_VIEW* view = m_frame->GetCanvas()->GetView();

        view->Add( aItem );
        view->Hide( aItem, false );
    }

    bool ShowRatsnest() const override
    {
        return m_frame->GetPcbNewSettings()->m_Display.m_ShowGlobalRatsnest;
    }

    void SetOrigin( UNDO_REDO aOrigin, EDA_ITEM* aOriginItem, const VECTOR2I& aPosition ) override
    {
        KIGFX::PCB_VIEW* view = m_frame->GetCanvas()->GetView();

        if( aOrigin == UNDO_REDO::DRILLORIGIN )
            BOARD_EDITOR_CONTROL::DoSetDrillOrigin( view, m_frame, aOriginItem, aPosition );
        else
            PCB_CONTROL::DoSetGridOrigin( view, m_frame, aOriginItem, aPosition );
    }

    void SwapPageSettings( DS_PROXY_UNDO_ITEM* aItem ) override
    {
        // swap current settings with stored settings
        DS_PROXY_UNDO_ITEM alt_item( m_frame );
        aItem->Restore( m_frame );
        *aItem = std::move( alt_item );
    }

private:
    PCB_BASE_EDIT_FRAME* m_frame;
};

} // namespace


void PCB_BASE_EDIT_FRAME::saveCopyInUndoList( PICKED_ITEMS_LIST* commandToUndo,
                                              const PICKED_ITEMS_LIST& aItemsList,
                                              UNDO_REDO aCommandType )
{
    int preExisting = (int) commandToUndo->GetCount();

    for( unsigned ii = 0; ii < aItemsList.GetCount(); ii++ )
        commandToUndo->PushItem( aItemsList.GetItemWrapper(ii) );

    BOARD_UNDO_REDO::CompleteEntry( *commandToUndo, preExisting, aCommandType );

    if( commandToUndo->GetCount() )
    {
        /* Save the copy in undo list */
        PushCommandToUndoList( commandToUndo );

        /* Clear redo list, because after a new command one cannot redo a command */
        ClearUndoORRedoList( REDO_LIST );
    }
    else
    {
        // Should not occur
        KI_ASSERT( false );
        delete commandToUndo;
    }
}


void PCB_BASE_EDIT_FRAME::SaveCopyInUndoList( EDA_ITEM* aItem, UNDO_REDO aCommandType )
{
    PICKED_ITEMS_LIST* commandToUndo = new PICKED_ITEMS_LIST();
    PICKED_ITEMS_LIST  itemsList;

    itemsList.PushItem( ITEM_PICKER( nullptr, aItem, aCommandType ) );
    saveCopyInUndoList( commandToUndo, itemsList, aCommandType );
}


void PCB_BASE_EDIT_FRAME::SaveCopyInUndoList( const PICKED_ITEMS_LIST& aItemsList,
                                              UNDO_REDO aCommandType )
{
    PICKED_ITEMS_LIST* commandToUndo = new PICKED_ITEMS_LIST();
    commandToUndo->SetDescription( aItemsList.GetDescription() );

    saveCopyInUndoList( commandToUndo, aItemsList, aCommandType );
}


void PCB_BASE_EDIT_FRAME::AppendCopyToUndoList( const PICKED_ITEMS_LIST& aItemsList,
                                                UNDO_REDO aCommandType )
{
    PICKED_ITEMS_LIST* commandToUndo = PopCommandFromUndoList();

    if( !commandToUndo )
    {
        commandToUndo = new PICKED_ITEMS_LIST();
        commandToUndo->SetDescription( aItemsList.GetDescription() );
    }

    saveCopyInUndoList( commandToUndo, aItemsList, aCommandType );
}


<<<<<<< b2e17e0a
void PCB_BASE_EDIT_FRAME::RestoreCopyFromUndoList()
||||||| 1304b76aad
/**
 * Check whether the undo/redo list contains any items that could affect the board outline
 * or shape hatching.  Used to skip expensive post-processing when only tracks changed.
 */
static bool undoListContainsShapesOrFootprints( const PICKED_ITEMS_LIST* aList )
{
    for( unsigned ii = 0; ii < aList->GetCount(); ++ii )
    {
        switch( aList->GetPickedItem( ii )->Type() )
        {
        case PCB_SHAPE_T:
        case PCB_FOOTPRINT_T:
        case PCB_TEXT_T:
        case PCB_TEXTBOX_T:
        case PCB_FIELD_T:
            return true;

        default:
            break;
        }
    }

    return false;
}


void PCB_BASE_EDIT_FRAME::RestoreCopyFromUndoList( wxCommandEvent& aEvent )
=======
/**
 * Check whether the undo/redo list contains any items that could affect the board outline
 * or shape hatching.  Used to skip expensive post-processing when only tracks changed.
 */
static bool undoListContainsShapesOrFootprints( const PICKED_ITEMS_LIST* aList )
{
    for( unsigned ii = 0; ii < aList->GetCount(); ++ii )
    {
        switch( aList->GetPickedItem( ii )->Type() )
        {
        case PCB_SHAPE_T:
        case PCB_FOOTPRINT_T:
        case PCB_TEXT_T:
        case PCB_TEXTBOX_T:
        case PCB_FIELD_T:
            return true;

        default:
            break;
        }
    }

    return false;
}


static bool undoListContainsDrillMap( const PICKED_ITEMS_LIST* aList )
{
    for( unsigned ii = 0; ii < aList->GetCount(); ++ii )
    {
        if( EDA_ITEM* item = aList->GetPickedItem( ii ) )
        {
            if( item->Type() == PCB_DRILL_MAP_T )
                return true;
        }
    }

    return false;
}


void PCB_BASE_EDIT_FRAME::RestoreCopyFromUndoList( wxCommandEvent& aEvent )
>>>>>>> ed8c16703e
{
    if( UndoRedoBlocked() )
        return;

    if( GetUndoCommandCount() <= 0 )
        return;

    // Inform tools that undo command was issued
    m_toolManager->ProcessEvent( { TC_MESSAGE, TA_UNDO_REDO_PRE, AS_GLOBAL } );

    // Get the old list
    PICKED_ITEMS_LIST* list = PopCommandFromUndoList();

<<<<<<< b2e17e0a
||||||| 1304b76aad
    bool shapesChanged = undoListContainsShapesOrFootprints( list );

=======
    bool shapesChanged = undoListContainsShapesOrFootprints( list );
    bool drillMapChanged = undoListContainsDrillMap( list );

>>>>>>> ed8c16703e
    // Undo the command
    bool shapesChanged = PutDataInPreviousState( list );

    if( drillMapChanged )
    {
        if( PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>( this ) )
            editFrame->RefreshDrillSymbols( KIGFX::LAYERS | KIGFX::GEOMETRY | KIGFX::REPAINT );
    }

    // Put the old list in RedoList
    list->ReversePickersListOrder();
    PushCommandToRedoList( list );

    OnModify();

    m_toolManager->ProcessEvent( { TC_MESSAGE, TA_UNDO_REDO_POST, AS_GLOBAL } );
    m_toolManager->PostEvent( EVENTS::SelectedItemsModified );

    if( shapesChanged )
    {
        m_pcb->UpdateBoardOutline();
        GetCanvas()->GetView()->Update( m_pcb->BoardOutline() );
        RefreshDrillMapOutlines( *m_pcb, GetCanvas()->GetView() );
    }

    GetCanvas()->Refresh();
}


void PCB_BASE_EDIT_FRAME::RestoreCopyFromRedoList()
{
    if( UndoRedoBlocked() )
        return;

    if( GetRedoCommandCount() == 0 )
        return;

    // Inform tools that redo command was issued
    m_toolManager->ProcessEvent( EVENTS::UndoRedoPreEvent );

    // Get the old list
    PICKED_ITEMS_LIST* list = PopCommandFromRedoList();

<<<<<<< b2e17e0a
||||||| 1304b76aad
    bool shapesChanged = undoListContainsShapesOrFootprints( list );

=======
    bool shapesChanged = undoListContainsShapesOrFootprints( list );
    bool drillMapChanged = undoListContainsDrillMap( list );

>>>>>>> ed8c16703e
    // Redo the command
    bool shapesChanged = PutDataInPreviousState( list );

    if( drillMapChanged )
    {
        if( PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>( this ) )
            editFrame->RefreshDrillSymbols( KIGFX::LAYERS | KIGFX::GEOMETRY | KIGFX::REPAINT );
    }

    // Put the old list in UndoList
    list->ReversePickersListOrder();
    PushCommandToUndoList( list );

    OnModify();

    m_toolManager->ProcessEvent( EVENTS::UndoRedoPostEvent );
    m_toolManager->PostEvent( EVENTS::SelectedItemsModified );

    if( shapesChanged )
    {
        m_pcb->UpdateBoardOutline();
        GetCanvas()->GetView()->Update( m_pcb->BoardOutline() );
        RefreshDrillMapOutlines( *m_pcb, GetCanvas()->GetView() );
    }

    GetCanvas()->Refresh();
}


bool PCB_BASE_EDIT_FRAME::PutDataInPreviousState( PICKED_ITEMS_LIST* aList, bool aRehatchShapes )
{
    FRAME_UNDO_REDO_LISTENER listener( this );
    const bool               boardEditor = IsType( FRAME_PCB_EDITOR );

    BOARD_UNDO_REDO::RESULT result = BOARD_UNDO_REDO::Apply( *GetBoard(), *aList, listener,
                                                             boardEditor );

<<<<<<< b2e17e0a
    if( result.itemsNotFound )
||||||| 1304b76aad
    GetBoard()->IncrementTimeStamp();   // clear caches

    // Enum to track the modification type of items. Used to enable bulk BOARD_LISTENER
    // callbacks at the end of the undo / redo operation
    enum ITEM_CHANGE_TYPE
    {
        ADDED,
        DELETED,
        CHANGED
    };

    std::unordered_map<EDA_ITEM*, ITEM_CHANGE_TYPE> item_changes;

    auto clear_local_ratsnest_flags =
            [&]( EDA_ITEM* item )
            {
                switch( item->Type() )
                {
                case PCB_TRACE_T:
                case PCB_ARC_T:
                case PCB_VIA_T:
                    static_cast<PCB_TRACK*>( item )->SetLocalRatsnestVisible( current_show_ratsnest );
                    break;

                case PCB_ZONE_T:
                    static_cast<ZONE*>( item )->SetLocalRatsnestVisible( current_show_ratsnest );
                    break;

                case PCB_FOOTPRINT_T:
                    for( PAD* pad : static_cast<FOOTPRINT*>( item )->Pads() )
                        pad->SetLocalRatsnestVisible( current_show_ratsnest );

                    break;

                default:
                    break;
                }
            };

    auto update_item_change_state =
            [&]( EDA_ITEM* item, ITEM_CHANGE_TYPE change_type )
            {
                auto item_itr = item_changes.find( item );

                if( item_itr == item_changes.end() )
                {
                    // First time we've seen this item - tag the current change type
                    item_changes.insert( { item, change_type } );
                    return;
                }

                // Update the item state based on the current and next change type
                switch( item_itr->second )
                {
                case ITEM_CHANGE_TYPE::ADDED:
                    if( change_type == ITEM_CHANGE_TYPE::DELETED )
                    {
                        // The item was previously added, now deleted - as far as bulk callbacks
                        // are concerned, the item has never existed
                        item_changes.erase( item_itr );
                    }
                    else if( change_type == ITEM_CHANGE_TYPE::ADDED )
                    {
                        // Error condition - added an already added item
                        KI_ASSERT_MSG( false, wxT( "UndoRedo: should not add already added item" ) );
                    }

                    // For all other cases, the item remains as ADDED as seen by the bulk callbacks
                    break;

                case ITEM_CHANGE_TYPE::DELETED:
                    // This is an error condition - item has already been deleted so should not
                    // be operated on further
                    KI_ASSERT_MSG( false, wxT( "UndoRedo: should not alter already deleted item" ) );
                    break;

                case ITEM_CHANGE_TYPE::CHANGED:
                    if( change_type == ITEM_CHANGE_TYPE::DELETED )
                    {
                        item_itr->second = ITEM_CHANGE_TYPE::DELETED;
                    }
                    else if( change_type == ITEM_CHANGE_TYPE::ADDED )
                    {
                        // This is an error condition - item has already been changed so should not
                        // be added
                        KI_ASSERT_MSG( false, wxT( "UndoRedo: should not add already changed item" ) );
                    }

                    // Otherwise, item remains CHANGED
                    break;
                }
            };

    // Undo in the reverse order of list creation: (this can allow stacked changes
    // like the same item can be changes and deleted in the same complex command

    // Restore changes in reverse order
    for( int ii = (int) aList->GetCount() - 1; ii >= 0 ; ii-- )
    {
        EDA_ITEM* eda_item = aList->GetPickedItem( (unsigned) ii );

        /* Test for existence of item on board.
         * It could be deleted, and no more on board:
         *   - if a call to SaveCopyInUndoList was forgotten in Pcbnew
         *   - in zones outlines, when a change in one zone merges this zone with an other
         * This test avoids a Pcbnew crash
         * Obviously, this test is not made for deleted items
         */
        UNDO_REDO status = aList->GetPickedItemStatus( ii );

        if( status != UNDO_REDO::DELETED
                && status != UNDO_REDO::DRILLORIGIN     // origin markers never on board
                && status != UNDO_REDO::GRIDORIGIN      // origin markers never on board
                && status != UNDO_REDO::PAGESETTINGS )  // nor are page settings proxy items
        {
            if( !GetBoard()->ResolveItem( eda_item->m_Uuid, true ) )
            {
                // Remove this non existent item
                aList->RemovePicker( ii );
                not_found = true;

                if( aList->GetCount() == 0 )
                    break;

                continue;
            }
        }

        // see if we must rebuild ratsnets and pointers lists
        switch( eda_item->Type() )
        {
        case PCB_FOOTPRINT_T:
            deep_reBuild_ratsnest = true;   // Pointers on pads can be invalid
            KI_FALLTHROUGH;

        case PCB_ZONE_T:
        case PCB_TRACE_T:
        case PCB_ARC_T:
        case PCB_VIA_T:
        case PCB_PAD_T:
            reBuild_ratsnest = true;
            break;

        case PCB_NETINFO_T:
            reBuild_ratsnest = true;
            deep_reBuild_ratsnest = true;
            break;

        default:
            break;
        }

        switch( eda_item->Type() )
        {
        case PCB_FOOTPRINT_T:
            solder_mask_dirty = true;
            break;

        case PCB_VIA_T:
            solder_mask_dirty = true;
            break;

        case PCB_ZONE_T:
        case PCB_TRACE_T:
        case PCB_ARC_T:
        case PCB_PAD_T:
        case PCB_SHAPE_T:
        {
            LSET layers = static_cast<BOARD_ITEM*>( eda_item )->GetLayerSet();

            if( layers.test( F_Mask ) || layers.test( B_Mask ) )
                solder_mask_dirty = true;

            break;
        }

        default:
            break;
        }

        switch( aList->GetPickedItemStatus( ii ) )
        {
        case UNDO_REDO::CHANGED:    /* Exchange old and new data for each item */
            if( eda_item->IsBOARD_ITEM() )
            {
                BOARD_ITEM*           item = static_cast<BOARD_ITEM*>( eda_item );
                BOARD_ITEM*           image = static_cast<BOARD_ITEM*>( aList->GetPickedItemLink( ii ) );
                BOARD_ITEM_CONTAINER* parent = GetBoard();

                // The stored pointer can be stale if a swap (e.g. ExchangeFootprint)
                // replaced the live item earlier. Resolve by UUID to find the current one.
                if( BOARD_ITEM* resolved = GetBoard()->ResolveItem( item->m_Uuid, true ) )
                    item = resolved;

                if( item->GetParentFootprint() )
                    parent = item->GetParentFootprint();

                view->Remove( item );
                parent->Remove( item, REMOVE_MODE::BULK );

                item->SwapItemData( image );

                clear_local_ratsnest_flags( item );
                item->ClearFlags( UR_TRANSIENT );
                image->SetFlags( UR_TRANSIENT );

                view->Add( item );
                view->Hide( item, false );
                parent->Add( item, ADD_MODE::BULK_INSERT );

                if( item->Type() == PCB_ZONE_T && static_cast<ZONE*>( item )->GetIsRuleArea() )
                {
                    dirty_rule_areas.push_back( item->GetBoundingBox() );
                    dirty_rule_areas.push_back( image->GetBoundingBox() );
                }

                update_item_change_state( item, ITEM_CHANGE_TYPE::CHANGED );
            }

            break;

        case UNDO_REDO::NEWITEM:        /* new items are deleted */
            if( eda_item->IsBOARD_ITEM() )
            {
                BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( eda_item );

                aList->SetPickedItemStatus( UNDO_REDO::DELETED, ii );

                if( FOOTPRINT* parentFP = boardItem->GetParentFootprint() )
                    parentFP->Remove( boardItem );
                else
                    GetModel()->Remove( boardItem, REMOVE_MODE::BULK );

                update_item_change_state( eda_item, ITEM_CHANGE_TYPE::DELETED );

                if( eda_item->Type() != PCB_NETINFO_T )
                    view->Remove( eda_item );

                eda_item->SetFlags( UR_TRANSIENT );

                if( eda_item->Type() == PCB_ZONE_T && static_cast<ZONE*>( eda_item )->GetIsRuleArea() )
                    dirty_rule_areas.push_back( eda_item->GetBoundingBox() );
            }

            break;

        case UNDO_REDO::DELETED:    /* deleted items are put in List, as new items */
            if( eda_item->IsBOARD_ITEM() )
            {
                BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( eda_item );

                aList->SetPickedItemStatus( UNDO_REDO::NEWITEM, ii );

                clear_local_ratsnest_flags( eda_item );
                eda_item->ClearFlags( UR_TRANSIENT );

                if( FOOTPRINT* parentFP = boardItem->GetParentFootprint() )
                    parentFP->Add( boardItem );
                else
                    GetModel()->Add( boardItem, ADD_MODE::BULK_APPEND );

                update_item_change_state( eda_item, ITEM_CHANGE_TYPE::ADDED );

                if( eda_item->Type() != PCB_NETINFO_T )
                    view->Add( eda_item );

                if( eda_item->Type() == PCB_ZONE_T && static_cast<ZONE*>( eda_item )->GetIsRuleArea() )
                    dirty_rule_areas.push_back( eda_item->GetBoundingBox() );
            }

            break;

        case UNDO_REDO::DRILLORIGIN:
        case UNDO_REDO::GRIDORIGIN:
        {
            // Warning: DRILLORIGIN and GRIDORIGIN undo/redo command create EDA_ITEMs
            // that cannot be casted to BOARD_ITEMs
            EDA_ITEM* image = aList->GetPickedItemLink( ii );
            VECTOR2D origin = image->GetPosition();
            image->SetPosition( eda_item->GetPosition() );

            if( aList->GetPickedItemStatus( ii ) == UNDO_REDO::DRILLORIGIN )
                BOARD_EDITOR_CONTROL::DoSetDrillOrigin( view, this, eda_item, origin );
            else
                PCB_CONTROL::DoSetGridOrigin( view, this, eda_item, origin );

            break;
        }

        case UNDO_REDO::PAGESETTINGS:
            if( eda_item->Type() == WS_PROXY_UNDO_ITEM_T || eda_item->Type() == WS_PROXY_UNDO_ITEM_PLUS_T )
            {
                // swap current settings with stored settings
                DS_PROXY_UNDO_ITEM  alt_item( this );
                DS_PROXY_UNDO_ITEM* item = static_cast<DS_PROXY_UNDO_ITEM*>( eda_item );
                item->Restore( this );
                *item = std::move( alt_item );
            }

            break;

        default:
            KI_FAIL_MSG( wxString::Format( wxT( "PutDataInPreviousState() error (unknown code %X)" ),
                                          aList->GetPickedItemStatus( ii ) ) );
            break;
        }

        if( eda_item->Type() == PCB_FOOTPRINT_T )
        {
            FOOTPRINT* fp = static_cast<FOOTPRINT*>( eda_item );
            fp->InvalidateComponentClassCache();
            m_pcb->GetComponentClassManager().RebuildRequiredCaches( fp );
        }
    }

    if( not_found )
=======
    GetBoard()->IncrementTimeStamp();   // clear caches

    // Enum to track the modification type of items. Used to enable bulk BOARD_LISTENER
    // callbacks at the end of the undo / redo operation
    enum ITEM_CHANGE_TYPE
    {
        ADDED,
        DELETED,
        CHANGED
    };

    std::unordered_map<EDA_ITEM*, ITEM_CHANGE_TYPE> item_changes;

    auto clear_local_ratsnest_flags =
            [&]( EDA_ITEM* item )
            {
                switch( item->Type() )
                {
                case PCB_TRACE_T:
                case PCB_ARC_T:
                case PCB_VIA_T:
                    static_cast<PCB_TRACK*>( item )->SetLocalRatsnestVisible( current_show_ratsnest );
                    break;

                case PCB_ZONE_T:
                    static_cast<ZONE*>( item )->SetLocalRatsnestVisible( current_show_ratsnest );
                    break;

                case PCB_FOOTPRINT_T:
                    for( PAD* pad : static_cast<FOOTPRINT*>( item )->Pads() )
                        pad->SetLocalRatsnestVisible( current_show_ratsnest );

                    break;

                default:
                    break;
                }
            };

    auto update_item_change_state =
            [&]( EDA_ITEM* item, ITEM_CHANGE_TYPE change_type )
            {
                auto item_itr = item_changes.find( item );

                if( item_itr == item_changes.end() )
                {
                    // First time we've seen this item - tag the current change type
                    item_changes.insert( { item, change_type } );
                    return;
                }

                // Update the item state based on the current and next change type
                switch( item_itr->second )
                {
                case ITEM_CHANGE_TYPE::ADDED:
                    if( change_type == ITEM_CHANGE_TYPE::DELETED )
                    {
                        // The item was previously added, now deleted - as far as bulk callbacks
                        // are concerned, the item has never existed
                        item_changes.erase( item_itr );
                    }
                    else if( change_type == ITEM_CHANGE_TYPE::ADDED )
                    {
                        // Error condition - added an already added item
                        KI_ASSERT_MSG( false, wxT( "UndoRedo: should not add already added item" ) );
                    }

                    // For all other cases, the item remains as ADDED as seen by the bulk callbacks
                    break;

                case ITEM_CHANGE_TYPE::DELETED:
                    // This is an error condition - item has already been deleted so should not
                    // be operated on further
                    KI_ASSERT_MSG( false, wxT( "UndoRedo: should not alter already deleted item" ) );
                    break;

                case ITEM_CHANGE_TYPE::CHANGED:
                    if( change_type == ITEM_CHANGE_TYPE::DELETED )
                    {
                        item_itr->second = ITEM_CHANGE_TYPE::DELETED;
                    }
                    else if( change_type == ITEM_CHANGE_TYPE::ADDED )
                    {
                        // This is an error condition - item has already been changed so should not
                        // be added
                        KI_ASSERT_MSG( false, wxT( "UndoRedo: should not add already changed item" ) );
                    }

                    // Otherwise, item remains CHANGED
                    break;
                }
            };

    // Undo in the reverse order of list creation: (this can allow stacked changes
    // like the same item can be changes and deleted in the same complex command

    // Restore changes in reverse order
    for( int ii = (int) aList->GetCount() - 1; ii >= 0 ; ii-- )
    {
        EDA_ITEM* eda_item = aList->GetPickedItem( (unsigned) ii );

        /* Test for existence of item on board.
         * It could be deleted, and no more on board:
         *   - if a call to SaveCopyInUndoList was forgotten in Pcbnew
         *   - in zones outlines, when a change in one zone merges this zone with an other
         * This test avoids a Pcbnew crash
         * Obviously, this test is not made for deleted items
         */
        UNDO_REDO status = aList->GetPickedItemStatus( ii );

        if( status != UNDO_REDO::DELETED
                && status != UNDO_REDO::DRILLORIGIN     // origin markers never on board
                && status != UNDO_REDO::GRIDORIGIN      // origin markers never on board
                && status != UNDO_REDO::PAGESETTINGS )  // nor are page settings proxy items
        {
            if( !GetBoard()->ResolveItem( eda_item->m_Uuid, true ) )
            {
                // Remove this non existent item
                aList->RemovePicker( ii );
                not_found = true;

                if( aList->GetCount() == 0 )
                    break;

                continue;
            }
        }

        // see if we must rebuild ratsnets and pointers lists
        switch( eda_item->Type() )
        {
        case PCB_FOOTPRINT_T:
            deep_reBuild_ratsnest = true;   // Pointers on pads can be invalid
            KI_FALLTHROUGH;

        case PCB_ZONE_T:
        case PCB_TRACE_T:
        case PCB_ARC_T:
        case PCB_VIA_T:
        case PCB_PAD_T:
            reBuild_ratsnest = true;
            break;

        case PCB_NETINFO_T:
            reBuild_ratsnest = true;
            deep_reBuild_ratsnest = true;
            break;

        default:
            break;
        }

        switch( eda_item->Type() )
        {
        case PCB_FOOTPRINT_T:
            solder_mask_dirty = true;
            break;

        case PCB_VIA_T:
            solder_mask_dirty = true;
            break;

        case PCB_ZONE_T:
        case PCB_TRACE_T:
        case PCB_ARC_T:
        case PCB_PAD_T:
        case PCB_SHAPE_T:
        {
            LSET layers = static_cast<BOARD_ITEM*>( eda_item )->GetLayerSet();

            if( layers.test( F_Mask ) || layers.test( B_Mask ) )
                solder_mask_dirty = true;

            break;
        }

        default:
            break;
        }

        switch( aList->GetPickedItemStatus( ii ) )
        {
        case UNDO_REDO::CHANGED:    /* Exchange old and new data for each item */
            if( eda_item->IsBOARD_ITEM() )
            {
                BOARD_ITEM*           item = static_cast<BOARD_ITEM*>( eda_item );
                BOARD_ITEM*           image = static_cast<BOARD_ITEM*>( aList->GetPickedItemLink( ii ) );
                BOARD_ITEM_CONTAINER* parent = GetBoard();

                // The stored pointer can be stale if a swap (e.g. ExchangeFootprint)
                // replaced the live item earlier. Resolve by UUID to find the current one.
                if( BOARD_ITEM* resolved = GetBoard()->ResolveItem( item->m_Uuid, true ) )
                    item = resolved;

                if( item->GetParentFootprint() )
                    parent = item->GetParentFootprint();

                view->Remove( item );
                parent->Remove( item, REMOVE_MODE::BULK );

                if( item->Type() != PCB_MARKER_T )
                    item->SwapItemData( image );

                clear_local_ratsnest_flags( item );
                item->ClearFlags( UR_TRANSIENT );
                image->SetFlags( UR_TRANSIENT );

                view->Add( item );
                view->Hide( item, false );
                parent->Add( item, ADD_MODE::BULK_INSERT );

                if( item->Type() == PCB_ZONE_T && static_cast<ZONE*>( item )->GetIsRuleArea() )
                {
                    dirty_rule_areas.push_back( item->GetBoundingBox() );
                    dirty_rule_areas.push_back( image->GetBoundingBox() );
                }

                update_item_change_state( item, ITEM_CHANGE_TYPE::CHANGED );
            }

            break;

        case UNDO_REDO::NEWITEM:        /* new items are deleted */
            if( eda_item->IsBOARD_ITEM() )
            {
                BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( eda_item );

                aList->SetPickedItemStatus( UNDO_REDO::DELETED, ii );

                if( FOOTPRINT* parentFP = boardItem->GetParentFootprint() )
                    parentFP->Remove( boardItem );
                else
                    GetModel()->Remove( boardItem, REMOVE_MODE::BULK );

                if( PCB_GROUP* group = dynamic_cast<PCB_GROUP*>( boardItem ) )
                    group->RemoveAll();

                update_item_change_state( eda_item, ITEM_CHANGE_TYPE::DELETED );

                if( eda_item->Type() != PCB_NETINFO_T )
                    view->Remove( eda_item );

                eda_item->SetFlags( UR_TRANSIENT );

                if( eda_item->Type() == PCB_ZONE_T && static_cast<ZONE*>( eda_item )->GetIsRuleArea() )
                    dirty_rule_areas.push_back( eda_item->GetBoundingBox() );
            }

            break;

        case UNDO_REDO::DELETED:    /* deleted items are put in List, as new items */
            if( eda_item->IsBOARD_ITEM() )
            {
                BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( eda_item );

                aList->SetPickedItemStatus( UNDO_REDO::NEWITEM, ii );

                clear_local_ratsnest_flags( eda_item );
                eda_item->ClearFlags( UR_TRANSIENT );

                if( FOOTPRINT* parentFP = boardItem->GetParentFootprint() )
                    parentFP->Add( boardItem );
                else
                    GetModel()->Add( boardItem, ADD_MODE::BULK_APPEND );

                update_item_change_state( eda_item, ITEM_CHANGE_TYPE::ADDED );

                if( eda_item->Type() != PCB_NETINFO_T )
                    view->Add( eda_item );

                if( eda_item->Type() == PCB_ZONE_T && static_cast<ZONE*>( eda_item )->GetIsRuleArea() )
                    dirty_rule_areas.push_back( eda_item->GetBoundingBox() );
            }

            break;

        case UNDO_REDO::DRILLORIGIN:
        case UNDO_REDO::GRIDORIGIN:
        {
            // Warning: DRILLORIGIN and GRIDORIGIN undo/redo command create EDA_ITEMs
            // that cannot be casted to BOARD_ITEMs
            EDA_ITEM* image = aList->GetPickedItemLink( ii );
            VECTOR2D origin = image->GetPosition();
            image->SetPosition( eda_item->GetPosition() );

            if( aList->GetPickedItemStatus( ii ) == UNDO_REDO::DRILLORIGIN )
                BOARD_EDITOR_CONTROL::DoSetDrillOrigin( view, this, eda_item, origin );
            else
                PCB_CONTROL::DoSetGridOrigin( view, this, eda_item, origin );

            break;
        }

        case UNDO_REDO::PAGESETTINGS:
            if( eda_item->Type() == WS_PROXY_UNDO_ITEM_T || eda_item->Type() == WS_PROXY_UNDO_ITEM_PLUS_T )
            {
                // swap current settings with stored settings
                DS_PROXY_UNDO_ITEM  alt_item( this );
                DS_PROXY_UNDO_ITEM* item = static_cast<DS_PROXY_UNDO_ITEM*>( eda_item );
                item->Restore( this );
                *item = std::move( alt_item );
            }

            break;

        default:
            KI_FAIL_MSG( wxString::Format( wxT( "PutDataInPreviousState() error (unknown code %X)" ),
                                          aList->GetPickedItemStatus( ii ) ) );
            break;
        }

        if( eda_item->Type() == PCB_FOOTPRINT_T )
        {
            FOOTPRINT* fp = static_cast<FOOTPRINT*>( eda_item );
            fp->InvalidateComponentClassCache();
            m_pcb->GetComponentClassManager().RebuildRequiredCaches( fp );
        }
    }

    if( not_found )
>>>>>>> ed8c16703e
        wxMessageBox( _( "Incomplete undo/redo operation: some items not found" ) );

    if( boardEditor )
    {
<<<<<<< b2e17e0a
        if( !result.dirtyRuleAreas.empty()
                && (   GetPcbNewSettings()->m_Display.m_TrackClearance == SHOW_WITH_VIA_ALWAYS
                    || GetPcbNewSettings()->m_Display.m_PadClearance ) )
||||||| 1304b76aad
        ITEM_PICKER& wrapper = aList->GetItemWrapper( ii );

        if( wrapper.GetStatus() == UNDO_REDO::DELETED )
            continue;

        BOARD_ITEM* parentGroup = GetBoard()->ResolveItem( wrapper.GetGroupId(), true );
        wrapper.GetItem()->SetParentGroup( dynamic_cast<PCB_GROUP*>( parentGroup ) );

        // Restore the group's member list, which BOARD::Remove() cleared above.
        if( PCB_GROUP* parentPcbGroup = dynamic_cast<PCB_GROUP*>( parentGroup ) )
            parentPcbGroup->GetItems().insert( wrapper.GetItem() );

        if( EDA_GROUP* group = dynamic_cast<PCB_GROUP*>( wrapper.GetItem() ) )
=======
        ITEM_PICKER& wrapper = aList->GetItemWrapper( ii );

        if( wrapper.GetStatus() == UNDO_REDO::DELETED )
            continue;

        BOARD_ITEM* parentGroup = GetBoard()->ResolveItem( wrapper.GetGroupId(), true );
        BOARD_ITEM* boardItem = GetBoard()->ResolveItem( wrapper.GetItem()->m_Uuid, true );

        if( boardItem )
            boardItem->SetParentGroup( dynamic_cast<PCB_GROUP*>( parentGroup ) );

        // Restore the group's member list, which BOARD::Remove() cleared above.
        if( PCB_GROUP* parentPcbGroup = dynamic_cast<PCB_GROUP*>( parentGroup ) )
            parentPcbGroup->GetItems().insert( boardItem );

        if( EDA_GROUP* group = dynamic_cast<PCB_GROUP*>( wrapper.GetItem() ) )
>>>>>>> ed8c16703e
        {
            GetCanvas()->GetView()->UpdateCollidingItems( result.dirtyRuleAreas,
                                                          { PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T,
                                                            PCB_PAD_T } );
        }

<<<<<<< b2e17e0a
        if( result.connectivityChanged )
            Compile_Ratsnest( false );
||||||| 1304b76aad
        // And prepare for a redo by updating group info based on current image
        if( EDA_ITEM* item = wrapper.GetLink() )
            wrapper.SetLink( item );
    }

    if( IsType( FRAME_PCB_EDITOR ) )
    {
        if( !dirty_rule_areas.empty() && (   GetPcbNewSettings()->m_Display.m_TrackClearance == SHOW_WITH_VIA_ALWAYS
                                          || GetPcbNewSettings()->m_Display.m_PadClearance ) )
        {
            view->UpdateCollidingItems( dirty_rule_areas, { PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T, PCB_PAD_T } );
        }

        if( reBuild_ratsnest || deep_reBuild_ratsnest )
        {
            // Connectivity may have changed; rebuild internal caches to remove stale items
            GetBoard()->BuildConnectivity();
            Compile_Ratsnest( false );
        }
=======
        // And prepare for a redo by updating group info based on current image
        if( EDA_ITEM* item = wrapper.GetLink() )
            wrapper.SetLink( item );
    }

    if( IsType( FRAME_PCB_EDITOR ) )
    {
        if( !dirty_rule_areas.empty() && (   GetPcbNewSettings()->m_Display.m_TrackClearance == SHOW_WITH_VIA_ALWAYS
                                          || GetPcbNewSettings()->m_Display.m_PadClearance ) )
        {
            view->UpdateCollidingItems( dirty_rule_areas, { PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T, PCB_PAD_T } );
        }

        if( reBuild_ratsnest || deep_reBuild_ratsnest )
        {
            // Connectivity may have changed; rebuild internal caches to remove stale items
            GetBoard()->BuildConnectivity();
            GetBoard()->CompileRatsnest();
        }
>>>>>>> ed8c16703e

        if( result.solderMaskChanged )
            HideSolderMask();
    }

    PCB_SELECTION_TOOL* selTool = m_toolManager->GetTool<PCB_SELECTION_TOOL>();
    selTool->RebuildSelection();

    if( aRehatchShapes && result.shapesChanged )
        GetToolManager()->PostAction( PCB_ACTIONS::rehatchShapes );

    return result.shapesChanged;
}


void PCB_BASE_EDIT_FRAME::ClearUndoORRedoList( UNDO_REDO_LIST whichList, int aItemCount )
{
    if( aItemCount == 0 )
        return;

    UNDO_REDO_CONTAINER& list = ( whichList == UNDO_LIST ) ? m_undoList : m_redoList;

    if( aItemCount < 0 )
    {
        for( PICKED_ITEMS_LIST* cmd : list.m_CommandsList )
        {
            ClearListAndDeleteItems( cmd );
            delete cmd;
        }

        list.m_CommandsList.clear();
    }
    else
    {
        for( int ii = 0; ii < aItemCount; ii++ )
        {
            if( list.m_CommandsList.size() == 0 )
                break;

            PICKED_ITEMS_LIST* curr_cmd = list.m_CommandsList[0];
            list.m_CommandsList.erase( list.m_CommandsList.begin() );
            ClearListAndDeleteItems( curr_cmd );
            delete curr_cmd;    // Delete command
        }
    }
}


void PCB_BASE_EDIT_FRAME::ClearListAndDeleteItems( PICKED_ITEMS_LIST* aList )
{
    BOARD_UNDO_REDO::DeleteItems( *aList );
}


void PCB_BASE_EDIT_FRAME::RollbackFromUndo()
{
    PICKED_ITEMS_LIST* undo = PopCommandFromUndoList();
    PutDataInPreviousState( undo );
    ClearListAndDeleteItems( undo );
    delete undo;

    m_pcb->UpdateBoardOutline();
    GetCanvas()->GetView()->Update( m_pcb->BoardOutline() );
    RefreshDrillMapOutlines( *m_pcb, GetCanvas()->GetView() );
    GetCanvas()->Refresh();
}
