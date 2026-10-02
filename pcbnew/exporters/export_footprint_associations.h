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

#ifndef EXPORT_FOOTPRINT_ASSOCIATIONS_H
#define EXPORT_FOOTPRINT_ASSOCIATIONS_H

class BOARD;
class wxString;

/**
 * Write the footprint association (.cmp) file of @a aBrd: reference, value and footprint of
 * each footprint.  @return false if the file cannot be created.
 */
bool RecreateCmpFile( BOARD* aBrd, const wxString& aFullCmpFileName );

#endif // EXPORT_FOOTPRINT_ASSOCIATIONS_H
