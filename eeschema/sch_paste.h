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

#include <wx/string.h>

#include <kiid.h>

class LIB_SYMBOL;
class SCH_SCREEN;
class SCH_SHEET_PATH;
class SCH_SYMBOL;


/**
 * The parts of a paste of schematic items that do not depend on the editor: the library
 * symbols and the instances of the pasted symbols.  Used by the schematic editor
 * (SCH_EDITOR_CONTROL) and by the Qt shell.
 */
class SCH_PASTE
{
public:
    /**
     * Choose which cached library symbol a pasted instance should adopt.
     *
     * The clipboard and destination screens may both cache a same-named library symbol that
     * differs (e.g. pins renumbered or power type changed in place). The clipboard copy is a
     * matched pair with the pasted instance, so it must win to preserve the copied edits; the
     * destination cache is only a fallback when the clipboard carries no copy.
     *
     * @param aClipboardScreen Screen holding the clipboard's cached library symbols.
     * @param aDestScreen      Destination schematic screen.
     * @param aLibSymbolName   Cached library symbol name to look up in both screens.
     * @return Non-owning pointer to the chosen source symbol, or nullptr if neither has it.
     */
    static const LIB_SYMBOL* ChooseLibSymbol( const SCH_SCREEN* aClipboardScreen,
                                              const SCH_SCREEN* aDestScreen,
                                              const wxString&   aLibSymbolName );

    /**
     * Give a pasted symbol its instance for the sheet it is pasted in: the one it had in the
     * clipboard (@a aClipPath), or its fields when the clipboard has none.
     *
     * @param aProjectName the project of the destination schematic.
     * @param aKeepAnnotation keep the reference of the clipboard; otherwise clear it.
     */
    static void UpdateSymbolInstance( SCH_SYMBOL* aSymbol, const SCH_SHEET_PATH& aPastePath,
                                      const KIID_PATH& aClipPath, const wxString& aProjectName,
                                      bool aKeepAnnotation );

    /**
     * Remove from a pasted symbol the instances that are not of @a aProjectName (the ones it
     * had in its source schematic).
     *
     * @warning This should **only** be called when cleaning up after a paste.  Otherwise it
     *          could clobber symbol instances for schematics shared across projects.
     */
    static void PruneSymbolInstances( SCH_SYMBOL* aSymbol, const wxString& aProjectName );
};
