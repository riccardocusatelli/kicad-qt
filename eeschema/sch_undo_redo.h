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

#pragma once

#include <sch_sheet_path.h>
#include <schematic.h>
#include <undo_redo_container.h>

class DS_PROXY_UNDO_ITEM;
class EDA_ITEM;
class SCH_ITEM;
class SCH_SCREEN;
class SCH_SHEET;


/**
 * The undo and redo of the changes of a schematic, without an editor.
 *
 * An undo entry is a PICKED_ITEMS_LIST, as made by SCH_COMMIT::Push(): the changed items with
 * their copies, the new items and the deleted ones, each with its screen.  Apply() puts the
 * schematic back in the state before the entry (or after it, for a redo) and turns the entry
 * into its reverse.  The editor is told of the changes by a SCH_UNDO_REDO::LISTENER: the
 * schematic editor through its frame (schematic_undo_redo.cpp), the Qt shell through its
 * document.
 */
class SCH_UNDO_REDO
{
public:
    /// The editor of the schematic, which shows it.
    class LISTENER
    {
    public:
        virtual ~LISTENER() = default;

        /// The sheet shown by the editor.
        virtual const SCH_SHEET_PATH& CurrentSheet() const = 0;

        /// Add @a aItem to @a aScreen, and show it if the screen is the one shown.
        virtual void AddToScreen( EDA_ITEM* aItem, SCH_SCREEN* aScreen ) = 0;

        /// Remove @a aItem from @a aScreen (and from the view if the screen is the one shown).
        virtual void RemoveFromScreen( EDA_ITEM* aItem, SCH_SCREEN* aScreen ) = 0;

        /// @a aSheet is about to be removed: leave it if it is the one shown.
        virtual void SheetRemoving( SCH_SHEET* aSheet ) = 0;

        /**
         * Exchange the page settings (and title block) of @a aSheet with the ones in @a aItem.
         */
        virtual void SwapPageSettings( DS_PROXY_UNDO_ITEM* aItem, const SCH_SHEET_PATH& aSheet ) = 0;

        /// The items repeated by the editor (insert key): clear them, then add the restored ones.
        virtual void ClearRepeatItems() = 0;
        virtual void AddRepeatItem( SCH_ITEM* aItem ) = 0;
    };

    /// What Apply() did, for the editor.
    struct RESULT
    {
        bool              connectivityChanged = false;  ///< The connections must be recalculated
        bool              connectablesChanged = false;  ///< Connectable items have changed
        SCH_CLEANUP_FLAGS connectivityCleanUp = NO_CLEANUP;
        bool              hierarchyChanged = false;     ///< Sheets were added, removed or renamed
        bool              variantsChanged = false;      ///< Symbols or sheets have changed
    };

    /**
     * Undo (or redo) @a aEntry on @a aSchematic, and turn it into the entry that redoes (or
     * undoes) it: the caller then reverses the picker order and moves it to the other list.
     */
    static RESULT Apply( SCHEMATIC& aSchematic, PICKED_ITEMS_LIST& aEntry, LISTENER& aListener );

    /// Delete the items owned by @a aEntry (deleted items and copies) and clear it.
    static void DeleteItems( PICKED_ITEMS_LIST& aEntry );
};
