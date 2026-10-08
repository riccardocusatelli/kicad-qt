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
 * @file eda_list_dialog.h
 * EDA_LIST_DIALOG of the Qt build: the layout of eda_list_dialog_base.fbp, with the widgets of
 * Qt (see include/eda_list_dialog.h).
 */

#pragma once

#include <map>
#include <vector>

#include <dialogs/eda_list_dialog_base.h>
#include <toolkit/toolkit_types.h>
#include <wx/arrstr.h>

class QCheckBox;


/**
 * A dialog which shows:
 *   a list of elements for selection,
 *   a text control to display help or info about the selected item.
 *   2 buttons (OK and Cancel)
 */
class EDA_LIST_DIALOG : public EDA_LIST_DIALOG_BASE
{
public:

    /**
     * @param aParent Pointer to the parent window.
     * @param aTitle The title shown on top.
     * @param aItemHeaders an optional array containing the column header names for the dialog.
     * @param aItemList A wxArrayString of the list of elements.
     * @param aPreselectText An item name if an item must be preselected.
     */
    EDA_LIST_DIALOG( TOOLKIT_WINDOW* aParent, const wxString& aTitle,
                     const wxArrayString& aItemHeaders,
                     const std::vector<wxArrayString>& aItemList,
                     const wxString& aPreselectText = wxEmptyString, bool aSortList = true );

    EDA_LIST_DIALOG( TOOLKIT_WINDOW* aParent, const wxString& aTitle, bool aSortList = true );

    void SetListLabel( const wxString& aLabel );
    void SetOKLabel( const wxString& aLabel );
    void HideFilter();

    void Append( const wxArrayString& aItemStr );
    void InsertItems( const std::vector<wxArrayString>& aItemList, int aPosition = 0 );

    /**
     * Return the selected text from \a aColumn in the wxListCtrl in the dialog.
     *
     * @param aColumn is the column to return the text from.
     * @return a wxString object containing the selected text from \a aColumn.
     */
    wxString GetTextSelection( int aColumn = 0 );

    /**
     * Add a checkbox value to the dialog.
     *
     * @param aLabel is the label of the checkbox
     * @param aValuePtr is a pointer to the value of the checkbox; it is initialized from the value
     *        and updated when the dialog is closed with OK.
     */
    void AddExtraCheckbox( const wxString& aLabel, bool* aValuePtr );

    /**
     * Fills in the value pointers from the checkboxes after the dialog has run.
     */
    void GetExtraCheckboxValues();

    long GetSelection();

protected:
    void initDialog( const wxArrayString& aItemHeaders, const std::vector<wxArrayString>& aItemList,
                     const wxString& aPreselectText);

    void textChangeInFilterBox( QWidget* aSource ) override;

private:
    void onSize( QWidget* aSource ) override;
    void onListItemActivated( QWidget* aSource ) override;

    void sortList();

private:
    // The list of items, locally stored
    std::vector<wxArrayString>   m_itemsList;
    bool                         m_sortList;
    std::map<QCheckBox*, bool*>  m_extraCheckboxMap;
};
