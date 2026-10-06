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

#include <macros.h>
#include <core/kicad_algo.h>

#include <lib_symbol.h>

#include <sch_group.h>
#include <sch_screen.h>
#include <schematic.h>

#include <sch_commit.h>
#include <sch_field.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <sch_pin.h>
#include <undo_redo_container.h>
#include <connection_graph.h>

#include <functional>
#include <wx/log.h>
#include <core/ki_log.h>
#include <core/ki_assert.h>


SCH_COMMIT::SCH_COMMIT() :
        COMMIT(),
        m_isLibEditor( false )
{
}


SCH_COMMIT::SCH_COMMIT( std::unique_ptr<SCH_COMMIT_LISTENER> aListener ) :
        COMMIT(),
        m_listener( std::move( aListener ) ),
        m_isLibEditor( false )
{
}


SCH_COMMIT::~SCH_COMMIT()
{
}


COMMIT& SCH_COMMIT::Stage( EDA_ITEM *aItem, CHANGE_TYPE aChangeType, BASE_SCREEN *aScreen,
                           RECURSE_MODE aRecurse )
{
    KI_CHECK( aItem, *this );

    if( aRecurse == RECURSE_MODE::RECURSE )
    {
        if( SCH_GROUP* group = dynamic_cast<SCH_GROUP*>( aItem ) )
        {
            for( EDA_ITEM* member : group->GetItems() )
                Stage( member, aChangeType, aScreen, aRecurse );
        }
    }

    // IS_SELECTED flag should not be set on undo items which were added for a drag operation.
    if( aItem->IsSelected() && aItem->HasFlag( SELECTED_BY_DRAG ) )
    {
        aItem->ClearSelected();
        COMMIT::Stage( aItem, aChangeType, aScreen );
        aItem->SetSelected();
    }
    else
    {
        COMMIT::Stage( aItem, aChangeType, aScreen );
    }

    return *this;
}


COMMIT& SCH_COMMIT::Stage( std::vector<EDA_ITEM*> &container, CHANGE_TYPE aChangeType,
                           BASE_SCREEN *aScreen )
{
    for( EDA_ITEM* item : container )
        Stage( item, aChangeType, aScreen );

    return *this;
}


