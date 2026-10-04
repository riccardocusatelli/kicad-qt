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
 * @file sch_commit_gui.cpp
 * @brief SCH_COMMIT in an editor: the SCH_COMMIT_LISTENER that keeps the view, the selection,
 *        the undo list and the frame in step with the schematic.
 */

#include <sch_commit.h>

#include <lib_symbol.h>
#include <sch_base_frame.h>
#include <sch_edit_frame.h>
#include <sch_group.h>
#include <sch_screen.h>
#include <schematic.h>
#include <symbol_editor/symbol_edit_frame.h>
#include <tool/tool_manager.h>
#include <tools/sch_selection_tool.h>
#include <tools/sch_tool_base.h>
#include <undo_redo_container.h>
#include <view/view.h>
#include <core/ki_log.h>


namespace
{

/// The listener of the commits made in the schematic and symbol editors.
class TOOL_COMMIT_LISTENER : public SCH_COMMIT_LISTENER
{
public:
    explicit TOOL_COMMIT_LISTENER( TOOL_MANAGER* aMgr ) :
            m_toolMgr( aMgr )
    {
    }

    SCH_GROUP* EnteredGroup() const override
    {
        SCH_SELECTION_TOOL* selTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
        return selTool ? selTool->GetEnteredGroup() : nullptr;
    }

    SCH_SCREEN* CurrentScreen() const override
    {
        SCH_EDIT_FRAME* frame = editFrame();
        return frame ? frame->GetScreen() : nullptr;
    }

    bool GetCurrentSheet( SCH_SHEET_PATH& aSheet ) const override
    {
        SCH_EDIT_FRAME* frame = editFrame();

        if( !frame )
            return false;

        aSheet = frame->GetCurrentSheet();
        return true;
    }

    void UpdateHopOveredWires( SCH_ITEM* aItem ) override
    {
        if( SCH_EDIT_FRAME* frame = editFrame() )
            frame->UpdateHopOveredWires( aItem );
    }

    void ItemAdded( SCH_ITEM* aItem ) override
    {
        if( KIGFX::VIEW* view = m_toolMgr->GetView() )
            view->Add( aItem );
    }

    void ItemRemoved( SCH_ITEM* aItem ) override
    {
        if( KIGFX::VIEW* view = m_toolMgr->GetView() )
            view->Remove( aItem );
    }

    void UpdateItem( SCH_ITEM* aItem, SCH_SCREEN* aScreen, bool aAddOrDelete ) override
    {
        if( SCH_EDIT_FRAME* frame = editFrame() )
            frame->UpdateItem( aItem, aAddOrDelete, true );
        else
            aScreen->Update( aItem );
    }

    void Deselect( SCH_ITEM* aItem ) override
    {
        if( SCH_SELECTION_TOOL* selTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>() )
            selTool->RemoveItemFromSel( aItem, true /* quiet mode */ );
    }

    void HierarchyChanged() override
    {
        if( SCH_EDIT_FRAME* frame = editFrame() )
            frame->UpdateHierarchyNavigator();
    }

    void Pushed( SCH_COMMIT& aCommit, PICKED_ITEMS_LIST& aUndoList, int aCommitFlags,
                 const PUSH_RESULT& aResult ) override
    {
        SCH_EDIT_FRAME* frame = editFrame();

        if( !( aCommitFlags & SKIP_UNDO ) && frame )
        {
            if( aUndoList.GetCount() > 0 )
                frame->SaveCopyInUndoList( aUndoList, UNDO_REDO::UNSPECIFIED, false );

            if( aResult.dirtyConnectivity )
            {
                SCH_CLEANUP_FLAGS cleanUp = static_cast<SCH_CLEANUP_FLAGS>( aResult.connectivityCleanUp );

                KI_LOG_TRACE( wxS( "CONN_PROFILE" ),
                              wxS( "SCH_COMMIT::pushSchEdit() %s clean up connectivity rebuild." ),
                              cleanUp == LOCAL_CLEANUP ? wxS( "local" ) : wxS( "global" ) );
                frame->RecalculateConnections( &aCommit, cleanUp );
            }
        }

        m_toolMgr->PostEvent( { TC_MESSAGE, TA_MODEL_CHANGE, AS_GLOBAL } );

        if( aResult.itemsDeselected )
            m_toolMgr->PostEvent( EVENTS::UnselectedEvent );

        if( aResult.selectedModified )
            m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );
    }

