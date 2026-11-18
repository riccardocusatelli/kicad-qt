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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <commit.h>

#include <memory>

class LIB_SYMBOL;
class PICKED_ITEMS_LIST;
class SCH_COMMIT;
class SCH_GROUP;
class SCH_ITEM;
class SCH_SCREEN;
class SCH_SHEET_PATH;
class TOOL_MANAGER;
class SCH_EDIT_FRAME;
class SCH_BASE_FRAME;
class SCH_ITEM;
class SCH_SCREEN;
class EDA_DRAW_FRAME;
class TOOL_BASE;

template<class T>
class SCH_TOOL_BASE;

#define SKIP_UNDO          0x0001
#define APPEND_UNDO        0x0002
#define SKIP_SET_DIRTY     0x0004
#define SKIP_CONNECTIVITY  0x0008
// With SKIP_UNDO (implicit headless), transfer ownership of removed items to the commit.
#define DELETE_REMOVED_ITEMS 0x0010


/**
 * The editor side of a SCH_COMMIT.
 *
 * The commit applies its changes to the screens of the schematic (the data model); the
 * listener keeps the view, the selection, the undo list and the frame in step with them.  A
 * commit without a listener (kicad-cli, tests) only changes the schematic.
 *
 * sch_commit_gui.cpp implements the listener on top of a TOOL_MANAGER, for the schematic and
 * the symbol editors.
 */
class SCH_COMMIT_LISTENER
{
public:
    /// What SCH_COMMIT::Push() did, for SCH_COMMIT_LISTENER::Pushed().
    struct PUSH_RESULT
    {
        bool itemsDeselected = false;     ///< Removed items were selected
        bool selectedModified = false;    ///< Selected items were changed
        bool dirtyConnectivity = false;   ///< Connectable items were changed
        int  connectivityCleanUp = 0;     ///< SCH_CLEANUP_FLAGS for the connectivity update
    };

    virtual ~SCH_COMMIT_LISTENER() = default;

    // Schematic editor

    /// The group the user has entered (new items are added to it), if any.
    virtual SCH_GROUP* EnteredGroup() const = 0;

    /// The screen shown by the editor, if any.
    virtual SCH_SCREEN* CurrentScreen() const = 0;

    /// Get the sheet shown by the editor.  @return false if there is none.
    virtual bool GetCurrentSheet( SCH_SHEET_PATH& aSheet ) const = 0;

    /// Wires crossing @a aItem (a wire) must update their hop-over shapes.
    virtual void UpdateHopOveredWires( SCH_ITEM* aItem ) = 0;

    /// @a aItem has been added to or removed from the screen shown by the view.
    virtual void ItemAdded( SCH_ITEM* aItem ) = 0;
    virtual void ItemRemoved( SCH_ITEM* aItem ) = 0;

    /// @a aItem, on the current screen, has been added or removed (@a aAddOrDelete) or changed.
    virtual void UpdateItem( SCH_ITEM* aItem, SCH_SCREEN* aScreen, bool aAddOrDelete ) = 0;

    /// @a aItem is selected and is being removed from the schematic: drop it from the selection.
    virtual void Deselect( SCH_ITEM* aItem ) = 0;

    /// The sheet hierarchy has changed.
    virtual void HierarchyChanged() = 0;

    /// A sheet reference suffix has changed: the references shown must be composed again.
    virtual void RefSuffixChanged() {}

    /**
     * @return true if the listener is an editor, which takes the undo entries and recalculates
     *         the connections in Pushed(); false (a headless tool manager) to leave both to the
     *         commit.
     */
    virtual bool HasEditor() const { return true; }

    /**
     * The changes have been applied to the schematic.
     * @param aUndoList the undo entries (empty with SKIP_UNDO); the listener takes them.
     */
    virtual void Pushed( SCH_COMMIT& aCommit, PICKED_ITEMS_LIST& aUndoList, int aCommitFlags,
                         const PUSH_RESULT& aResult ) = 0;

    /// The commit has been reverted.
    virtual void Reverted() = 0;

    // Symbol editor

    /// The symbol being edited.
    virtual LIB_SYMBOL* CurrentSymbol() const = 0;

    /**
     * The symbol editor commit has been applied.
     * @param aSymbol the edited symbol (may be null).
     * @param aCopy the symbol before the changes; the listener takes it.
     */
    virtual void SymbolPushed( LIB_SYMBOL* aSymbol, LIB_SYMBOL* aCopy, const wxString& aMessage,
                               int aCommitFlags ) = 0;

    /// The symbol editor commit has been reverted to @a aCopy (owned by the listener).
    virtual void SymbolReverted( LIB_SYMBOL* aCopy ) = 0;

    // Both editors

    /// The commit has been applied (after Pushed() or SymbolPushed()).
    virtual void Committed( int aCommitFlags ) = 0;
};



/**
 * Commit for the schematic and symbol editors.
 *
 * Outside the symbol editor, Stage() bumps the connectivity revision of the staged item's screen,
 * and the connectivity facade stops serving that screen until the next recalculation.  A commit
 * that staged an item must therefore end in Push() or Revert().  A caller that drops such a commit
 * on purpose must request the recalculation itself.
 */
class SCH_COMMIT : public COMMIT
{
public:
    /**
     * A commit with no editor: the changes are applied to the schematic only.  Used by
     * kicad-cli and by the data model to record clean-up changes.
     */
    SCH_COMMIT();

    /// A commit in the schematic editor, which @a aListener keeps in step with the schematic.
    explicit SCH_COMMIT( std::unique_ptr<SCH_COMMIT_LISTENER> aListener );

    /// Commits in an editor (sch_commit_gui.cpp).
    SCH_COMMIT( TOOL_MANAGER* aToolMgr );
    SCH_COMMIT( TOOL_MANAGER* aToolMgr, bool aIsLibEditor );
    SCH_COMMIT( EDA_DRAW_FRAME* aFrame );
    SCH_COMMIT( SCH_TOOL_BASE<SCH_BASE_FRAME>* aFrame );

    virtual ~SCH_COMMIT();

    void Push( const wxString& aMessage = wxEmptyString, int aCommitFlags = 0 ) override;

    void Revert() override;
    void RevertToCheckpoint( int aCheckpoint ) override;

    COMMIT& Stage( EDA_ITEM *aItem, CHANGE_TYPE aChangeType, BASE_SCREEN *aScreen = nullptr,
                   RECURSE_MODE aRecurse = RECURSE_MODE::NO_RECURSE ) override;
    COMMIT& Stage( std::vector<EDA_ITEM*> &container, CHANGE_TYPE aChangeType,
                   BASE_SCREEN *aScreen = nullptr ) override;

    EDA_ITEM* ResolveItem( KIID& aID ) override;

    /** Retain the pre-edit state of an item already removed from its screen by cleanup. */
    void RemovedForCleanup( SCH_ITEM* aItem, SCH_SCREEN* aScreen );

protected:
    EDA_ITEM* undoLevelItem( EDA_ITEM* aItem ) const override;

    EDA_ITEM* makeImage( EDA_ITEM* aItem ) const override;

    void pushSchEdit(  const wxString& aMessage, int aCommitFlags );

private:
    std::unique_ptr<SCH_COMMIT_LISTENER> m_listener;
    bool                                 m_isLibEditor;
};
