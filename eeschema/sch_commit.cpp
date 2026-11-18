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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <macros.h>
#include <core/kicad_algo.h>

#include <lib_symbol.h>
#include <advanced_config.h>

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
#include <memory>
#include <set>
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

    if( !m_isLibEditor )
    {
        SCH_ITEM*   item = dynamic_cast<SCH_ITEM*>( aItem );
        SCH_SCREEN* screen = dynamic_cast<SCH_SCREEN*>( aScreen );

        if( !screen && item )
            screen = item->GetParentScreen();

        if( screen && item && SCH_SCREEN::IsConnectivitySource( item ) )
            screen->BumpConnectivityRevision( item->Type() );
    }

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


void SCH_COMMIT::RemovedForCleanup( SCH_ITEM* aItem, SCH_SCREEN* aScreen )
{
    KI_ASSERT( !aScreen->CheckIfOnDrawList( aItem ) );

    if( COMMIT_LINE* entry = findEntry( aItem, aScreen ); entry && ( entry->m_type & CHT_TYPE ) == CHT_MODIFY )
    {
        // Staging REMOVE discards MODIFY, so preserve the original geometry on the removed item
        // before its image is discarded. Cleanup must still skip it until the commit is processed.
        aItem->SwapItemData( static_cast<SCH_ITEM*>( entry->m_copy ) );
        aItem->SetFlags( STRUCT_DELETED );
    }

    Removed( aItem, aScreen );
}


