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

#pragma once

#include <commit.h>
#include <math/box2.h>

#include <memory>
#include <unordered_set>
#include <vector>

class BOARD_ITEM;
class PCB_GROUP;
class PCB_SHAPE;
class ZONE;
class BOARD;
class PICKED_ITEMS_LIST;
class PCB_TOOL_BASE;
class TOOL_MANAGER;
class EDA_DRAW_FRAME;
class TOOL_BASE;

#define SKIP_UNDO          0x0001
#define APPEND_UNDO        0x0002
#define SKIP_SET_DIRTY     0x0004
#define SKIP_CONNECTIVITY  0x0008
#define ZONE_FILL_OP       0x0010
#define SKIP_TEARDROPS     0x0020
#define SKIP_ENTERED_GROUP 0x0040


/**
 * The editor side of a BOARD_COMMIT.
 *
 * The commit applies its changes to the BOARD (the data model); the listener keeps the view,
 * the selection, the undo list and the frame in step with them.  A commit without a listener
 * (kicad-cli, scripts, tests) only changes the board.
 *
 * board_commit_gui.cpp implements the listener on top of a TOOL_MANAGER.
 */
class BOARD_COMMIT_LISTENER
{
public:
    /// What a BOARD_COMMIT::Push() did, for BOARD_COMMIT_LISTENER::Pushed().
    struct PUSH_RESULT
    {
        bool               itemsDeselected = false;   ///< Removed items were selected
        bool               selectedModified = false;  ///< Selected items were changed
        bool               autofillZones = false;     ///< staleZones must be refilled
        std::vector<ZONE*> staleZones;
    };

    virtual ~BOARD_COMMIT_LISTENER() = default;

    /// The board being edited.
    virtual BOARD* Board() const = 0;

    /// The group the user has entered (new items are added to it), if any.
    virtual PCB_GROUP* EnteredGroup() const = 0;

    /// True if the zones touched by a commit are refilled automatically.
    virtual bool AutoRefillZones() const = 0;

    /// @a aItem has been added to, removed from or changed in the board.
    virtual void ItemAdded( BOARD_ITEM* aItem ) = 0;
    virtual void ItemRemoved( BOARD_ITEM* aItem ) = 0;
    virtual void ItemChanged( BOARD_ITEM* aItem ) = 0;

    /// @a aItem is selected and is being removed from the board: drop it from the selection.
    virtual void Deselect( BOARD_ITEM* aItem ) = 0;

    /**
     * Drop from the selection the items that are descendants of @a aRemoved (pads, fields,
     * table cells...): they are freed with their parent.
     * @return true if the selection has changed.
     */
    virtual bool PruneSelection( const std::unordered_set<EDA_ITEM*>& aRemoved ) = 0;

    virtual void RatsnestChanged() = 0;
    virtual void SolderMaskChanged() = 0;

    /// The board outline has been rebuilt.
    virtual void BoardOutlineChanged() = 0;

    /// Zones that can have custom DRC rules have changed in @a aAreas.
    virtual void RuleAreasChanged( const std::vector<BOX2I>& aAreas ) = 0;

    /**
     * The commit has been applied.
     * @param aUndoList the undo entries (empty with SKIP_UNDO); the listener takes them.
     */
    virtual void Pushed( PICKED_ITEMS_LIST& aUndoList, int aCommitFlags,
                         const PUSH_RESULT& aResult ) = 0;

    /// The commit has been reverted.
    virtual void Reverted() = 0;
};


class BOARD_COMMIT : public COMMIT
{
public:
    /**
     * A commit on @a aBoard with no editor: the changes are applied to the board only.
     * Used by kicad-cli, the scripting API and the tests.
     */
    explicit BOARD_COMMIT( BOARD* aBoard, bool aIsBoardEditor = false,
                           bool aIsFootprintEditor = false );

    /// A commit in an editor, which @a aListener keeps in step with the board.
    BOARD_COMMIT( std::unique_ptr<BOARD_COMMIT_LISTENER> aListener, bool aIsBoardEditor,
                  bool aIsFootprintEditor );

    /// Commits in an editor (board_commit_gui.cpp).
    BOARD_COMMIT( EDA_DRAW_FRAME* aFrame );
    BOARD_COMMIT( TOOL_BASE* aTool );
    BOARD_COMMIT( TOOL_MANAGER* aMgr );
    BOARD_COMMIT( TOOL_MANAGER* aMgr, bool aIsBoardEditor, bool aIsFootprintEditor );

    virtual ~BOARD_COMMIT();

    BOARD* GetBoard() const;

    virtual void Push( const wxString& aMessage = wxEmptyString, int aCommitFlags = 0 ) override;

    virtual void Revert() override;
    COMMIT&      Stage( EDA_ITEM* aItem, CHANGE_TYPE aChangeType,
                        BASE_SCREEN* aScreen = nullptr,
                        RECURSE_MODE aRecurse = RECURSE_MODE::NO_RECURSE ) override;
    COMMIT&      Stage( std::vector<EDA_ITEM*>& container, CHANGE_TYPE aChangeType,
                        BASE_SCREEN* aScreen = nullptr ) override;
    COMMIT&      Stage( const PICKED_ITEMS_LIST& aItems,
                        UNDO_REDO aModFlag = UNDO_REDO::UNSPECIFIED,
                        BASE_SCREEN* aScreen = nullptr ) override;

    static EDA_ITEM* MakeImage( EDA_ITEM* aItem );

private:
    EDA_ITEM* undoLevelItem( EDA_ITEM* aItem ) const override;

    EDA_ITEM* makeImage( EDA_ITEM* aItem ) const override;

    void propagateDamage( BOARD_ITEM* aItem, std::vector<ZONE*>* aStaleZones,
                          std::vector<BOX2I>& aStaleRuleAreas );

private:
    BOARD*                                 m_board;      ///< Set when there is no listener
    std::unique_ptr<BOARD_COMMIT_LISTENER> m_listener;
    bool                                   m_isBoardEditor;
    bool                                   m_isFootprintEditor;
};
