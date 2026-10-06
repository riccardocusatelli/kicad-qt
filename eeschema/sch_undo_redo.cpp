/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
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

/* Undo and redo of the changes of a schematic.
 *
 * An entry (PICKED_ITEMS_LIST) holds pickers (ITEM_PICKER), one for each item concerned by
 * the command, with its screen.  There are 3 cases:
 *  - deleted items: the picker holds the item, which is out of its screen;
 *  - changed items: the picker holds the item and a copy of it (the link) with the old data;
 *  - new items: the picker holds the item.
 *
 * Undo puts the deleted items back on their screen, exchanges the data of the changed items
 * with their copies and removes the new items; the entry becomes the one that redoes the
 * command.
 *
 * The data of a hierarchical sheet (sub-hierarchy, undo/redo list) is deleted only when the
 * sheet is really deleted (i.e. when deleted from undo or redo list), by its destructor.
 */

#include <sch_undo_redo.h>

#include <limits>

#include <core/ki_assert.h>
#include <drawing_sheet/ds_proxy_undo_item.h>
#include <sch_field.h>
#include <sch_group.h>
#include <sch_pin.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <sch_symbol.h>


SCH_UNDO_REDO::RESULT SCH_UNDO_REDO::Apply( SCHEMATIC& aSchematic, PICKED_ITEMS_LIST& aEntry,
                                            LISTENER& aListener )
{
    RESULT                 result;
    std::vector<SCH_ITEM*> bulkAddedItems;
    std::vector<SCH_ITEM*> bulkRemovedItems;
    std::vector<SCH_ITEM*> bulkChangedItems;
    bool                   refreshHierarchy = false;
    SCH_SHEET_LIST         sheets = aSchematic.Hierarchy();
    bool                   clearedRepeatItems = false;

    // Undo in the reverse order of list creation: (this can allow stacked changes like the
    // same item can be changed and deleted in the same complex command).
    // After hitting 0, subtracting 1 will roll the value over to its max representation
    for( unsigned ii = aEntry.GetCount() - 1; ii < std::numeric_limits<unsigned>::max(); ii-- )
    {
        UNDO_REDO      status = aEntry.GetPickedItemStatus( ii );
        EDA_ITEM*      eda_item = aEntry.GetPickedItem( ii );
        SCH_SCREEN*    screen = dynamic_cast<SCH_SCREEN*>( aEntry.GetScreenForItem( ii ) );
        SCH_SHEET_PATH undoSheet = sheets.FindSheetForScreen( screen );

        eda_item->SetFlags( aEntry.GetPickerFlags( ii ) );
        eda_item->ClearEditFlags();
        eda_item->ClearTempFlags();

        // Set connectable object connectivity status.
        auto propagateConnectivityDamage =
                [&]( SCH_ITEM* schItem )
                {
                    if( schItem->IsConnectable() )
                    {
                        schItem->SetConnectivityDirty();

                        if( schItem->Type() == SCH_SYMBOL_T )
                        {
                            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( schItem );

                            for( SCH_PIN* pin : symbol->GetPins( &undoSheet ) )
                                pin->SetConnectivityDirty();
                        }
                        else if( schItem->Type() == SCH_SHEET_T )
                        {
                            SCH_SHEET* sheet = static_cast<SCH_SHEET*>( schItem );

                            for( SCH_SHEET_PIN* pin : sheet->GetPins() )
                                pin->SetConnectivityDirty();
                        }

                        result.connectablesChanged = true;
                        result.connectivityChanged = true;

                        // Do a local clean up if there are any connectable objects in the commit
                        if( result.connectivityCleanUp == NO_CLEANUP )
                            result.connectivityCleanUp = LOCAL_CLEANUP;

                        // Do a full rebauild of the connectivity if there is a sheet in the commit
                        if( schItem->Type() == SCH_SHEET_T )
                            result.connectivityCleanUp = GLOBAL_CLEANUP;
                    }
                    else if( schItem->Type() == SCH_RULE_AREA_T )
                    {
                        result.connectivityChanged = true;
                    }
                };

        SCH_ITEM* schItem = dynamic_cast<SCH_ITEM*>( eda_item );

        if( status == UNDO_REDO::NEWITEM )
        {
            if( schItem )
                propagateConnectivityDamage( schItem );

            // If we are removing the current sheet, get out first
            if( eda_item->Type() == SCH_SHEET_T )
            {
                result.hierarchyChanged = true;
                refreshHierarchy = true;

                aListener.SheetRemoving( static_cast<SCH_SHEET*>( eda_item ) );
            }

            aListener.RemoveFromScreen( eda_item, screen );
            aEntry.SetPickedItemStatus( UNDO_REDO::DELETED, ii );

            bulkRemovedItems.emplace_back( schItem );
        }
        else if( status == UNDO_REDO::DELETED )
        {
            if( eda_item->Type() == SCH_SHEET_T )
            {
                result.hierarchyChanged = true;
                refreshHierarchy = true;
            }

            if( schItem )
                propagateConnectivityDamage( schItem );

            // deleted items are re-inserted on undo
            aListener.AddToScreen( eda_item, screen );
            aEntry.SetPickedItemStatus( UNDO_REDO::NEWITEM, ii );

            bulkAddedItems.emplace_back( schItem );
        }
        else if( status == UNDO_REDO::PAGESETTINGS )
        {
            aListener.SwapPageSettings( static_cast<DS_PROXY_UNDO_ITEM*>( eda_item ), undoSheet );
        }
        else if( status == UNDO_REDO::REPEAT_ITEM )
        {
            if( !clearedRepeatItems )
            {
                aListener.ClearRepeatItems();
                clearedRepeatItems = true;
            }

            if( schItem )
            {
                propagateConnectivityDamage( schItem );
                aListener.AddRepeatItem( schItem );

                if( schItem->Type() == SCH_SHEET_T )
                {
                    result.hierarchyChanged = true;
                    refreshHierarchy = true;
                }
            }
        }
        else if( schItem )
        {
            SCH_ITEM* itemCopy = dynamic_cast<SCH_ITEM*>( aEntry.GetPickedItemLink( ii ) );

            KI_CHECK2( itemCopy, continue );

            if( schItem->HasConnectivityChanges( itemCopy, &aListener.CurrentSheet() ) )
                propagateConnectivityDamage( schItem );

            // The root sheet is a pseudo object that owns the root screen object but is not on
            // the root screen so do not attempt to remove it from the screen it owns.
            if( schItem != &aSchematic.Root() )
                aListener.RemoveFromScreen( schItem, screen );

            switch( status )
            {
            case UNDO_REDO::CHANGED:
                if( schItem->Type() == SCH_SHEET_T )
                {
                    const SCH_SHEET* origSheet = static_cast<const SCH_SHEET*>( schItem );
                    const SCH_SHEET* copySheet = static_cast<const SCH_SHEET*>( itemCopy );

                    KI_CHECK2( origSheet && copySheet, continue );

                    if( origSheet->GetName() != copySheet->GetName()
                            || origSheet->GetFileName() != copySheet->GetFileName()
                            || origSheet->HasPageNumberChanges( *copySheet ) )
                    {
                        result.hierarchyChanged = true;
                    }

                    // Sheet name changes do not require rebuilding the hiearchy.
                    if( origSheet->GetFileName() != copySheet->GetFileName()
                            || origSheet->HasPageNumberChanges( *copySheet ) )
                    {
                        refreshHierarchy = true;
                    }

                    result.variantsChanged = true;
                }

                if( schItem->Type() == SCH_SYMBOL_T )
                    result.variantsChanged = true;

                schItem->SwapItemData( itemCopy );

                bulkChangedItems.emplace_back( schItem );

                // Special cases for items which have instance data
                if( schItem->GetParent() && schItem->GetParent()->Type() == SCH_SYMBOL_T
                  && schItem->Type() == SCH_FIELD_T )
                {
                    SCH_FIELD*  field = static_cast<SCH_FIELD*>( schItem );
                    SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( schItem->GetParent() );

                    if( field->GetId() == FIELD_T::REFERENCE )
                    {
                        // Lazy eval of sheet list; this is expensive even when unsorted
                        if( sheets.empty() )
                            sheets = aSchematic.Hierarchy();

                        SCH_SHEET_PATH sheet = sheets.FindSheetForScreen( screen );
                        symbol->SetRef( &sheet, field->GetText() );
                    }

                    bulkChangedItems.emplace_back( symbol );
                }

                break;

            default:
                KI_FAIL_MSG( wxString::Format( wxT( "Unknown undo/redo command %d" ), status ) );
                break;
            }

            if( schItem->Type() == SCH_SYMBOL_T )
            {
                SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( schItem );
                sym->UpdatePins();
            }

            if( schItem != &aSchematic.Root() )
                aListener.AddToScreen( schItem, screen );
        }
    }

    // We have now swapped all the group parent and group member pointers.  But it is a
    // risky proposition to bet on the pointers being invariant, so validate them all.
    //
    // Two passes: restore each item's parent group first, then rebuild each group's members. The
    // rebuild comes last so a member keeps its group even if its item was restored before it.
    for( int ii = 0; ii < (int) aEntry.GetCount(); ++ii )
    {
        ITEM_PICKER& wrapper = aEntry.GetItemWrapper( ii );

        if( wrapper.GetStatus() == UNDO_REDO::DELETED )
            continue;

        SCH_ITEM* parentGroup = aSchematic.ResolveItem( wrapper.GetGroupId(), nullptr, true );
        wrapper.GetItem()->SetParentGroup( dynamic_cast<SCH_GROUP*>( parentGroup ) );
    }

    for( int ii = 0; ii < (int) aEntry.GetCount(); ++ii )
    {
        ITEM_PICKER& wrapper = aEntry.GetItemWrapper( ii );

        if( wrapper.GetStatus() == UNDO_REDO::DELETED )
            continue;

        if( EDA_GROUP* group = dynamic_cast<SCH_GROUP*>( wrapper.GetItem() ) )
        {
            // Items list may contain dodgy pointers, so don't use RemoveAll().  AddItem() also
            // re-links each member back to the group.
            group->GetItems().clear();

            for( const KIID& member : wrapper.GetGroupMembers() )
            {
                if( SCH_ITEM* memberItem = aSchematic.ResolveItem( member, nullptr, true ) )
                    group->AddItem( memberItem );
            }
        }

        // And prepare for a redo by updating group info based on current image
        if( EDA_ITEM* item = wrapper.GetLink() )
            wrapper.SetLink( item );
    }

    // Notify the listeners of the schematic
    if( bulkAddedItems.size() > 0 )
        aSchematic.OnItemsAdded( bulkAddedItems );

    if( bulkRemovedItems.size() > 0 )
        aSchematic.OnItemsRemoved( bulkRemovedItems );

    if( bulkChangedItems.size() > 0 )
        aSchematic.OnItemsChanged( bulkChangedItems );

    if( refreshHierarchy )
        aSchematic.RefreshHierarchy();

    return result;
}


void SCH_UNDO_REDO::DeleteItems( PICKED_ITEMS_LIST& aEntry )
{
    aEntry.ClearListAndDeleteItems(
            []( EDA_ITEM* aItem )
            {
                delete aItem;
            } );
}
