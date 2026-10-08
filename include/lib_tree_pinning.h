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


/**
 * @file lib_tree_pinning.h
 * The mark of the pinned libraries in the library trees (LIB_TREE_MODEL_ADAPTER), without the
 * GUI toolkit: the lists that show the names of the trees remove it.
 */

#pragma once

#include <wx/string.h>


/// The symbol before the names of the pinned libraries.
inline wxString LibTreePinningSymbol()
{
    return wxString::FromUTF8( "☆ " );
}
