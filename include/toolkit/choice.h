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

#pragma once

/**
 * @file choice.h
 * The choice boxes (drop-down lists of strings) of the toolkit-independent code (the controls of
 * the toolbars, the choices of the dialogs).
 *
 * This is the wxWidgets version; the Qt build has its own version of this header
 * (qt/include/toolkit/choice.h), earlier in its include path.
 */

#include <functional>

#include <wx/choice.h>

#include <import_export.h>
#include <toolkit/toolkit_types.h>

class wxArrayString;
class wxString;


namespace TOOLKIT
{

/**
 * Create a choice box.
 *
 * @param aParent is the window of the choice box (a toolbar).
 * @param aId is the id of the choice box (its id in the toolbar).
 * @param aOnSelect is called when the user selects an item.
 */
APIVISIBLE TOOLKIT_CHOICE* CreateChoice( TOOLKIT_WINDOW* aParent, int aId,
                                         std::function<void()> aOnSelect );

APIVISIBLE void ChoiceClear( TOOLKIT_CHOICE* aChoice );
APIVISIBLE void ChoiceAppend( TOOLKIT_CHOICE* aChoice, const wxString& aItem );

/// Replace the items of a choice box.
APIVISIBLE void ChoiceSet( TOOLKIT_CHOICE* aChoice, const wxArrayString& aItems );

APIVISIBLE int  ChoiceGetCount( const TOOLKIT_CHOICE* aChoice );

/// @return the index of the selected item, or wxNOT_FOUND.
APIVISIBLE int  ChoiceGetSelection( const TOOLKIT_CHOICE* aChoice );
APIVISIBLE void ChoiceSetSelection( TOOLKIT_CHOICE* aChoice, int aIndex );

APIVISIBLE wxString ChoiceGetString( const TOOLKIT_CHOICE* aChoice, int aIndex );

/// @return the index of the item @a aItem, or wxNOT_FOUND.
APIVISIBLE int  ChoiceFindString( const TOOLKIT_CHOICE* aChoice, const wxString& aItem );

APIVISIBLE void ChoiceSetToolTip( TOOLKIT_CHOICE* aChoice, const wxString& aTip );

} // namespace TOOLKIT
