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

#include <sch_actions.h>
#include <sch_edit_frame.h>
#include <tool/tool_manager.h>
#include <schematic.h>
#include <sch_bus_entry.h>
#include <sch_commit.h>
#include <sch_undo_redo.h>
#include <sch_group.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_sheet_pin.h>
#include <sch_table.h>
#include <tools/sch_selection_tool.h>
#include <drawing_sheet/ds_proxy_undo_item.h>
#include <tool/actions.h>
#include <wx/log.h>
#include <core/ki_log.h>
#include <core/ki_assert.h>


/* Functions to undo and redo edit commands.
 *
 *  m_UndoList and m_RedoList handle a std::vector of PICKED_ITEMS_LIST.  Each PICKED_ITEMS_LIST handles
 *  a std::vector of ITEM_PICKER that store the list of schematic items that are concerned by the command
 *  to undo or redo and is created for each command to undo/redo).  Each picker has a pointer pointing to
 *  an item to undo or redo (in fact: deleted, added or modified), and has a pointer to a copy of this
 *  item, when this item has been modified (the old values of parameters are therefore saved).
 *
 *  there are 3 cases:
 *  - delete item(s) command
 *  - change item(s) command
 *  - add item(s) command
 *
 *  Undo command
 *  - delete item(s) command:
 *       =>  deleted items are moved in undo list
 *
 *  - change item(s) command
 *      => A copy of item(s) is made (a DrawPickedStruct list of wrappers)
 *      the .m_Link member of each wrapper points the modified item.
 *      the .m_Item member of each wrapper points the old copy of this item.
 *
 *  - add item(s) command
 *      =>A list of item(s) is made. The .m_Item member of each wrapper points
 *        the new item.
 *
 *  Redo command
 *  - delete item(s) old command:
 *      => deleted items are moved into m_tree
 *
 *  - change item(s) command
 *      => the copy of item(s) is moved in Undo list
 *
 *  - add item(s) command
 *      => The list of item(s) is used to create a deleted list in undo
 *         list(same as a delete command)
 *
 *  A problem is the hierarchical sheet handling.  The data associated (sub-hierarchy, undo/redo list) is
 *  deleted only when the sheet is really deleted (i.e. when deleted from undo or redo list).  This is
 *  handled by its destructor.
 */

void SCH_EDIT_FRAME::SaveCopyInUndoList( SCH_SCREEN* aScreen, SCH_ITEM* aItem, UNDO_REDO aCommandType,
                                         bool aAppend )
{
    PICKED_ITEMS_LIST* commandToUndo = nullptr;

    KI_CHECK( aItem, /* void */ );

    PICKED_ITEMS_LIST* lastUndo = PopCommandFromUndoList();

    // If the last stack was empty, use that one instead of creating a new stack
    if( lastUndo )
    {
        if( aAppend || !lastUndo->GetCount() )
            commandToUndo = lastUndo;
        else
            PushCommandToUndoList( lastUndo );
    }

    if( !commandToUndo )
    {
        commandToUndo = new PICKED_ITEMS_LIST();
    }

    ITEM_PICKER itemWrapper( aScreen, aItem, aCommandType );
    itemWrapper.SetFlags( aItem->GetFlags() );

    switch( aCommandType )
    {
    case UNDO_REDO::CHANGED: /* Create a copy of item */
        itemWrapper.SetLink( aItem->Duplicate( IGNORE_PARENT_GROUP, nullptr, true ) );
        commandToUndo->PushItem( itemWrapper );
        break;

    case UNDO_REDO::NEWITEM:
    case UNDO_REDO::DELETED:
        commandToUndo->PushItem( itemWrapper );
        break;

    default:
        KI_FAIL_MSG( wxString::Format( wxT( "SaveCopyInUndoList() error (unknown code %X)" ), aCommandType ) );
        break;
    }

    if( commandToUndo->GetCount() )
    {
        /* Save the copy in undo list */
        PushCommandToUndoList( commandToUndo );

        /* Clear redo list, because after new save there is no redo to do */
        ClearUndoORRedoList( REDO_LIST );
    }
    else
    {
        delete commandToUndo;
    }
}