void SCH_COMMIT::pushSchEdit( const wxString& aMessage, int aCommitFlags )
{
    // Objects potentially interested in changes:
    SCH_COMMIT_LISTENER*             listener = m_listener.get();
    PICKED_ITEMS_LIST                undoList;
    SCH_SCREEN*                      currentScreen = listener ? listener->CurrentScreen() : nullptr;
    SCH_GROUP*                       enteredGroup = listener ? listener->EnteredGroup() : nullptr;
    SCH_COMMIT_LISTENER::PUSH_RESULT result;
    bool                             refreshHierarchy = false;
    SCH_CLEANUP_FLAGS                connectivityCleanUp = NO_CLEANUP;

    if( Empty() )
        return;

    undoList.SetDescription( aMessage );

    SCHEMATIC*             schematic = nullptr;
    std::vector<SCH_ITEM*> bulkAddedItems;
    std::vector<SCH_ITEM*> bulkRemovedItems;
    std::vector<SCH_ITEM*> itemsChanged;

    auto updateConnectivityFlag =
            [&]( SCH_ITEM* schItem )
            {
                if( schItem->IsConnectable() || ( schItem->Type() == SCH_RULE_AREA_T ) )
                {
                    result.dirtyConnectivity = true;

                    // Do a local clean up if there are any connectable objects in the commit.
                    if( connectivityCleanUp == NO_CLEANUP )
                        connectivityCleanUp = LOCAL_CLEANUP;

                    // Do a full rebuild of the connectivity if there is a sheet in the commit.
                    if( schItem->Type() == SCH_SHEET_T )
                        connectivityCleanUp = GLOBAL_CLEANUP;
                }
            };

    // We don't know that anything will be added to the entered group, but it does no harm to
    // add it to the commit anyway.
    if( enteredGroup )
        Modify( enteredGroup, currentScreen );

    // Handle wires with Hop Over shapes:
    for( COMMIT_LINE& entry : m_entries )
    {
        if( !listener )
            break;

        SCH_ITEM* schCopyItem = dynamic_cast<SCH_ITEM*>( entry.m_copy );
        SCH_ITEM* schItem = dynamic_cast<SCH_ITEM*>( entry.m_item );

        if( schCopyItem && schCopyItem->Type() == SCH_LINE_T )
            listener->UpdateHopOveredWires( schCopyItem );

        if( schItem && schItem->Type() == SCH_LINE_T )
            listener->UpdateHopOveredWires( schItem );
    }


    // Modify() appends to m_entries, so collect first and stage after the loop.
    std::vector<std::pair<EDA_GROUP*, BASE_SCREEN*>> removedItemGroups;

    for( COMMIT_LINE& entry : m_entries )
    {
        SCH_ITEM* schItem = dynamic_cast<SCH_ITEM*>( entry.m_item );
        int       changeType = entry.m_type & CHT_TYPE;

        KI_CHECK2( schItem, continue );

        if( changeType == CHT_REMOVE && schItem->GetParentGroup() )
            removedItemGroups.emplace_back( schItem->GetParentGroup(), entry.m_screen );
    }

    for( const auto& [group, screen] : removedItemGroups )
        Modify( group->AsEdaItem(), screen );

    for( COMMIT_LINE& entry : m_entries )
    {
        int         changeType = entry.m_type & CHT_TYPE;
        int         changeFlags = entry.m_type & CHT_FLAGS;
        SCH_ITEM*   schItem = dynamic_cast<SCH_ITEM*>( entry.m_item );
        SCH_SCREEN* screen = dynamic_cast<SCH_SCREEN*>( entry.m_screen );

        KI_CHECK2( schItem, continue );
        KI_CHECK2( screen, continue );

        if( !schematic )
            schematic = schItem->Schematic();

        if( schItem->IsSelected() )
        {
            result.selectedModified = true;
        }
        else
        {
            schItem->RunOnChildren(
                    [&result]( SCH_ITEM* aChild )
                    {
                        if( aChild->IsSelected() )
                            result.selectedModified = true;
                    },
                    RECURSE_MODE::NO_RECURSE );
        }

        switch( changeType )
        {
        case CHT_ADD:
        {
            if( enteredGroup && schItem->IsGroupableType() && !schItem->GetParentGroup() )
                enteredGroup->AddItem( schItem );

            updateConnectivityFlag( schItem );

            if( !( aCommitFlags & SKIP_UNDO ) )
                undoList.PushItem( ITEM_PICKER( screen, schItem, UNDO_REDO::NEWITEM ) );

            if( !( changeFlags & CHT_DONE ) )
            {
                if( !screen->CheckIfOnDrawList( schItem ) )  // don't want a loop!
                    screen->Append( schItem );

                if( listener && screen == currentScreen )
                    listener->ItemAdded( schItem );
            }

            if( listener && screen == currentScreen )
                listener->UpdateItem( schItem, screen, true );
            else
                screen->Update( schItem );

            bulkAddedItems.push_back( schItem );

            if( schItem->Type() == SCH_SHEET_T )
                refreshHierarchy = true;

            break;
        }

        case CHT_REMOVE:
        {
            updateConnectivityFlag( schItem );

            if( !( aCommitFlags & SKIP_UNDO ) )
            {
                ITEM_PICKER itemWrapper( screen, schItem, UNDO_REDO::DELETED );
                itemWrapper.SetLink( entry.m_copy );
                entry.m_copy = nullptr;   // We've transferred ownership to the undo list
                undoList.PushItem( itemWrapper );
            }

            if( schItem->IsSelected() )
            {
                if( listener )
                    listener->Deselect( schItem );

                result.itemsDeselected = true;
            }

            if( schItem->Type() == SCH_FIELD_T )
            {
                static_cast<SCH_FIELD*>( schItem )->SetVisible( false );
                break;
            }

            if( EDA_GROUP* group = schItem->GetParentGroup() )
                group->RemoveItem( schItem );

            if( !( changeFlags & CHT_DONE ) )
            {
                screen->Remove( schItem );

                if( listener && screen == currentScreen )
                    listener->ItemRemoved( schItem );
            }

            if( listener && screen == currentScreen )
                listener->UpdateItem( schItem, screen, true );
            else
                screen->Update( schItem );

            if( schItem->Type() == SCH_SHEET_T )
                refreshHierarchy = true;

            bulkRemovedItems.push_back( schItem );
            break;
        }

        case CHT_MODIFY:
        {
            const SCH_ITEM* itemCopy = static_cast<const SCH_ITEM*>( entry.m_copy );
            SCH_SHEET_PATH  currentSheet;

            if( listener )
                listener->GetCurrentSheet( currentSheet );

            if( itemCopy->HasConnectivityChanges( schItem, &currentSheet )
                || ( itemCopy->Type() == SCH_RULE_AREA_T ) )
            {
                updateConnectivityFlag( schItem );
            }

            if( schItem->Type() == SCH_SYMBOL_T )
            {
                const SCH_SYMBOL* origSymbol = static_cast<const SCH_SYMBOL*>( itemCopy );
                const SCH_SYMBOL* modSymbol = static_cast<const SCH_SYMBOL*>( schItem );

                if( origSymbol->GetPins().size() != modSymbol->GetPins().size() )
                    connectivityCleanUp = GLOBAL_CLEANUP;
            }

            if( !( aCommitFlags & SKIP_UNDO ) )
            {
#if 0
                // While this keeps us from marking documents modified when someone OK's a dialog with
                // no changes, it depends on our various SCH_ITEM::operator=='s being bullet-proof. They
                // currently aren't.
                if( *itemCopy == *schItem )
                {
                    // No actual changes made; short-circuit undo
                    delete entry.m_copy;
                    entry.m_copy = nullptr;
                    break;
                }
#endif

                ITEM_PICKER itemWrapper( screen, schItem, UNDO_REDO::CHANGED );
                itemWrapper.SetLink( entry.m_copy );
                entry.m_copy = nullptr;   // We've transferred ownership to the undo list
                undoList.PushItem( itemWrapper );
            }

            if( schItem->Type() == SCH_SHEET_T )
            {
                const SCH_SHEET* modifiedSheet = static_cast<const SCH_SHEET*>( schItem );
                const SCH_SHEET* originalSheet = static_cast<const SCH_SHEET*>( itemCopy );
                KI_CHECK2( modifiedSheet && originalSheet, continue );

                if( originalSheet->HasPageNumberChanges( *modifiedSheet ) )
                    refreshHierarchy = true;
            }

            if( listener && screen == currentScreen )
                listener->UpdateItem( schItem, screen, false );
            else
                screen->Update( schItem );

            itemsChanged.push_back( schItem );
            break;
        }

        default:
            KI_ASSERT( false );
            break;
        }

        // Delete any copies we still have ownership of
        delete entry.m_copy;
        entry.m_copy = nullptr;

        // Clear all flags but SELECTED and others used to move and rotate commands,
        // after edition (selected items must keep their selection flag).
        const int selected_mask = ( SELECTED | STARTPOINT | ENDPOINT );
        schItem->ClearFlags( EDA_ITEM_ALL_FLAGS - selected_mask );

        if( schItem->Type() == SCH_SHEET_T || schItem->Type() == SCH_SYMBOL_T )
        {
            schItem->RunOnChildren(
                    [&]( SCH_ITEM* child )
                    {
                        child->ClearFlags( EDA_ITEM_ALL_FLAGS - selected_mask );
                    },
                    RECURSE_MODE::NO_RECURSE );
        }
    }

    if( schematic )
    {
        if( bulkAddedItems.size() > 0 )
            schematic->OnItemsAdded( bulkAddedItems );

        if( bulkRemovedItems.size() > 0 )
            schematic->OnItemsRemoved( bulkRemovedItems );

        if( itemsChanged.size() > 0 )
            schematic->OnItemsChanged( itemsChanged );

        if( refreshHierarchy )
        {
            schematic->RefreshHierarchy();

            if( listener )
                listener->HierarchyChanged();
        }
    }

    result.connectivityCleanUp = connectivityCleanUp;

    if( listener )
        listener->Pushed( *this, undoList, aCommitFlags, result );
}


