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

#include <vector>

#include <math/box2.h>
#include <math/vector2d.h>
#include <undo_redo_container.h>

class BOARD;
class DS_PROXY_UNDO_ITEM;
class EDA_ITEM;


/**
 * The undo and redo of the changes of a board, without an editor.
 *
 * An undo entry is a PICKED_ITEMS_LIST, as made by BOARD_COMMIT::Push(): the changed items
 * with their copies, the new items and the deleted ones.  Apply() puts the board back in the
 * state before the entry (or after it, for a redo) and turns the entry into its reverse.  The
 * editor is told of the changes by a BOARD_UNDO_REDO::LISTENER: the PCB editors through their
 * frame (undo_redo.cpp), the Qt shell through its document.
 */
class BOARD_UNDO_REDO
{
public:
    /// The editor of the board, which shows it.
    class LISTENER
    {
    public:
        virtual ~LISTENER() = default;

        /// @a aItem leaves the board (or is about to be changed): remove it from the view.
        virtual void ItemRemoved( EDA_ITEM* aItem ) = 0;

        /// @a aItem is (again) on the board: show it.
        virtual void ItemAdded( EDA_ITEM* aItem ) = 0;

        /// True if the ratsnest is shown: the restored items show their own ratsnest so.
        virtual bool ShowRatsnest() const = 0;

        /**
         * Move the drill origin (UNDO_REDO::DRILLORIGIN) or the grid origin
         * (UNDO_REDO::GRIDORIGIN) to @a aPosition.
         * @param aOriginItem the item of the undo entry for the origin.
         */
        virtual void SetOrigin( UNDO_REDO aOrigin, EDA_ITEM* aOriginItem,
                                const VECTOR2I& aPosition ) = 0;

        /// Exchange the page settings (and title block) of the editor with the ones in @a aItem.
        virtual void SwapPageSettings( DS_PROXY_UNDO_ITEM* aItem ) = 0;
    };

    /// What Apply() did, for the editor.
    struct RESULT
    {
        bool               itemsNotFound = false;   ///< Some items were no more on the board
        bool               connectivityChanged = false;
        bool               solderMaskChanged = false;
        bool               shapesChanged = false;   ///< The outline or hatching may have changed
        std::vector<BOX2I> dirtyRuleAreas;          ///< Areas of the changed rule areas
    };

    /**
     * Complete the pickers of an undo entry from @a aFirst: an UNSPECIFIED status becomes
     * @a aCommandType, and a changed item without a copy gets one.
     */
    static void CompleteEntry( PICKED_ITEMS_LIST& aEntry, unsigned aFirst, UNDO_REDO aCommandType );

    /**
     * Undo (or redo) @a aEntry on @a aBoard, and turn it into the entry that redoes (or undoes)
     * it: the caller then reverses the picker order and moves it to the other list.
     *
     * @param aRebuildConnectivity rebuild the connectivity of the board if items with nets
     *                             have changed (the board editor).
     */
    static RESULT Apply( BOARD& aBoard, PICKED_ITEMS_LIST& aEntry, LISTENER& aListener,
                         bool aRebuildConnectivity );

    /// Delete the items owned by @a aEntry (deleted items and copies) and clear it.
    static void DeleteItems( PICKED_ITEMS_LIST& aEntry );
};
