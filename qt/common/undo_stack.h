/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <functional>

#include <undo_redo_container.h>


/**
 * The undo and redo lists of a document of the Qt shell, and the state saved last.
 *
 * The entries are the PICKED_ITEMS_LISTs made by the commits; the document applies them
 * (BOARD_UNDO_REDO, SCH_UNDO_REDO).  Compiled into each Qt KIFACE, with the model.
 */
class UNDO_STACK
{
public:
    /// Undo or redo an entry, turning it into its reverse.
    using APPLY = std::function<void( PICKED_ITEMS_LIST& aEntry )>;

    /// Delete the items an entry owns (deleted items, copies).
    using DELETE_ITEMS = std::function<void( PICKED_ITEMS_LIST& aEntry )>;

    explicit UNDO_STACK( DELETE_ITEMS aDeleteItems );
    ~UNDO_STACK();

    UNDO_STACK( const UNDO_STACK& ) = delete;
    UNDO_STACK& operator=( const UNDO_STACK& ) = delete;

    /// Add the entry of a new change (the stack takes it); the changes undone are forgotten.
    void Push( PICKED_ITEMS_LIST* aEntry );

    bool CanUndo() const { return !m_undo.m_CommandsList.empty(); }
    bool CanRedo() const { return !m_redo.m_CommandsList.empty(); }

    /// The entry of the last change, or null.
    PICKED_ITEMS_LIST* LastChange() const;

    void Undo( const APPLY& aApply );
    void Redo( const APPLY& aApply );

    /// True if the document differs from the state saved last (or loaded).
    bool IsModified() const;

    /// The document has been saved in its current state.
    void SetSaved();

    /// Forget all the entries.
    void Clear();

private:
    static void move( UNDO_REDO_CONTAINER& aFrom, UNDO_REDO_CONTAINER& aTo, const APPLY& aApply );

    void clear( UNDO_REDO_CONTAINER& aList );

    UNDO_REDO_CONTAINER m_undo;
    UNDO_REDO_CONTAINER m_redo;
    DELETE_ITEMS        m_deleteItems;

    /// The number of undo entries in the state saved last; -1 when that state is lost
    int                 m_savedDepth = 0;
};