void SCH_EDIT_FRAME::SaveCopyInUndoList( const PICKED_ITEMS_LIST& aItemsList, UNDO_REDO aTypeCommand,
                                         bool aAppend )
{
    PICKED_ITEMS_LIST* commandToUndo = nullptr;

    if( !aItemsList.GetCount() )
        return;

    PICKED_ITEMS_LIST* lastUndo = PopCommandFromUndoList();

    // If the last stack was empty, use that one instead of creating a new stack
    if( lastUndo )
    {
        if( aAppend || !lastUndo->GetCount() )
            commandToUndo = lastUndo;
        else
            PushCommandToUndoList( lastUndo );
    }

    if( !commandToUndo )
    {
        commandToUndo = new PICKED_ITEMS_LIST();
        commandToUndo->SetDescription( aItemsList.GetDescription() );
    }

    // Copy picker list:
    if( !commandToUndo->GetCount() )
    {
        commandToUndo->CopyList( aItemsList );

        for( const std::unique_ptr<SCH_ITEM>& item : GetRepeatItems() )
        {
            EDA_ITEM* repeatItemClone = item->Clone();
            repeatItemClone->SetFlags( UR_TRANSIENT );

            ITEM_PICKER repeatItemPicker( nullptr, repeatItemClone, UNDO_REDO::REPEAT_ITEM );
            commandToUndo->PushItem( repeatItemPicker );
        }
    }
    else
    {
        // Unless we are appending, in which case, get the picker items
        for( unsigned ii = 0; ii < aItemsList.GetCount(); ii++ )
            commandToUndo->PushItem( aItemsList.GetItemWrapper( ii) );
    }

    // Verify list, and creates data if needed
    for( unsigned ii = 0; ii < commandToUndo->GetCount(); ii++ )
    {
        SCH_ITEM* sch_item = dynamic_cast<SCH_ITEM*>( commandToUndo->GetPickedItem( ii ) );

        // Common items implemented in EDA_DRAW_FRAME will not be SCH_ITEMs.
        if( !sch_item )
            continue;

        UNDO_REDO command = commandToUndo->GetPickedItemStatus( ii );

        if( command == UNDO_REDO::UNSPECIFIED )
        {
            command = aTypeCommand;
            commandToUndo->SetPickedItemStatus( command, ii );
        }

        switch( command )
        {
        case UNDO_REDO::CHANGED:

            /* If needed, create a copy of item, and put in undo list
             * in the picker, as link
             * If this link is not null, the copy is already done
             */
            if( commandToUndo->GetPickedItemLink( ii ) == nullptr )
                commandToUndo->SetPickedItemLink( sch_item->Duplicate( IGNORE_PARENT_GROUP, nullptr, true ), ii );

            KI_ASSERT( commandToUndo->GetPickedItemLink( ii ) );
            break;

        case UNDO_REDO::NEWITEM:
        case UNDO_REDO::DELETED:
        case UNDO_REDO::PAGESETTINGS:
        case UNDO_REDO::REPEAT_ITEM:
            break;

        default:
            KI_FAIL_MSG( wxString::Format( wxT( "Unknown undo/redo command %d" ), command ) );
            break;
        }
    }

    if( commandToUndo->GetCount() )
    {
        /* Save the copy in undo list */
        PushCommandToUndoList( commandToUndo );

        /* Clear redo list, because after new save there is no redo to do */
        ClearUndoORRedoList( REDO_LIST );
    }
    else    // Should not occur
    {
        delete commandToUndo;
    }
}


namespace
{

/// The schematic editor, for SCH_UNDO_REDO::Apply().
class FRAME_UNDO_REDO_LISTENER : public SCH_UNDO_REDO::LISTENER
{
public:
    explicit FRAME_UNDO_REDO_LISTENER( SCH_EDIT_FRAME* aFrame ) :
            m_frame( aFrame )
    {
    }

    const SCH_SHEET_PATH& CurrentSheet() const override { return m_frame->GetCurrentSheet(); }

    void AddToScreen( EDA_ITEM* aItem, SCH_SCREEN* aScreen ) override
    {
        m_frame->AddToScreen( aItem, aScreen );
    }

    void RemoveFromScreen( EDA_ITEM* aItem, SCH_SCREEN* aScreen ) override
    {
        m_frame->RemoveFromScreen( aItem, aScreen );
    }

