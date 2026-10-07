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

#include <map>
#include <vector>

#include <gal/gal.h>
#include <gal/kicursor.h>
#include <math/vector2d.h>


/**
 * The image of a cursor of KiCad, at one size.
 */
struct CURSOR_DEF
{
    const char** m_xpm;             ///< The image (XPM)
    VECTOR2I     m_hotspot;         ///< The "hotspot" where the cursor "is" in the image
};


/**
 * The images of the cursors drawn by KiCad (not the cursors of the system): the standard
 * size, then the HiDPI one.  Each GUI toolkit makes its cursors from them (CURSOR_STORE).
 */
GAL_API const std::map<KICURSOR, std::vector<CURSOR_DEF>>& CursorDefinitions();
