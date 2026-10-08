/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FRAME_PANES_H
#define FRAME_PANES_H

/**
 * The side of a frame a pane is docked to.
 *
 * The values are the ones stored in the settings (the dock directions of wxAUI), so they must
 * not change.
 */
enum class PANE_DOCK : int
{
    NONE   = 0,
    TOP    = 1,
    RIGHT  = 2,
    BOTTOM = 3,
    LEFT   = 4,
    CENTER = 5
};

#endif // FRAME_PANES_H