void SCH_COMMIT::Push( const wxString& aMessage, int aCommitFlags )
{
    if( Empty() )
        return;

    if( m_isLibEditor )
    {
        // Symbol editor just saves copies of the whole symbol, so grab the first and discard
        // the rest
        KI_ASSERT( m_listener );

        m_listener->SymbolPushed( dynamic_cast<LIB_SYMBOL*>( m_entries.front().m_item ),
                                  dynamic_cast<LIB_SYMBOL*>( m_entries.front().m_copy ),
                                  aMessage, aCommitFlags );
    }
    else
    {
        pushSchEdit( aMessage, aCommitFlags );
    }

    if( m_listener )
        m_listener->Committed( aCommitFlags );

    clear();
}


EDA_ITEM* SCH_COMMIT::undoLevelItem( EDA_ITEM* aItem ) const
{
    EDA_ITEM* parent = aItem->GetParent();

    if( m_isLibEditor )
        return m_listener->CurrentSymbol();

    if( parent && parent->IsType( { SCH_SYMBOL_T, SCH_TABLE_T, SCH_SHEET_T, SCH_LABEL_LOCATE_ANY_T } ) )
        return parent;

    return aItem;
}


EDA_ITEM* SCH_COMMIT::makeImage( EDA_ITEM* aItem ) const
{
    if( m_isLibEditor )
    {
        LIB_SYMBOL*       symbol = m_listener->CurrentSymbol();
        std::vector<KIID> selected;

        // Cloning will clear the selected flags, but we want to keep them.
        for( const SCH_ITEM& item : symbol->GetDrawItems() )
        {
            if( item.IsSelected() )
                selected.push_back( item.m_Uuid );
        }

        symbol = new LIB_SYMBOL( *symbol );

        // Restore selected flags.
        for( SCH_ITEM& item : symbol->GetDrawItems() )
        {
            if( alg::contains( selected, item.m_Uuid ) )
                item.SetSelected();
        }

        return symbol;
    }

    return aItem->Clone();
}