void SCH_COMMIT::pushSchEdit( const wxString& aMessage, int aCommitFlags )
{
    // Objects potentially interested in changes:
    SCH_COMMIT_LISTENER* listener = m_listener.get();
    PICKED_ITEMS_LIST   undoList;
    SCH_SCREEN*         currentScreen = listener ? listener->CurrentScreen() : nullptr;
    SCH_GROUP*          enteredGroup = listener ? listener->EnteredGroup() : nullptr;
    bool                itemsDeselected = false;
    bool                selectedModified = false;
    bool                dirtyConnectivity = false;
    bool                refreshConnectivity = false;
    bool                refreshHierarchy = false;
    bool                refSuffixChanged = false;
    SCH_CLEANUP_FLAGS   connectivityCleanUp = NO_CLEANUP;

    if( Empty() )
    {
        return;
    }

    // Without an editor there is no undo list to take the changes
    if( !listener || !listener->HasEditor() )
        aCommitFlags |= SKIP_UNDO;

    undoList.SetDescription( aMessage );

    SCHEMATIC*                             schematic = nullptr;
    std::vector<SCH_ITEM*>                 bulkAddedItems;
    std::vector<SCH_ITEM*>                 bulkRemovedItems;
    std::vector<SCH_ITEM*>                 itemsChanged;
    std::vector<std::unique_ptr<SCH_ITEM>> cleanupRemovedItems;
    std::set<SCH_SCREEN*>                  connectivityScreens;

    auto notifyModel =
            [&]()
            {
                if( !schematic )
                    return;

                if( !bulkAddedItems.empty() )
                    schematic->OnItemsAdded( bulkAddedItems );

                if( !bulkRemovedItems.empty() )
                    schematic->OnItemsRemoved( bulkRemovedItems );

                if( !itemsChanged.empty() )
                    schematic->OnItemsChanged( itemsChanged );

                if( refreshHierarchy )
                    schematic->RefreshHierarchy();

                bulkAddedItems.clear();
                bulkRemovedItems.clear();
                itemsChanged.clear();
            };

    auto updateConnectivityFlag =
            [&]( SCH_ITEM* schItem, SCH_SCREEN* screen, bool fullSheetUpdate = true )
            {
                if( schItem->IsConnectable() || ( schItem->Type() == SCH_RULE_AREA_T ) )
                {
                    dirtyConnectivity = true;
                    connectivityScreens.insert( screen );

                    // Do a local clean up if there are any connectable objects in the commit.
                    if( connectivityCleanUp == NO_CLEANUP )
                        connectivityCleanUp = LOCAL_CLEANUP;

                    if( schItem->Type() == SCH_SHEET_T && fullSheetUpdate )
                        connectivityCleanUp = GLOBAL_CLEANUP;
                }
            };

    // We don't know that anything will be added to the entered group, but it does no harm to
    // add it to the commit anyway (well, as long as we're not trying to remove it).
    if( enteredGroup && GetStatus( enteredGroup ) == 0 )
        Modify( enteredGroup, currentScreen );


    // Handle wires with Hop Over shapes (view update only; skipped headless):
    if( listener )
    {
        for( COMMIT_LINE& entry : m_entries )
        {
            SCH_ITEM* schCopyItem = dynamic_cast<SCH_ITEM*>( entry.m_copy );
            SCH_ITEM* schItem = dynamic_cast<SCH_ITEM*>( entry.m_item );

            if( schCopyItem && schCopyItem->Type() == SCH_LINE_T )
                listener->UpdateHopOveredWires( schCopyItem );

            if( schItem && schItem->Type() == SCH_LINE_T )
                listener->UpdateHopOveredWires( schItem );
        }
    }

    auto stageRemovedGroups =
            [&]()
            {
                // Modify() appends to m_entries, so collect before staging group changes.
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
            };

    stageRemovedGroups();
    bool cleanupPending = true;
    std::set<SCH_SCREEN*> touchedScreens;

    for( size_t index = 0; ; )
    {
        if( index == m_entries.size() )
        {
            if( !cleanupPending || !dirtyConnectivity || ( aCommitFlags & SKIP_CONNECTIVITY ) || !schematic )
                break;

            cleanupPending = false;
            notifyModel();

            // Cleanup can replace a MODIFY with REMOVE. Keep its staging separate from processed
            // entries while retaining their undo records, so both phases form one user action.
            clear();
            // Legacy cleanup stays schematic-wide; only the engine scopes it to the staged screens
            schematic->CleanUpConnections( this, connectivityCleanUp,
                                           ADVANCED_CFG::GetCfg().m_ConnectivityEngine ? connectivityScreens
                                                                                       : std::set<SCH_SCREEN*>() );

            stageRemovedGroups();
            index = 0;
            continue;
        }

        COMMIT_LINE& entry = m_entries[index++];
        int          changeType = entry.m_type & CHT_TYPE;
        int          changeFlags = entry.m_type & CHT_FLAGS;
        SCH_ITEM*    schItem = dynamic_cast<SCH_ITEM*>( entry.m_item );
        SCH_SCREEN*  screen = dynamic_cast<SCH_SCREEN*>( entry.m_screen );

        KI_CHECK2( schItem, continue );
        KI_CHECK2( screen, continue );

        touchedScreens.insert( screen );

        if( !schematic )
            schematic = schItem->Schematic();

        // Staging invalidates captured sources even when the final connectivity is unchanged
        if( ADVANCED_CFG::GetCfg().m_ConnectivityEngine && SCH_SCREEN::IsConnectivitySource( schItem ) )
            refreshConnectivity = true;

        if( schItem->IsSelected() )
        {
            selectedModified = true;
        }
        else
        {
            schItem->RunOnChildren(
                    [&selectedModified]( SCH_ITEM* aChild )
                    {
                        if( aChild->IsSelected() )
                            selectedModified = true;
                    },
                    RECURSE_MODE::NO_RECURSE );
        }

        switch( changeType )
        {
        case CHT_ADD:
        {
            if( enteredGroup && schItem->IsGroupableType() && !schItem->GetParentGroup() )
                enteredGroup->AddItem( schItem );

            updateConnectivityFlag( schItem, screen );

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
            else if( screen )
                screen->Update( schItem );

            bulkAddedItems.push_back( schItem );

            if( schItem->Type() == SCH_SHEET_T )
                refreshHierarchy = true;

            break;
        }

        case CHT_REMOVE:
        {
            updateConnectivityFlag( schItem, screen );

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

                // In an undo we're going to re-create from the item, not the image.  That seems wrong, but
                // it would be a big risky change to make it use the image, so let's just reset the selection
                // flag for now (so that on undo it comes back selected).
                schItem->SetFlags( SELECTED );

                itemsDeselected = true;
            }

            if( schItem->Type() == SCH_FIELD_T )
            {
                static_cast<SCH_FIELD*>( schItem )->SetVisible( false );
                break;
            }

            if( ( aCommitFlags & SKIP_UNDO ) && ( !cleanupPending || ( aCommitFlags & DELETE_REMOVED_ITEMS ) ) )
                cleanupRemovedItems.emplace_back( schItem );

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
            else if( screen )
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
            bool            fullSheetUpdate = true;

            if( listener )
                listener->GetCurrentSheet( currentSheet );

            if( schItem->Type() == SCH_SHEET_T )
            {
                const SCH_SHEET* modifiedSheet = static_cast<const SCH_SHEET*>( schItem );
                const SCH_SHEET* originalSheet = static_cast<const SCH_SHEET*>( itemCopy );
                const bool       hierarchyChanged = originalSheet->HasHierarchyChanges( *modifiedSheet );

                refreshHierarchy |= hierarchyChanged;

                if( hierarchyChanged && originalSheet->HasRefSuffixChanges( *modifiedSheet ) )
                {
                    static_cast<SCH_SHEET*>( schItem )->SetDescendantsModified();
                    refSuffixChanged = true;
                }

                // Stable ports are invalidated through their containing screen, including shared instances
                fullSheetUpdate = !ADVANCED_CFG::GetCfg().m_ConnectivityEngine
                                      || hierarchyChanged
                                      || originalSheet->HasPinIdentityChanges( *modifiedSheet );
            }

            if( itemCopy->HasConnectivityChanges( schItem, &currentSheet )
                || ( itemCopy->Type() == SCH_RULE_AREA_T ) )
            {
                updateConnectivityFlag( schItem, screen, fullSheetUpdate );
            }
            else if( schItem->IsConnectable() && schItem->IsConnectivityDirty() )
            {
                // Move previews can change dangling flags without changing the final geometry.
                refreshConnectivity = true;
            }

            if( schItem->Type() == SCH_SYMBOL_T )
            {
                const SCH_SYMBOL* origSymbol = static_cast<const SCH_SYMBOL*>( itemCopy );
                const SCH_SYMBOL* modSymbol = static_cast<const SCH_SYMBOL*>( schItem );

                if( origSymbol->GetPins( &currentSheet ).size() != modSymbol->GetPins( &currentSheet ).size() )
                    connectivityCleanUp = GLOBAL_CLEANUP;

                if( origSymbol->GetSchSymbolLibraryName() != modSymbol->GetSchSymbolLibraryName() )
                    screen->PruneUnusedLibSymbol( origSymbol->GetSchSymbolLibraryName() );
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

            if( listener && screen == currentScreen )
                listener->UpdateItem( schItem, screen, false );
            else if( screen )
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

    notifyModel();

    // Mark the touched screens dirty so that the state is tracked even in headless API server mode
    if( !( aCommitFlags & SKIP_SET_DIRTY ) )
    {
        for( SCH_SCREEN* screen : touchedScreens )
            screen->SetContentModified();
    }

    cleanupRemovedItems.clear();

    SCH_COMMIT_LISTENER::PUSH_RESULT result;
    result.itemsDeselected = itemsDeselected;
    result.selectedModified = selectedModified;
    result.connectivityCleanUp = connectivityCleanUp;

    if( ( dirtyConnectivity || refreshConnectivity ) && !( aCommitFlags & SKIP_CONNECTIVITY ) )
    {
        KI_LOG_TRACE( wxS( "CONN_PROFILE" ), wxS( "SCH_COMMIT::pushSchEdit() connectivity refresh, cleanup=%d." ),
                      static_cast<int>( connectivityCleanUp ) );

        // An editor refreshes its view with the connections; without one, only the model
        if( listener && listener->HasEditor() )
            result.dirtyConnectivity = true;
        else if( schematic )
            schematic->RecalculateConnections( this, connectivityCleanUp, nullptr, nullptr, nullptr, nullptr, true );
    }

    if( listener )
    {
        // The listener takes the undo entries (the changes of the clean up included) and
        // recalculates the connections
        listener->Pushed( *this, undoList, aCommitFlags, result );

        if( refreshHierarchy )
            listener->HierarchyChanged();

        // An API edit can change a suffix while one of its child sheets is on screen
        if( refSuffixChanged )
            listener->RefSuffixChanged();
    }
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
    {
        // Headless commits operate directly on the staged item
        if( LIB_SYMBOL* symbol = m_listener ? m_listener->CurrentSymbol() : nullptr )
            return symbol;

        return aItem;
    }

    if( parent && parent->IsType( { SCH_SYMBOL_T, SCH_TABLE_T, SCH_SHEET_T, SCH_LABEL_LOCATE_ANY_T } ) )
        return parent;

    return aItem;
}


EDA_ITEM* SCH_COMMIT::makeImage( EDA_ITEM* aItem ) const
{
    if( m_isLibEditor )
    {
        LIB_SYMBOL*       symbol = m_listener ? m_listener->CurrentSymbol() : nullptr;
        std::vector<KIID> selected;

        if( !symbol )
            return aItem->Clone();

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
    RevertToCheckpoint( 0 );
}


void SCH_COMMIT::RevertToCheckpoint( int aCheckpoint )
{
    SCH_COMMIT_LISTENER* listener = m_listener.get();
    SCH_SHEET_LIST       sheets;

    if( m_entries.empty() )
        return;

    if( m_isLibEditor )
    {
        // Symbol editor just saves copies of the whole symbol, so grab the first and discard
        // the rest
        LIB_SYMBOL* copy = dynamic_cast<LIB_SYMBOL*>( m_entries.front().m_copy );

        m_entries.front().m_copy = nullptr;

        if( listener )
            listener->SymbolReverted( copy );
        else
            delete copy;

        clear();
        return;
    }

    SCHEMATIC*             schematic = nullptr;
    std::vector<SCH_ITEM*> bulkAddedItems;
    std::vector<SCH_ITEM*> bulkRemovedItems;
    std::vector<SCH_ITEM*> itemsChanged;

    // Detach added groups before restoring any saved group memberships.
    for( int ii = aCheckpoint; ii < (int) m_entries.size(); ++ii )
    {
        COMMIT_LINE& entry = m_entries[ii];

        if( ( entry.m_type & CHT_TYPE ) == CHT_ADD )
        {
            if( SCH_GROUP* group = dynamic_cast<SCH_GROUP*>( entry.m_item ) )
                group->RemoveAll();
        }
    }

    for( int ii = aCheckpoint; ii < (int) m_entries.size(); ++ii )
    {
        COMMIT_LINE& entry = m_entries[ii];
        int          changeType = entry.m_type & CHT_TYPE;
        int          changeFlags = entry.m_type & CHT_FLAGS;
        SCH_ITEM*    item = dynamic_cast<SCH_ITEM*>( entry.m_item );
        SCH_ITEM*    copy = dynamic_cast<SCH_ITEM*>( entry.m_copy );
        SCH_SCREEN*  screen = dynamic_cast<SCH_SCREEN*>( entry.m_screen );

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

            item->ClearFlags( STRUCT_DELETED );

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
                SCH_SYMBOL* symbolCopy = static_cast<SCH_SYMBOL*>( copy );

                symbol->UpdatePins();

                if( symbol->GetSchSymbolLibraryName() != symbolCopy->GetSchSymbolLibraryName() )
                    screen->PruneUnusedLibSymbol( symbolCopy->GetSchSymbolLibraryName() );

                CONNECTION_GRAPH* graph = schematic->ConnectionGraph();

                graph->RemoveItem( symbolCopy );

                // Lazy eval of sheet list; this is expensive even when unsorted
                if( sheets.empty() )
                    sheets = schematic->Hierarchy();

                SCH_SHEET_PATH sheet = sheets.FindSheetForScreen( screen );

                for( SCH_PIN* pin : symbolCopy->GetPins( &sheet ) )
                    graph->RemoveItem( pin );
            }

            item->SetConnectivityDirty();

            if( listener )
                listener->ItemAdded( item );

            itemsChanged.push_back( item );
            break;
        }

        default:
            KI_ASSERT( false );
            break;
        }

        delete copy;
        entry.m_copy = nullptr;
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

    dropEntriesAfter( aCheckpoint );
}


EDA_ITEM* SCH_COMMIT::ResolveItem( KIID& aID )
{
    if( aID == niluuid )
        return nullptr;

    for( COMMIT_LINE& entry : m_entries )
    {
        if( entry.m_item && entry.m_item->IsSCH_ITEM() && entry.m_item->m_Uuid == aID )
            return entry.m_item;
    }

    return nullptr;
}