    void SheetRemoving( SCH_SHEET* aSheet ) override
    {
        // If we are removing the current sheet, get out first
        if( aSheet->GetScreen() == m_frame->GetScreen() )
            m_frame->GetToolManager()->PostAction( SCH_ACTIONS::leaveSheet );
    }

    void SwapPageSettings( DS_PROXY_UNDO_ITEM* aItem, const SCH_SHEET_PATH& aSheet ) override
    {
        if( m_frame->GetCurrentSheet() != aSheet )
        {
            m_frame->SetCurrentSheet( aSheet );
            m_frame->DisplayCurrentSheet();
        }

        // swap current settings with stored settings
        DS_PROXY_UNDO_ITEM alt_item( m_frame );
        aItem->Restore( m_frame );
        *aItem = std::move( alt_item );
    }

    void ClearRepeatItems() override { m_frame->ClearRepeatItemsList(); }

    void AddRepeatItem( SCH_ITEM* aItem ) override { m_frame->AddCopyForRepeatItem( aItem ); }

private:
    SCH_EDIT_FRAME* m_frame;
};

} // namespace


void SCH_EDIT_FRAME::PutDataInPreviousState( PICKED_ITEMS_LIST* aList )
{
    FRAME_UNDO_REDO_LISTENER listener( this );

    SCH_UNDO_REDO::RESULT result = SCH_UNDO_REDO::Apply( Schematic(), *aList, listener );

    GetCanvas()->GetView()->ClearHiddenFlags();

    if( result.connectablesChanged )
        m_highlightedConnChanged = true;

    if( result.connectivityChanged )
    {
        KI_LOG_TRACE( wxS( "CONN_PROFILE" ), wxS( "Undo/redo %s clean up connectivity rebuild." ),
                    result.connectivityCleanUp == LOCAL_CLEANUP ? wxS( "local" ) : wxS( "global" ) );

        SCH_COMMIT localCommit( m_toolManager );

        RecalculateConnections( &localCommit, result.connectivityCleanUp );

        if( result.connectivityCleanUp == GLOBAL_CLEANUP )
            SetSheetNumberAndCount();

        // Restore hop over shapes of wires, if any
        if( m_schematic->Settings().GetHopOverScale() > 0.0 )
        {
            for( SCH_ITEM* item : GetScreen()->Items() )
            {
                if( item->Type() != SCH_LINE_T )
                    continue;

                SCH_LINE* line = static_cast<SCH_LINE*>( item );

                if( line->IsWire() || line->IsBus() )
                    UpdateHopOveredWires( line );
            }
        }
    }

    // Update the hierarchy navigator when there are sheet changes.
    if( result.hierarchyChanged )
        UpdateHierarchyNavigator();

    if( result.variantsChanged && m_schematic )
    {
        m_schematic->LoadVariants();
        UpdateVariantSelectionCtrl( m_schematic->GetVariantNamesForUI() );
    }
}


void SCH_EDIT_FRAME::RollbackSchematicFromUndo()
{
    PICKED_ITEMS_LIST* undo = PopCommandFromUndoList();

    // Skip empty frames
    while( undo && !undo->GetCount() )
    {
        delete undo;
        undo = PopCommandFromUndoList();
    }

    if( undo )
    {
        PutDataInPreviousState( undo );
        SCH_UNDO_REDO::DeleteItems( *undo );

        delete undo;

        m_toolManager->GetTool<SCH_SELECTION_TOOL>()->RebuildSelection();
    }

    GetCanvas()->Refresh();
}


void SCH_EDIT_FRAME::ClearUndoORRedoList( UNDO_REDO_LIST whichList, int aItemCount )
{
    if( aItemCount == 0 )
        return;

    UNDO_REDO_CONTAINER& list = ( whichList == UNDO_LIST ) ? m_undoList : m_redoList;

    if( aItemCount < 0 )
    {
        list.ClearCommandList();
    }
    else
    {
        for( int ii = 0; ii < aItemCount; ii++ )
        {
            if( list.m_CommandsList.size() == 0 )
                break;

            PICKED_ITEMS_LIST* curr_cmd = list.m_CommandsList[0];
            list.m_CommandsList.erase( list.m_CommandsList.begin() );

            SCH_UNDO_REDO::DeleteItems( *curr_cmd );
            delete curr_cmd;    // Delete command
        }
    }
}


