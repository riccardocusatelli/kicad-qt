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

#include <macros.h>
#include <board.h>
#include <component_classes/component_class_manager.h>
#include <footprint.h>
#include <lset.h>
#include <pad.h>
#include <pcb_group.h>
#include <pcb_track.h>
#include <pcb_shape.h>
#include <board_commit.h>
#include <connectivity/connectivity_data.h>
#include <connectivity/connectivity_algo.h>
#include <teardrop/teardrop.h>
#include <pcb_board_outline.h>

#include <functional>
#include <unordered_set>
#include <project/project_file.h>
#include <core/ki_assert.h>
using namespace std::placeholders;


BOARD_COMMIT::BOARD_COMMIT( BOARD* aBoard, bool aIsBoardEditor, bool aIsFootprintEditor ) :
        COMMIT(),
        m_board( aBoard ),
        m_isBoardEditor( aIsBoardEditor ),
        m_isFootprintEditor( aIsFootprintEditor )
{
}


BOARD_COMMIT::BOARD_COMMIT( std::unique_ptr<BOARD_COMMIT_LISTENER> aListener,
                            bool aIsBoardEditor, bool aIsFootprintEditor ) :
        COMMIT(),
        m_board( nullptr ),
        m_listener( std::move( aListener ) ),
        m_isBoardEditor( aIsBoardEditor ),
        m_isFootprintEditor( aIsFootprintEditor )
{
}


BOARD_COMMIT::~BOARD_COMMIT()
{
}


BOARD* BOARD_COMMIT::GetBoard() const
{
    return m_listener ? m_listener->Board() : m_board;
}


COMMIT& BOARD_COMMIT::Stage( EDA_ITEM* aItem, CHANGE_TYPE aChangeType, BASE_SCREEN* aScreen, RECURSE_MODE aRecurse )
{
    if( aRecurse == RECURSE_MODE::RECURSE )
    {
        if( PCB_GROUP* group = dynamic_cast<PCB_GROUP*>( aItem ) )
        {
            for( EDA_ITEM* member : group->GetItems() )
                Stage( member, aChangeType, aScreen, aRecurse );
        }
    }

    if( m_isBoardEditor && ( aChangeType & CHT_TYPE ) == CHT_REMOVE
            && aItem->IsBOARD_ITEM()
            && static_cast<BOARD_ITEM*>( aItem )->GetParentFootprint() )
    {
        if( m_deletedItems.find( { aItem, aScreen } ) == m_deletedItems.end() )
            makeEntry( aItem, aChangeType, makeImage( aItem ), aScreen );

        return *this;
    }

    return COMMIT::Stage( aItem, aChangeType );
}


COMMIT& BOARD_COMMIT::Stage( std::vector<EDA_ITEM*>& container, CHANGE_TYPE aChangeType, BASE_SCREEN* aScreen )
{
    return COMMIT::Stage( container, aChangeType, aScreen );
}


COMMIT& BOARD_COMMIT::Stage( const PICKED_ITEMS_LIST& aItems, UNDO_REDO aModFlag, BASE_SCREEN* aScreen )
{
    return COMMIT::Stage( aItems, aModFlag, aScreen );
}


void BOARD_COMMIT::propagateDamage( BOARD_ITEM* aChangedItem, std::vector<ZONE*>* aStaleZones,
                                    std::vector<BOX2I>& aStaleRuleAreas )
{
    KI_CHECK( aChangedItem, /* void */ );

    if( aStaleZones && aChangedItem->Type() == PCB_ZONE_T )
        aStaleZones->push_back( static_cast<ZONE*>( aChangedItem ) );

    aChangedItem->RunOnChildren( std::bind( &BOARD_COMMIT::propagateDamage, this, _1, aStaleZones, aStaleRuleAreas ),
                                 RECURSE_MODE::NO_RECURSE );

    BOARD* board = GetBoard();
    BOX2I  damageBBox = aChangedItem->GetBoundingBox();
    LSET   damageLayers = aChangedItem->GetLayerSet();

    if( m_isBoardEditor && aChangedItem->Type() == PCB_ZONE_T )
    {
        // A named zone can have custom DRC rules targetting it.
        if( !static_cast<ZONE*>( aChangedItem )->GetZoneName().IsEmpty() )
            aStaleRuleAreas.push_back( damageBBox );
    }

    if( aStaleZones )
    {
        if( damageLayers.test( Edge_Cuts ) || damageLayers.test( Margin ) )
            damageLayers = LSET::PhysicalLayersMask();
        else
            damageLayers &= LSET::AllCuMask();

        if( damageLayers.any() )
        {
            for( ZONE* zone : board->Zones() )
            {
                if( zone->GetIsRuleArea() )
                    continue;

                if( ( zone->GetLayerSet() & damageLayers ).any() && zone->GetBoundingBox().Intersects( damageBBox ) )
                    aStaleZones->push_back( zone );
            }
        }
    }
}


