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

#ifndef EXPORT_IDF_H
#define EXPORT_IDF_H

class BOARD;
class PROJECT;
class wxString;

/**
 * Write IDFv3 board (.emn) and library (.emp) files for @a aPcb.
 *
 * @param aProject is used to resolve the 3D model paths.
 * @param aErrorMsg (optional) receives the error message on failure.
 * @return true on success.
 */
bool ExportBoardToIDF3( BOARD* aPcb, PROJECT* aProject, const wxString& aFullFileName,
                        bool aUseThou, double aXRef, double aYRef, bool aIncludeUnspecified,
                        bool aIncludeDNP, wxString* aErrorMsg = nullptr );

#endif // EXPORT_IDF_H