void SCH_COMMIT::Revert()
{
    SCH_COMMIT_LISTENER* listener = m_listener.get();
    SCH_SHEET_LIST       sheets;

    if( m_entries.empty() )
        return;

    if( m_isLibEditor )
    {
        // Symbol editor just saves copies of the whole symbol, so grab the first and discard
        // the rest
        KI_ASSERT( listener );
        listener->SymbolReverted( dynamic_cast<LIB_SYMBOL*>( m_entries.front().m_copy ) );
        clear();
        return;
    }

    SCHEMATIC*             schematic = nullptr;
    std::vector<SCH_ITEM*> bulkAddedItems;
    std::vector<SCH_ITEM*> bulkRemovedItems;
    std::vector<SCH_ITEM*> itemsChanged;

    for( COMMIT_LINE& ent : m_entries )
    {
        int         changeType = ent.m_type & CHT_TYPE;
        int         changeFlags = ent.m_type & CHT_FLAGS;
        SCH_ITEM*   item = dynamic_cast<SCH_ITEM*>( ent.m_item );
        SCH_ITEM*   copy = dynamic_cast<SCH_ITEM*>( ent.m_copy );
        SCH_SCREEN* screen = dynamic_cast<SCH_SCREEN*>( ent.m_screen );

        KI_CHECK2( item && screen, continue );

        if( !schematic )
            schematic = item->Schematic();

        switch( changeType )
        {
        case CHT_ADD:
            if( !( changeFlags & CHT_DONE ) )
                break;

            if( listener )
                listener->ItemRemoved( item );

            screen->Remove( item );
            bulkRemovedItems.push_back( item );
            break;

        case CHT_REMOVE:
            item->SetConnectivityDirty();

            if( !( changeFlags & CHT_DONE ) )
                break;

            if( listener )
                listener->ItemAdded( item );

            screen->Append( item );
            bulkAddedItems.push_back( item );
            break;

        case CHT_MODIFY:
        {
            KI_CHECK2( copy, break );

            if( listener )
                listener->ItemRemoved( item );

            bool unselect = !item->IsSelected();

            item->SwapItemData( copy );

            if( unselect )
            {
                item->ClearSelected();
                item->RunOnChildren( []( SCH_ITEM* aChild )
                                     {
                                         aChild->ClearSelected();
                                     },
                                     RECURSE_MODE::NO_RECURSE );
            }

            // Special cases for items which have instance data
            if( item->GetParent() && item->GetParent()->Type() == SCH_SYMBOL_T && item->Type() == SCH_FIELD_T )
            {
                SCH_FIELD*  field = static_cast<SCH_FIELD*>( item );
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item->GetParent() );

                if( field->GetId() == FIELD_T::REFERENCE )
                {
                    // Lazy eval of sheet list; this is expensive even when unsorted
                    if( sheets.empty() )
                        sheets = schematic->Hierarchy();

                    SCH_SHEET_PATH sheet = sheets.FindSheetForScreen( screen );
                    symbol->SetRef( &sheet, field->GetText() );
                }
            }

            // This must be called before any calls that require stable object pointers.
            screen->Update( item );

            // This hack is to prevent incorrectly parented symbol pins from breaking the
            // connectivity algorithm.
            if( item->Type() == SCH_SYMBOL_T )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
                symbol->UpdatePins();

                CONNECTION_GRAPH* graph = schematic->ConnectionGraph();

                SCH_SYMBOL* symbolCopy = static_cast<SCH_SYMBOL*>( copy );
                graph->RemoveItem( symbolCopy );

                for( SCH_PIN* pin : symbolCopy->GetPins() )
                    graph->RemoveItem( pin );
            }

            item->SetConnectivityDirty();

            if( listener )
                listener->ItemAdded( item );

            delete copy;
            break;
        }

        default:
            KI_ASSERT( false );
            break;
        }
    }

    if( schematic )
    {
        if( bulkAddedItems.size() > 0 )
            schematic->OnItemsAdded( bulkAddedItems );

        if( bulkRemovedItems.size() > 0 )
            schematic->OnItemsRemoved( bulkRemovedItems );

        if( itemsChanged.size() > 0 )
            schematic->OnItemsChanged( itemsChanged );
    }

    if( listener )
        listener->Reverted();

    clear();
}

