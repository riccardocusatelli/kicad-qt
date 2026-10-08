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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
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


void PCB_BASE_EDIT_FRAME::RestoreCopyFromUndoList()
{
    if( UndoRedoBlocked() )
        return;

    if( GetUndoCommandCount() <= 0 )
        return;

    // Inform tools that undo command was issued
    m_toolManager->ProcessEvent( { TC_MESSAGE, TA_UNDO_REDO_PRE, AS_GLOBAL } );

    // Get the old list
    PICKED_ITEMS_LIST* list = PopCommandFromUndoList();

    // Undo the command
    bool shapesChanged = PutDataInPreviousState( list );

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

    // Redo the command
    bool shapesChanged = PutDataInPreviousState( list );

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
    }

    GetCanvas()->Refresh();
}


bool PCB_BASE_EDIT_FRAME::PutDataInPreviousState( PICKED_ITEMS_LIST* aList, bool aRehatchShapes )
{
    FRAME_UNDO_REDO_LISTENER listener( this );
    const bool               boardEditor = IsType( FRAME_PCB_EDITOR );

    BOARD_UNDO_REDO::RESULT result = BOARD_UNDO_REDO::Apply( *GetBoard(), *aList, listener,
                                                             boardEditor );

    if( result.itemsNotFound )
        wxMessageBox( _( "Incomplete undo/redo operation: some items not found" ) );

    if( boardEditor )
    {
        if( !result.dirtyRuleAreas.empty()
                && (   GetPcbNewSettings()->m_Display.m_TrackClearance == SHOW_WITH_VIA_ALWAYS
                    || GetPcbNewSettings()->m_Display.m_PadClearance ) )
        {
            GetCanvas()->GetView()->UpdateCollidingItems( result.dirtyRuleAreas,
                                                          { PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T,
                                                            PCB_PAD_T } );
        }

        if( result.connectivityChanged )
            Compile_Ratsnest( false );

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
    GetCanvas()->Refresh();
}