    void Reverted() override
    {
        if( SCH_SELECTION_TOOL* selTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>() )
            selTool->RebuildSelection();

        if( SCH_EDIT_FRAME* frame = editFrame() )
            frame->RecalculateConnections( nullptr, NO_CLEANUP );
    }

    LIB_SYMBOL* CurrentSymbol() const override
    {
        SYMBOL_EDIT_FRAME* frame = symbolEditFrame();
        return frame ? frame->GetCurSymbol() : nullptr;
    }

    void SymbolPushed( LIB_SYMBOL* aSymbol, LIB_SYMBOL* aCopy, const wxString& aMessage,
                       int aCommitFlags ) override
    {
        if( aSymbol )
        {
            if( KIGFX::VIEW* view = m_toolMgr->GetView() )
            {
                view->Update( aSymbol );

                aSymbol->RunOnChildren(
                        [&]( SCH_ITEM* aChild )
                        {
                            view->Update( aChild );
                        },
                        RECURSE_MODE::NO_RECURSE );
            }

            if( SYMBOL_EDIT_FRAME* frame = symbolEditFrame() )
            {
                if( !( aCommitFlags & SKIP_UNDO ) && aCopy )
                {
                    frame->PushSymbolToUndoList( aMessage, aCopy );
                    aCopy = nullptr;   // we've transferred ownership to the undo stack
                }
            }

            // if no undo entry was needed, the copy would create a memory leak
            delete aCopy;
        }

        m_toolMgr->PostEvent( { TC_MESSAGE, TA_MODEL_CHANGE, AS_GLOBAL } );
        m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );
    }

    void SymbolReverted( LIB_SYMBOL* aCopy ) override
    {
        SYMBOL_EDIT_FRAME*  frame = symbolEditFrame();
        SCH_SELECTION_TOOL* selTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();

        if( frame && aCopy )
        {
            frame->SetCurSymbol( aCopy, false );
            m_toolMgr->ResetTools( TOOL_BASE::MODEL_RELOAD );
        }

        if( selTool )
            selTool->RebuildSelection();
    }

    void Committed( int aCommitFlags ) override
    {
        if( SCH_BASE_FRAME* frame = dynamic_cast<SCH_BASE_FRAME*>( m_toolMgr->GetToolHolder() ) )
        {
            if( !( aCommitFlags & SKIP_SET_DIRTY ) )
                frame->OnModify();

            if( frame->GetCanvas() )
                frame->GetCanvas()->Refresh();
        }
    }

private:
    SCH_EDIT_FRAME* editFrame() const
    {
        return dynamic_cast<SCH_EDIT_FRAME*>( m_toolMgr->GetToolHolder() );
    }

    SYMBOL_EDIT_FRAME* symbolEditFrame() const
    {
        return dynamic_cast<SYMBOL_EDIT_FRAME*>( m_toolMgr->GetToolHolder() );
    }

    TOOL_MANAGER* m_toolMgr;
};

} // namespace


SCH_COMMIT::SCH_COMMIT( TOOL_MANAGER* aToolMgr ) :
        COMMIT(),
        m_listener( std::make_unique<TOOL_COMMIT_LISTENER>( aToolMgr ) ),
        m_isLibEditor( false )
{
    SCH_BASE_FRAME* frame = dynamic_cast<SCH_BASE_FRAME*>( aToolMgr->GetToolHolder() );
    m_isLibEditor = frame && frame->IsType( FRAME_SCH_SYMBOL_EDITOR );
}


SCH_COMMIT::SCH_COMMIT( SCH_TOOL_BASE<SCH_BASE_FRAME>* aTool ) :
        COMMIT(),
        m_listener( std::make_unique<TOOL_COMMIT_LISTENER>( aTool->GetManager() ) ),
        m_isLibEditor( aTool->IsSymbolEditor() )
{
}


SCH_COMMIT::SCH_COMMIT( EDA_DRAW_FRAME* aFrame ) :
        COMMIT(),
        m_listener( std::make_unique<TOOL_COMMIT_LISTENER>( aFrame->GetToolManager() ) ),
        m_isLibEditor( aFrame->IsType( FRAME_SCH_SYMBOL_EDITOR ) )
{
}
