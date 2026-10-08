
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

/**
 * @file filter_combobox.h
 * FILTER_COMBOBOX of the Qt build: a combo box whose list is shown in a popup with a filter
 * (see include/widgets/filter_combobox.h).
 */

#pragma once

#include <functional>
#include <optional>
#include <vector>

#include <kiqt.h>     // Qt, without its METHOD macro
#include <QComboBox>
#include <QFrame>

#include <wx/arrstr.h>
#include <wx/gdicmn.h>
#include <wx/string.h>

class QLineEdit;
class QListWidget;
class FILTER_COMBOBOX;


/**
 * The popup of a FILTER_COMBOBOX: a filter and the list of the strings matching it.
 */
class FILTER_COMBOPOPUP : public QFrame
{
public:
    FILTER_COMBOPOPUP();
    ~FILTER_COMBOPOPUP() override;

    /// Make the controls of the popup, for the combo box @a aParent.
    bool Create( FILTER_COMBOBOX* aParent );

    void SetStringList( const wxArrayString& aStringList );

    virtual wxString GetStringValue() const;

    void SetSelectedString( const wxString& aString );

    /// The popup is shown.
    void OnPopup();

    /// The popup was opened by a key: the key starts the filter.
    void OnStartingKey( QKeyEvent* aEvent );

    virtual void Accept();

    /// Hide the popup.
    void Dismiss();

    FILTER_COMBOBOX* GetComboCtrl() const { return m_combo; }

protected:
    /**
     * Get the currently selected value in the list, or std::nullopt
     */
    std::optional<wxString> getSelectedValue() const;

    /**
     * Get the current value of the filter control. Can be empty.
     */
    wxString getFilterValue() const;

    /**
     * Fill the combobox list
     */
    virtual void getListContent( wxArrayString& aStringList );

    /**
     * Call this to rebuild the list from the getListContent() method.
     */
    void rebuildList();

    /// Set the text shown by the combo box.
    void setComboValue( const wxString& aValue );

    /// Tell the combo box that an item was selected (after the event, as FILTERED_ITEM_SELECTED).
    void notifySelected();

    bool eventFilter( QObject* aWatched, QEvent* aEvent ) override;
    void hideEvent( QHideEvent* aEvent ) override;

private:
    /// Fit the popup to its list (at least the width of the combo box).
    void updateSize();

    void doStartingKey( QKeyEvent* aEvent );

    /// Handle the keys of the popup (from the filter or the list).
    bool onKeyDown( QKeyEvent* aEvent );

protected:
    QLineEdit*       m_filterCtrl;
    QListWidget*     m_listBox;
    FILTER_COMBOBOX* m_combo;
    wxString         m_selectedString;
    wxArrayString    m_stringList;
};


/**
 * A combo box showing its list in a popup with a filter.
 */
class FILTER_COMBOBOX : public QComboBox
{
public:
    // C'tor matching wxFormBuilder's Custom Control
    FILTER_COMBOBOX( QWidget* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize, long style = 0 );

    // C'tor matching wxFormBuilder's ComboxBox.
    FILTER_COMBOBOX( QWidget* parent, wxWindowID id, const wxString& value,
                     const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                     int count = 0, wxString strings[] = nullptr, long style = 0 );

    ~FILTER_COMBOBOX() override;

    virtual void SetStringList( const wxArrayString& aStringList );
    virtual void SetSelectedString( const wxString& aString );

    /// Set the text shown by the combo box.
    void SetValue( const wxString& aValue );

    /// @return the text shown by the combo box.
    wxString GetValue() const;

    /// Call @a aHandler when an item of the popup is selected (FILTERED_ITEM_SELECTED).
    void OnFilteredItemSelected( std::function<void()> aHandler );

    /// Tell the handlers that an item was selected (after the current event).
    void NotifyItemSelected();

    void showPopup() override;
    void hidePopup() override;

    bool IsPopupShown() const;

protected:
    void setFilterPopup( FILTER_COMBOPOPUP* aPopup );

    void keyPressEvent( QKeyEvent* aEvent ) override;

    FILTER_COMBOPOPUP*                 m_filterPopup;
    long                               m_style;
    std::vector<std::function<void()>> m_selectedHandlers;
};
