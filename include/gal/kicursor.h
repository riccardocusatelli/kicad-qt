/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#ifndef KICURSOR_H
#define KICURSOR_H

/// Cursor shapes requested by tools and the GAL.  Turned into native cursors by
/// CURSOR_STORE (gal/cursors.h), which is the only part that needs the GUI toolkit.
enum class KICURSOR
{
    DEFAULT,
    ARROW,
    ARROW64,
    MOVING,
    MOVING64,
    WARNING,
    WARNING64,
    PENCIL,
    PENCIL64,
    REMOVE,
    REMOVE64,
    HAND,
    HAND64,
    BULLSEYE,
    BULLSEYE64,
    VOLTAGE_PROBE,
    VOLTAGE_PROBE64,
    CURRENT_PROBE,
    CURRENT_PROBE64,
    TUNE,
    TUNE64,
    TEXT,
    TEXT64,
    MEASURE,
    MEASURE64,
    ADD,
    ADD64,
    SUBTRACT,
    SUBTRACT64,
    XOR,
    XOR64,
    ZOOM_IN,
    ZOOM_IN64,
    ZOOM_OUT,
    ZOOM_OUT64,
    LABEL_NET,
    LABEL_NET64,
    LABEL_GLOBAL,
    LABEL_GLOBAL64,
    COMPONENT,
    COMPONENT64,
    SELECT_WINDOW,
    SELECT_WINDOW64,
    SELECT_LASSO,
    SELECT_LASSO64,
    LINE_BUS,
    LINE_BUS64,
    LINE_GRAPHIC,
    LINE_GRAPHIC64,
    LINE_WIRE,
    LINE_WIRE64,
    LABEL_HIER,
    LABEL_HIER64,
    PLACE,
    PLACE64
};

#endif // KICURSOR_H