void BOARD_COMMIT::Push( const wxString& aMessage, int aCommitFlags )
{
    BOARD_COMMIT_LISTENER* listener = m_listener.get();
    BOARD*                 board = GetBoard();
    PCB_GROUP*             enteredGroup = listener && !( aCommitFlags & SKIP_ENTERED_GROUP )
                                                ? listener->EnteredGroup()
                                                : nullptr;
    // Notification info
    PICKED_ITEMS_LIST                  undoList;
    BOARD_COMMIT_LISTENER::PUSH_RESULT result;

    std::unordered_set<EDA_ITEM*> removedItems;

    // Dirty flags and lists
    bool                     solderMaskDirty = false;
    bool                     updateBoardBoundingBox = false;
    std::vector<BOARD_ITEM*> staleTeardropPadsAndVias;
    std::set<PCB_TRACK*>     staleTeardropTracks;
    std::vector<ZONE*>*      staleZones = nullptr;
    std::vector<BOX2I>       staleRuleAreas;

    if( Empty() )
        return;

    undoList.SetDescription( aMessage );

    TEARDROP_MANAGER                   teardropMgr( board );
    std::shared_ptr<CONNECTIVITY_DATA> connectivity = board->GetConnectivity();

    std::vector<BOARD_ITEM*> bulkAddedItems;
    std::vector<BOARD_ITEM*> bulkRemovedItems;
    std::vector<BOARD_ITEM*> itemsChanged;

    if( m_isBoardEditor && !( aCommitFlags & ZONE_FILL_OP ) && listener && listener->AutoRefillZones() )
    {
        result.autofillZones = true;
        staleZones = &result.staleZones;

        for( ZONE* zone : board->Zones() )
            zone->CacheBoundingBox();
    }

    for( COMMIT_LINE& entry : m_entries )
    {
        if( !entry.m_item || !entry.m_item->IsBOARD_ITEM() )
            continue;

        BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( entry.m_item );

        if( m_isBoardEditor )
        {
            if( boardItem->Type() == PCB_VIA_T || boardItem->Type() == PCB_FOOTPRINT_T
                    || boardItem->IsOnLayer( F_Mask ) || boardItem->IsOnLayer( B_Mask ) )
            {
                solderMaskDirty = true;
            }

            if( boardItem->GetLayer() == Edge_Cuts )
            {
                updateBoardBoundingBox = true;
            }

            if( !( aCommitFlags & SKIP_TEARDROPS ) )
            {
                if( boardItem->Type() == PCB_FOOTPRINT_T )
                {
                    for( PAD* pad : static_cast<FOOTPRINT*>( boardItem )->Pads() )
                        staleTeardropPadsAndVias.push_back( pad );
                }
                else if( boardItem->Type() == PCB_PAD_T || boardItem->Type() == PCB_VIA_T )
                {
                    staleTeardropPadsAndVias.push_back( boardItem );
                }
                else if( boardItem->Type() == PCB_TRACE_T || boardItem->Type() == PCB_ARC_T )
                {
                    PCB_TRACK* track = static_cast<PCB_TRACK*>( boardItem );

                    staleTeardropTracks.insert( track );

                    std::vector<PAD*>     connectedPads;
                    std::vector<PCB_VIA*> connectedVias;

                    connectivity->GetConnectedPadsAndVias( track, &connectedPads, &connectedVias );

                    for( PAD* pad : connectedPads )
                        staleTeardropPadsAndVias.push_back( pad );

                    for( PCB_VIA* via : connectedVias )
                        staleTeardropPadsAndVias.push_back( via );
                }
            }
        }

        if( boardItem->IsSelected() || ( m_isFootprintEditor && boardItem == board->GetFirstFootprint() ) )
            result.selectedModified = true;
    }

    // Old teardrops must be removed before connectivity is rebuilt
    if( !staleTeardropPadsAndVias.empty() || !staleTeardropTracks.empty() )
        teardropMgr.RemoveTeardrops( *this, &staleTeardropPadsAndVias, &staleTeardropTracks );

    auto updateComponentClasses =
            [this]( BOARD_ITEM* boardItem )
            {
                if( boardItem->Type() != PCB_FOOTPRINT_T )
                    return;

                FOOTPRINT* footprint = static_cast<FOOTPRINT*>( boardItem );
                GetBoard()->GetComponentClassManager().RebuildRequiredCaches( footprint );
            };

    // We don't know that anything will be added to the entered group, but it does no harm to
    // add it to the commit anyway.
    if( enteredGroup )
        Modify( enteredGroup );


    for( COMMIT_LINE& entry : m_entries )
    {
        if( !entry.m_item || !entry.m_item->IsBOARD_ITEM() )
            continue;

        BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( entry.m_item );
        int         changeType = entry.m_type & CHT_TYPE;
        int         changeFlags = entry.m_type & CHT_FLAGS;

        switch( changeType )
        {
        case CHT_ADD:
            if( enteredGroup && boardItem->IsGroupableType() && !boardItem->GetParentGroup() )
                enteredGroup->AddItem( boardItem );

            if( !( aCommitFlags & SKIP_UNDO ) )
                undoList.PushItem( ITEM_PICKER( nullptr, boardItem, UNDO_REDO::NEWITEM ) );

            if( !( changeFlags & CHT_DONE ) )
            {
                if( m_isFootprintEditor )
                {
                    if( FOOTPRINT* parentFP = board->GetFirstFootprint() )
                        parentFP->Add( boardItem );
                }
                else
                {
                    board->Add( boardItem, ADD_MODE::BULK_INSERT ); // handles connectivity
                    bulkAddedItems.push_back( boardItem );
                }
            }

            if( boardItem->Type() != PCB_MARKER_T )
                propagateDamage( boardItem, staleZones, staleRuleAreas );

            if( listener && boardItem->Type() != PCB_NETINFO_T )
                listener->ItemAdded( boardItem );

            updateComponentClasses( boardItem );

            break;

        case CHT_REMOVE:
        {
            EDA_GROUP* parentGroup = boardItem->GetParentGroup();

            if( !( aCommitFlags & SKIP_UNDO ) )
            {
                UNDO_REDO   status = boardItem->Type() == PCB_FIELD_T ? UNDO_REDO::CHANGED : UNDO_REDO::DELETED;
                ITEM_PICKER itemWrapper( nullptr, boardItem, status );
                itemWrapper.SetLink( entry.m_copy );
                entry.m_copy = nullptr;   // We've transferred ownership to the undo list
                undoList.PushItem( itemWrapper );
            }

            if( boardItem->IsSelected() )
            {
                if( listener )
                    listener->Deselect( boardItem );

                result.itemsDeselected = true;
            }

            removedItems.insert( boardItem );

            if( parentGroup && !( parentGroup->AsEdaItem()->GetFlags() & STRUCT_DELETED ) )
                parentGroup->RemoveItem( boardItem );

            if( boardItem->Type() != PCB_MARKER_T )
                propagateDamage( boardItem, staleZones, staleRuleAreas );

            switch( boardItem->Type() )
            {
            case PCB_FIELD_T:
                static_cast<PCB_FIELD*>( boardItem )->SetVisible( false );

                if( listener )
                    listener->ItemChanged( boardItem );

                break;

            case PCB_TEXT_T:
            case PCB_PAD_T:
            case PCB_SHAPE_T:
            case PCB_REFERENCE_IMAGE_T:
            case PCB_GENERATOR_T:
            case PCB_TEXTBOX_T:
            case PCB_BARCODE_T:
            case PCB_TABLE_T:
            case PCB_TRACE_T:
            case PCB_ARC_T:
            case PCB_VIA_T:
            case PCB_DIM_ALIGNED_T:
            case PCB_DIM_CENTER_T:
            case PCB_DIM_RADIAL_T:
            case PCB_DIM_ORTHOGONAL_T:
            case PCB_DIM_LEADER_T:
            case PCB_TARGET_T:
            case PCB_MARKER_T:
            case PCB_POINT_T:
            case PCB_ZONE_T:
            case PCB_FOOTPRINT_T:
            case PCB_GROUP_T:
                if( listener )
                    listener->ItemRemoved( boardItem );

                if( !( changeFlags & CHT_DONE ) )
                {
                    if( m_isFootprintEditor )
                    {
                        if( FOOTPRINT* parentFP = board->GetFirstFootprint() )
                            parentFP->Remove( boardItem );
                    }
                    else if( FOOTPRINT* parentFP = boardItem->GetParentFootprint() )
                    {
                        parentFP->Remove( boardItem );
                    }
                    else
                    {
                        board->Remove( boardItem, REMOVE_MODE::BULK );
                        bulkRemovedItems.push_back( boardItem );
                    }
                }

                break;

            // Metadata items
            case PCB_NETINFO_T:
                board->Remove( boardItem, REMOVE_MODE::BULK );
                bulkRemovedItems.push_back( boardItem );
                break;

            default:                // other types do not need to (or should not) be handled
                KI_ASSERT( false );
                break;
            }

            // Removed items are owned by undo/redo, but a hidden field still belongs to its footprint
            if( boardItem->Type() != PCB_FIELD_T )
                boardItem->SetFlags( UR_TRANSIENT );

            break;
        }

        case CHT_MODIFY:
        {
            BOARD_ITEM* boardItemCopy = static_cast<BOARD_ITEM*>( entry.m_copy );

            if( !( aCommitFlags & SKIP_UNDO ) )
            {
                ITEM_PICKER itemWrapper( nullptr, boardItem, UNDO_REDO::CHANGED );
                itemWrapper.SetLink( entry.m_copy );
                entry.m_copy = nullptr;   // We've transferred ownership to the undo list
                undoList.PushItem( itemWrapper );
            }

            if( !( aCommitFlags & SKIP_CONNECTIVITY ) )
            {
                connectivity->MarkItemNetAsDirty( boardItemCopy );
                connectivity->Update( boardItem );
            }

            if( boardItem->Type() != PCB_MARKER_T )
            {
                propagateDamage( boardItemCopy, staleZones, staleRuleAreas );   // before
                propagateDamage( boardItem, staleZones, staleRuleAreas );       // after
            }

            updateComponentClasses( boardItem );

            if( listener && boardItem->Type() != PCB_NETINFO_T )
                listener->ItemChanged( boardItem );

            itemsChanged.push_back( boardItem );
            break;
        }

        default:
            UNIMPLEMENTED_FOR( boardItem->GetClass() );
            break;
        }

        // Delete any copies we still have ownership of
        delete entry.m_copy;
        entry.m_copy = nullptr;

        boardItem->ClearEditFlags();
        boardItem->RunOnChildren(
                [&]( BOARD_ITEM* item )
                {
                    item->ClearEditFlags();
                },
                RECURSE_MODE::RECURSE );
    } // ... and regenerate them.

    // Deselection above keys off the SELECTED flag on the removed item itself, but the selection
    // may separately hold an owned descendant such as a pad, field or table cell.  Descendants
    // are freed along with the removed parent, so prune them here or the selection keeps a
    // dangling pointer that the next repaint dereferences.
    if( listener && !removedItems.empty() && listener->PruneSelection( removedItems ) )
        result.itemsDeselected = true;

    // Invalidate component classes
    board->GetComponentClassManager().InvalidateComponentClasses();

    if( m_isBoardEditor )
    {
        size_t originalCount = m_entries.size();

        if( aCommitFlags & SKIP_CONNECTIVITY )
        {
            connectivity->ClearRatsnest();
            connectivity->ClearLocalRatsnest();
        }
        else
        {
            connectivity->RecalculateRatsnest( this );
            board->UpdateRatsnestExclusions();
            connectivity->ClearLocalRatsnest();

            if( listener )
                listener->RatsnestChanged();

            board->OnRatsnestChanged();
        }

        if( solderMaskDirty && listener )
            listener->SolderMaskChanged();

        if( updateBoardBoundingBox && listener && board->BoardOutline() )
        {
            board->UpdateBoardOutline();
            listener->BoardOutlineChanged();
        }

        if( !staleRuleAreas.empty() && listener )
            listener->RuleAreasChanged( staleRuleAreas );

        if( !staleTeardropPadsAndVias.empty() || !staleTeardropTracks.empty() )
        {
            teardropMgr.UpdateTeardrops( *this, &staleTeardropPadsAndVias, &staleTeardropTracks );

            // UpdateTeardrops() can modify the ratsnest data. So rebuild this ratsnest data
            connectivity->RecalculateRatsnest( this );
        }

        // Log undo items for any connectivity or teardrop changes
        for( size_t i = originalCount; i < m_entries.size(); ++i )
        {
            COMMIT_LINE& entry = m_entries[i];
            BOARD_ITEM*  boardItem = nullptr;
            BOARD_ITEM*  boardItemCopy = nullptr;

            if( entry.m_item && entry.m_item->IsBOARD_ITEM() )
                boardItem = static_cast<BOARD_ITEM*>( entry.m_item );

            if( entry.m_copy && entry.m_copy->IsBOARD_ITEM() )
                boardItemCopy = static_cast<BOARD_ITEM*>( entry.m_copy );

            KI_CHECK2( boardItem, continue );

            if( !( aCommitFlags & SKIP_UNDO ) )
            {
                ITEM_PICKER itemWrapper( nullptr, boardItem, convert( entry.m_type & CHT_TYPE ) );
                itemWrapper.SetLink( boardItemCopy );
                undoList.PushItem( itemWrapper );
            }
            else
            {
                delete entry.m_copy;
            }

            if( listener && boardItem->Type() != PCB_NETINFO_T )
            {
                if( ( entry.m_type & CHT_TYPE ) == CHT_ADD )
                    listener->ItemAdded( boardItem );
                else if( ( entry.m_type & CHT_TYPE ) == CHT_REMOVE )
                    listener->ItemRemoved( boardItem );
                else
                    listener->ItemChanged( boardItem );
            }
        }
    }

    if( bulkAddedItems.size() > 0 || bulkRemovedItems.size() > 0 || itemsChanged.size() > 0 )
        board->OnItemsCompositeUpdate( bulkAddedItems, bulkRemovedItems, itemsChanged );

    if( listener )
        listener->Pushed( undoList, aCommitFlags, result );

    clear();
}


