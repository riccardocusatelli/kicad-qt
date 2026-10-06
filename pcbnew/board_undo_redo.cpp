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

/* Undo and redo of the changes of a board.
 *
 * An entry (PICKED_ITEMS_LIST) holds pickers (ITEM_PICKER), one for each item concerned by
 * the command.  There are 3 cases:
 *  - deleted items: the picker holds the item, which is out of the board;
 *  - changed items: the picker holds the item and a copy of it (the link) with the old data;
 *  - new items: the picker holds the item.
 *
 * Undo puts the deleted items back on the board, exchanges the data of the changed items with
 * their copies and removes the new items; the entry becomes the one that redoes the command.
 */

#include <board_undo_redo.h>

#include <unordered_map>

#include <board.h>
#include <board_commit.h>
#include <component_classes/component_class_manager.h>
#include <connectivity/connectivity_data.h>
#include <core/ki_assert.h>
#include <drawing_sheet/ds_proxy_undo_item.h>
#include <footprint.h>
#include <lset.h>
#include <pad.h>
#include <pcb_group.h>
#include <pcb_track.h>
#include <zone.h>


/// True if @a aEntry has items that can change the board outline or the shape hatching.
static bool hasShapesOrFootprints( const PICKED_ITEMS_LIST& aEntry )
{
    for( unsigned ii = 0; ii < aEntry.GetCount(); ++ii )
    {
        switch( aEntry.GetPickedItem( ii )->Type() )
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


void BOARD_UNDO_REDO::CompleteEntry( PICKED_ITEMS_LIST& aEntry, unsigned aFirst,
                                     UNDO_REDO aCommandType )
{
    for( unsigned ii = aFirst; ii < aEntry.GetCount(); ii++ )
    {
        EDA_ITEM* item    = aEntry.GetPickedItem( ii );
        UNDO_REDO command = aEntry.GetPickedItemStatus( ii );

        if( command == UNDO_REDO::UNSPECIFIED )
        {
            command = aCommandType;
            aEntry.SetPickedItemStatus( command, ii );
        }

        KI_ASSERT( item );

        switch( command )
        {
        case UNDO_REDO::CHANGED:
        case UNDO_REDO::DRILLORIGIN:
        case UNDO_REDO::GRIDORIGIN:
            // If we don't yet have a copy in the link, set one up
            if( !aEntry.GetPickedItemLink( ii ) )
                aEntry.SetPickedItemLink( BOARD_COMMIT::MakeImage( item ), ii );

            break;

        case UNDO_REDO::NEWITEM:
        case UNDO_REDO::DELETED:
        case UNDO_REDO::PAGESETTINGS:
            break;

        default:
            KI_FAIL_MSG( wxString::Format( wxT( "Unrecognized undo command: %X" ), command ) );
            break;
        }
    }
}


BOARD_UNDO_REDO::RESULT BOARD_UNDO_REDO::Apply( BOARD& aBoard, PICKED_ITEMS_LIST& aEntry,
                                                LISTENER& aListener, bool aRebuildConnectivity )
{
    RESULT     result;
    const bool showRatsnest = aListener.ShowRatsnest();

    result.shapesChanged = hasShapesOrFootprints( aEntry );

    aBoard.IncrementTimeStamp();   // clear caches

    // The change of each item, for the bulk BOARD_LISTENER callbacks at the end
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
                    static_cast<PCB_TRACK*>( item )->SetLocalRatsnestVisible( showRatsnest );
                    break;

                case PCB_ZONE_T:
                    static_cast<ZONE*>( item )->SetLocalRatsnestVisible( showRatsnest );
                    break;

                case PCB_FOOTPRINT_T:
                    for( PAD* pad : static_cast<FOOTPRINT*>( item )->Pads() )
                        pad->SetLocalRatsnestVisible( showRatsnest );

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
    for( int ii = (int) aEntry.GetCount() - 1; ii >= 0 ; ii-- )
    {
        EDA_ITEM* eda_item = aEntry.GetPickedItem( (unsigned) ii );

        /* Test for existence of item on board.
         * It could be deleted, and no more on board:
         *   - if a change was not recorded in the undo list
         *   - in zones outlines, when a change in one zone merges this zone with an other
         * This test avoids a crash
         * Obviously, this test is not made for deleted items
         */
        UNDO_REDO status = aEntry.GetPickedItemStatus( ii );

        if( status != UNDO_REDO::DELETED
                && status != UNDO_REDO::DRILLORIGIN     // origin markers never on board
                && status != UNDO_REDO::GRIDORIGIN      // origin markers never on board
                && status != UNDO_REDO::PAGESETTINGS )  // nor are page settings proxy items
        {
            if( !aBoard.ResolveItem( eda_item->m_Uuid, true ) )
            {
                // Remove this non existent item
                aEntry.RemovePicker( ii );
                result.itemsNotFound = true;

                if( aEntry.GetCount() == 0 )
                    break;

                continue;
            }
        }

        // see if we must rebuild the connectivity
        switch( eda_item->Type() )
        {
        case PCB_FOOTPRINT_T:
        case PCB_ZONE_T:
        case PCB_TRACE_T:
        case PCB_ARC_T:
        case PCB_VIA_T:
        case PCB_PAD_T:
        case PCB_NETINFO_T:
            result.connectivityChanged = true;
            break;

        default:
            break;
        }

        switch( eda_item->Type() )
        {
        case PCB_FOOTPRINT_T:
        case PCB_VIA_T:
            result.solderMaskChanged = true;
            break;

        case PCB_ZONE_T:
        case PCB_TRACE_T:
        case PCB_ARC_T:
        case PCB_PAD_T:
        case PCB_SHAPE_T:
        {
            LSET layers = static_cast<BOARD_ITEM*>( eda_item )->GetLayerSet();

            if( layers.test( F_Mask ) || layers.test( B_Mask ) )
                result.solderMaskChanged = true;

            break;
        }

        default:
            break;
        }

        switch( aEntry.GetPickedItemStatus( ii ) )
        {
        case UNDO_REDO::CHANGED:    /* Exchange old and new data for each item */
            if( eda_item->IsBOARD_ITEM() )
            {
                BOARD_ITEM*           item = static_cast<BOARD_ITEM*>( eda_item );
                BOARD_ITEM*           image = static_cast<BOARD_ITEM*>( aEntry.GetPickedItemLink( ii ) );
                BOARD_ITEM_CONTAINER* parent = &aBoard;

                // The stored pointer can be stale if a swap (e.g. ExchangeFootprint)
                // replaced the live item earlier. Resolve by UUID to find the current one.
                if( BOARD_ITEM* resolved = aBoard.ResolveItem( item->m_Uuid, true ) )
                    item = resolved;

                if( item->GetParentFootprint() )
                    parent = item->GetParentFootprint();

                aListener.ItemRemoved( item );
                parent->Remove( item, REMOVE_MODE::BULK );

                item->SwapItemData( image );

                clear_local_ratsnest_flags( item );
                item->ClearFlags( UR_TRANSIENT );
                image->SetFlags( UR_TRANSIENT );

                aListener.ItemAdded( item );
                parent->Add( item, ADD_MODE::BULK_INSERT );

                if( item->Type() == PCB_ZONE_T && static_cast<ZONE*>( item )->GetIsRuleArea() )
                {
                    result.dirtyRuleAreas.push_back( item->GetBoundingBox() );
                    result.dirtyRuleAreas.push_back( image->GetBoundingBox() );
                }

                update_item_change_state( item, ITEM_CHANGE_TYPE::CHANGED );
            }

            break;

        case UNDO_REDO::NEWITEM:        /* new items are deleted */
            if( eda_item->IsBOARD_ITEM() )
            {
                BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( eda_item );

                aEntry.SetPickedItemStatus( UNDO_REDO::DELETED, ii );

                if( FOOTPRINT* parentFP = boardItem->GetParentFootprint() )
                    parentFP->Remove( boardItem );
                else
                    aBoard.Remove( boardItem, REMOVE_MODE::BULK );

                update_item_change_state( eda_item, ITEM_CHANGE_TYPE::DELETED );

                if( eda_item->Type() != PCB_NETINFO_T )
                    aListener.ItemRemoved( eda_item );

                eda_item->SetFlags( UR_TRANSIENT );

                if( eda_item->Type() == PCB_ZONE_T && static_cast<ZONE*>( eda_item )->GetIsRuleArea() )
                    result.dirtyRuleAreas.push_back( eda_item->GetBoundingBox() );
            }

            break;

        case UNDO_REDO::DELETED:    /* deleted items are put in List, as new items */
            if( eda_item->IsBOARD_ITEM() )
            {
                BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( eda_item );

                aEntry.SetPickedItemStatus( UNDO_REDO::NEWITEM, ii );

                clear_local_ratsnest_flags( eda_item );
                eda_item->ClearFlags( UR_TRANSIENT );

                if( FOOTPRINT* parentFP = boardItem->GetParentFootprint() )
                    parentFP->Add( boardItem );
                else
                    aBoard.Add( boardItem, ADD_MODE::BULK_APPEND );

                update_item_change_state( eda_item, ITEM_CHANGE_TYPE::ADDED );

                if( eda_item->Type() != PCB_NETINFO_T )
                    aListener.ItemAdded( eda_item );

                if( eda_item->Type() == PCB_ZONE_T && static_cast<ZONE*>( eda_item )->GetIsRuleArea() )
                    result.dirtyRuleAreas.push_back( eda_item->GetBoundingBox() );
            }

            break;

        case UNDO_REDO::DRILLORIGIN:
        case UNDO_REDO::GRIDORIGIN:
        {
            // Warning: DRILLORIGIN and GRIDORIGIN undo/redo command create EDA_ITEMs
            // that cannot be casted to BOARD_ITEMs
            EDA_ITEM* image = aEntry.GetPickedItemLink( ii );
            VECTOR2D  origin = image->GetPosition();
            image->SetPosition( eda_item->GetPosition() );

            aListener.SetOrigin( aEntry.GetPickedItemStatus( ii ), eda_item, origin );
            break;
        }

        case UNDO_REDO::PAGESETTINGS:
            if( eda_item->Type() == WS_PROXY_UNDO_ITEM_T || eda_item->Type() == WS_PROXY_UNDO_ITEM_PLUS_T )
                aListener.SwapPageSettings( static_cast<DS_PROXY_UNDO_ITEM*>( eda_item ) );

            break;

        default:
            KI_FAIL_MSG( wxString::Format( wxT( "BOARD_UNDO_REDO::Apply() error (unknown code %X)" ),
                                           aEntry.GetPickedItemStatus( ii ) ) );
            break;
        }

        if( eda_item->Type() == PCB_FOOTPRINT_T )
        {
            FOOTPRINT* fp = static_cast<FOOTPRINT*>( eda_item );
            fp->InvalidateComponentClassCache();
            aBoard.GetComponentClassManager().RebuildRequiredCaches( fp );
        }
    }

    // We have now swapped all the group parent and group member pointers.  But it is a
    // risky proposition to bet on the pointers being invariant, so validate them all.
    for( int ii = 0; ii < (int) aEntry.GetCount(); ++ii )
    {
        ITEM_PICKER& wrapper = aEntry.GetItemWrapper( ii );

        if( wrapper.GetStatus() == UNDO_REDO::DELETED )
            continue;

        BOARD_ITEM* parentGroup = aBoard.ResolveItem( wrapper.GetGroupId(), true );
        wrapper.GetItem()->SetParentGroup( dynamic_cast<PCB_GROUP*>( parentGroup ) );

        // Restore the group's member list, which BOARD::Remove() cleared above.
        if( PCB_GROUP* parentPcbGroup = dynamic_cast<PCB_GROUP*>( parentGroup ) )
            parentPcbGroup->GetItems().insert( wrapper.GetItem() );

        if( EDA_GROUP* group = dynamic_cast<PCB_GROUP*>( wrapper.GetItem() ) )
        {
            // Items list may contain dodgy pointers, so don't use RemoveAll()
            group->GetItems().clear();

            for( const KIID& member : wrapper.GetGroupMembers() )
            {
                if( BOARD_ITEM* memberItem = aBoard.ResolveItem( member, true ) )
                    group->AddItem( memberItem );
            }
        }

        // And prepare for a redo by updating group info based on current image
        if( EDA_ITEM* item = wrapper.GetLink() )
            wrapper.SetLink( item );
    }

    // Connectivity may have changed; rebuild internal caches to remove stale items
    if( aRebuildConnectivity && result.connectivityChanged )
        aBoard.BuildConnectivity();

    aBoard.GetComponentClassManager().InvalidateComponentClasses();
    aBoard.SanitizeNetcodes();

    // Invoke bulk BOARD_LISTENER callbacks
    std::vector<BOARD_ITEM*> added_items, deleted_items, changed_items;

    for( auto& [item, changeType] : item_changes )
    {
        switch( changeType )
        {
        case ITEM_CHANGE_TYPE::ADDED:
            added_items.push_back( static_cast<BOARD_ITEM*>( item ) );
            break;

        case ITEM_CHANGE_TYPE::DELETED:
            deleted_items.push_back( static_cast<BOARD_ITEM*>( item ) );
            break;

        case ITEM_CHANGE_TYPE::CHANGED:
            changed_items.push_back( static_cast<BOARD_ITEM*>( item ) );
            break;
        }
    }

    if( added_items.size() > 0 || deleted_items.size() > 0 || changed_items.size() > 0 )
        aBoard.OnItemsCompositeUpdate( added_items, deleted_items, changed_items );

    return result;
}


void BOARD_UNDO_REDO::DeleteItems( PICKED_ITEMS_LIST& aEntry )
{
    aEntry.ClearListAndDeleteItems(
            []( EDA_ITEM* item )
            {
                KI_ASSERT_MSG( item->HasFlag( UR_TRANSIENT ),
                               "Item on undo/redo list not owned by undo/redo!" );

                delete item;
            } );
}