EDA_ITEM* BOARD_COMMIT::undoLevelItem( EDA_ITEM* aItem ) const
{
    // Easiest way to disallow both a parent and one of its children appearing in the list
    // is to only ever add the parent when either can be legal (ie: in the board editor).
    if( m_isBoardEditor && aItem->IsBOARD_ITEM() )
    {
        if( FOOTPRINT* footprint = static_cast<BOARD_ITEM*>( aItem )->GetParentFootprint() )
            return footprint;
    }

    EDA_ITEM* parent = aItem->GetParent();

    if( parent && parent->Type() == PCB_TABLE_T )
        return parent;

    return aItem;
}


EDA_ITEM* BOARD_COMMIT::makeImage( EDA_ITEM* aItem ) const
{
    return MakeImage( aItem );
}


EDA_ITEM* BOARD_COMMIT::MakeImage( EDA_ITEM* aItem )
{
    EDA_ITEM* clone = aItem->Clone();
    clone->SetFlags( UR_TRANSIENT );

    return clone;
}


void BOARD_COMMIT::Revert()
{
    BOARD_COMMIT_LISTENER*             listener = m_listener.get();
    BOARD*                             board = GetBoard();
    std::shared_ptr<CONNECTIVITY_DATA> connectivity = board->GetConnectivity();

    board->IncrementTimeStamp();   // clear caches

    auto updateComponentClasses =
            [this]( BOARD_ITEM* boardItem )
            {
                if( boardItem->Type() != PCB_FOOTPRINT_T )
                    return;

                FOOTPRINT* footprint = static_cast<FOOTPRINT*>( boardItem );
                GetBoard()->GetComponentClassManager().RebuildRequiredCaches( footprint );
            };

    std::vector<BOARD_ITEM*> bulkAddedItems;
    std::vector<BOARD_ITEM*> bulkRemovedItems;
    std::vector<BOARD_ITEM*> itemsChanged;
    std::vector<BOARD_ITEM*> itemsToDelete;

    for( COMMIT_LINE& entry : m_entries )
    {
        if( !entry.m_item || !entry.m_item->IsBOARD_ITEM() )
            continue;

        BOARD_ITEM*  boardItem = static_cast<BOARD_ITEM*>( entry.m_item );
        int          changeType = entry.m_type & CHT_TYPE;
        int          changeFlags = entry.m_type & CHT_FLAGS;

        switch( changeType )
        {
        case CHT_ADD:
            if( changeFlags & CHT_DONE )
            {
                if( listener && boardItem->Type() != PCB_NETINFO_T )
                    listener->ItemRemoved( boardItem );

                connectivity->Remove( boardItem );

                if( m_isFootprintEditor )
                {
                    if( FOOTPRINT* parentFP = board->GetFirstFootprint() )
                        parentFP->Remove( boardItem );
                }
                else
                {
                    board->Remove( boardItem, REMOVE_MODE::BULK );
                    bulkRemovedItems.push_back( boardItem );
                }
            }

            if( PCB_GROUP* group = dynamic_cast<PCB_GROUP*>( boardItem ) )
                group->RemoveAll();

            // Defer deletion until after OnItemsCompositeUpdate so that
            // board listeners do not receive dangling pointers.
            itemsToDelete.push_back( boardItem );
            entry.m_item = nullptr;
            continue;

        case CHT_REMOVE:
        {
            if( !( changeFlags & CHT_DONE ) )
                break;

            if( listener && boardItem->Type() != PCB_NETINFO_T )
                listener->ItemAdded( boardItem );

            if( m_isFootprintEditor )
            {
                if( FOOTPRINT* parentFP = board->GetFirstFootprint() )
                    parentFP->Add( boardItem, ADD_MODE::INSERT );
            }
            else
            {
                board->Add( boardItem, ADD_MODE::INSERT );
                bulkAddedItems.push_back( boardItem );
            }

            updateComponentClasses( boardItem );
            break;
        }

        case CHT_MODIFY:
        {
            if( listener && boardItem->Type() != PCB_NETINFO_T )
                listener->ItemRemoved( boardItem );

            connectivity->Remove( boardItem );

            KI_ASSERT( entry.m_copy && entry.m_copy->IsBOARD_ITEM() );
            BOARD_ITEM* boardItemCopy = static_cast<BOARD_ITEM*>( entry.m_copy );
            boardItem->SwapItemData( boardItemCopy );

            if( listener && boardItem->Type() != PCB_NETINFO_T )
                listener->ItemAdded( boardItem );

            connectivity->Add( boardItem );
            itemsChanged.push_back( boardItem );

            updateComponentClasses( boardItem );
            break;
        }

        default:
            UNIMPLEMENTED_FOR( boardItem->GetClass() );
            break;
        }

        // Delete any copies we still have ownership of
        delete entry.m_copy;
        entry.m_copy = nullptr;

        boardItem->ClearEditFlags();
    }

    // Invalidate component classes
    board->GetComponentClassManager().InvalidateComponentClasses();

    if( bulkAddedItems.size() > 0 || bulkRemovedItems.size() > 0 || itemsChanged.size() > 0 )
        board->OnItemsCompositeUpdate( bulkAddedItems, bulkRemovedItems, itemsChanged );

    if( m_isBoardEditor )
    {
        for( BOARD_ITEM* item : itemsToDelete )
            connectivity->Remove( item );
    }

    for( BOARD_ITEM* item : itemsToDelete )
        delete item;

    if( m_isBoardEditor )
    {
        connectivity->RecalculateRatsnest();
        board->UpdateRatsnestExclusions();
        board->OnRatsnestChanged();
    }

    if( listener )
        listener->Reverted();

    clear();
}
