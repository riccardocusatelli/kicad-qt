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

/**
 * @file treelist.h
 * The tree with columns of the wxWidgets compatibility layer (wxTreeListCtrl): as wxWidgets, a
 * wxDataViewCtrl (GetDataView()) showing a model of the items of the control.
 */

#pragma once

#include <memory>
#include <vector>

#include <wx/clntdata.h>
#include <wx/kiwx_dataview.h>
#include <wx/kiwx_window.h>


#define wxCOL_RESIZABLE    wxDATAVIEW_COL_RESIZABLE
#define wxCOL_SORTABLE     wxDATAVIEW_COL_SORTABLE
#define wxCOL_REORDERABLE  wxDATAVIEW_COL_REORDERABLE
#define wxCOL_HIDDEN       wxDATAVIEW_COL_HIDDEN
#define wxCOL_DEFAULT_FLAGS wxCOL_RESIZABLE

#define wxTL_SINGLE          0x0000
#define wxTL_MULTIPLE        0x0001
#define wxTL_CHECKBOX        0x0002
#define wxTL_3STATE          0x0004
#define wxTL_USER_3STATE     0x0008
#define wxTL_NO_HEADER       0x0010
#define wxTL_DEFAULT_STYLE   wxTL_SINGLE
#define wxTL_STYLE_MASK      ( wxTL_SINGLE | wxTL_MULTIPLE | wxTL_CHECKBOX | wxTL_3STATE \
                               | wxTL_USER_3STATE )

class wxTreeListModelCompat;


/// An item of a wxTreeListCtrl (wxTreeListItem): an opaque id.
class wxTreeListItem
{
public:
    wxTreeListItem( void* aId = nullptr ) : m_id( aId ) {}

    bool IsOk() const { return m_id != nullptr; }
    void* GetID() const { return m_id; }
    void Unset() { m_id = nullptr; }

    bool operator==( const wxTreeListItem& aOther ) const { return m_id == aOther.m_id; }
    bool operator!=( const wxTreeListItem& aOther ) const { return m_id != aOther.m_id; }

private:
    void* m_id;
};

typedef std::vector<wxTreeListItem> wxTreeListItems;


/// The events of a wxTreeListCtrl (wxTreeListEvent).
class KIQT_API wxTreeListEvent : public wxNotifyEvent
{
public:
    explicit wxTreeListEvent( wxEventType aType = wxEVT_NULL, int aId = 0,
                              const wxTreeListItem& aItem = wxTreeListItem() ) :
            wxNotifyEvent( aType, aId ),
            m_item( aItem )
    {
    }

    wxTreeListItem GetItem() const { return m_item; }
    unsigned GetColumn() const { return m_column; }
    void SetColumn( unsigned aColumn ) { m_column = aColumn; }
    wxCheckBoxState GetOldCheckedState() const { return m_oldCheckedState; }
    void SetOldCheckedState( wxCheckBoxState aState ) { m_oldCheckedState = aState; }

private:
    wxTreeListItem  m_item;
    unsigned        m_column = unsigned( -1 );
    wxCheckBoxState m_oldCheckedState = wxCHK_UNDETERMINED;
};

#define wxTreeListEventHandler( func ) ( &func )


class KIQT_API wxTreeListCtrl : public wxWindowCompat<QWidget>
{
public:
    explicit wxTreeListCtrl( wxWidgetArgCompat aParent = nullptr, int aId = wxID_ANY,
                             const wxPoint& aPos = wxDefaultPosition,
                             const wxSize& aSize = wxDefaultSize,
                             long aStyle = wxTL_DEFAULT_STYLE,
                             const wxString& aName = wxEmptyString );
    ~wxTreeListCtrl() override;

    // The columns

    int AppendColumn( const wxString& aTitle, int aWidth = wxCOL_WIDTH_AUTOSIZE,
                      wxAlignment aAlign = wxALIGN_LEFT, int aFlags = wxCOL_RESIZABLE );
    unsigned GetColumnCount() const;
    bool DeleteColumn( unsigned aColumn );
    void ClearColumns();
    void SetColumnWidth( unsigned aColumn, int aWidth );
    int GetColumnWidth( unsigned aColumn ) const;
    int WidthFor( const wxString& aText ) const;

    // The items

    wxTreeListItem AppendItem( wxTreeListItem aParent, const wxString& aText,
                               int aImageClosed = -1, int aImageOpened = -1,
                               wxClientData* aData = nullptr );
    wxTreeListItem InsertItem( wxTreeListItem aParent, wxTreeListItem aPrevious,
                               const wxString& aText, int aImageClosed = -1,
                               int aImageOpened = -1, wxClientData* aData = nullptr );
    wxTreeListItem PrependItem( wxTreeListItem aParent, const wxString& aText,
                                int aImageClosed = -1, int aImageOpened = -1,
                                wxClientData* aData = nullptr );
    void DeleteItem( wxTreeListItem aItem );
    void DeleteAllItems();

    wxTreeListItem GetRootItem() const;
    wxTreeListItem GetItemParent( wxTreeListItem aItem ) const;
    wxTreeListItem GetFirstChild( wxTreeListItem aItem ) const;
    wxTreeListItem GetNextSibling( wxTreeListItem aItem ) const;
    wxTreeListItem GetFirstItem() const { return GetNextItem( GetRootItem() ); }
    wxTreeListItem GetNextItem( wxTreeListItem aItem ) const;

    const wxString& GetItemText( wxTreeListItem aItem, unsigned aColumn = 0 ) const;
    void SetItemText( wxTreeListItem aItem, unsigned aColumn, const wxString& aText );
    void SetItemText( wxTreeListItem aItem, const wxString& aText )
    {
        SetItemText( aItem, 0, aText );
    }

    wxClientData* GetItemData( wxTreeListItem aItem ) const;
    void SetItemData( wxTreeListItem aItem, wxClientData* aData );

    void Expand( wxTreeListItem aItem );
    void Collapse( wxTreeListItem aItem );
    bool IsExpanded( wxTreeListItem aItem ) const;

    // The selection

    unsigned GetSelections( wxTreeListItems& aSelections ) const;
    wxTreeListItem GetSelection() const;
    void Select( wxTreeListItem aItem );
    void Unselect( wxTreeListItem aItem );
    bool IsSelected( wxTreeListItem aItem ) const;
    void SelectAll();
    void UnselectAll();
    void EnsureVisible( wxTreeListItem aItem );

    // The check boxes (wxTL_CHECKBOX)

    void CheckItem( wxTreeListItem aItem, wxCheckBoxState aState = wxCHK_CHECKED );
    void UncheckItem( wxTreeListItem aItem ) { CheckItem( aItem, wxCHK_UNCHECKED ); }
    wxCheckBoxState GetCheckedState( wxTreeListItem aItem ) const;

    // The sorting: the order of the items is kept

    void SetSortColumn( unsigned aColumn, bool aAscendingOrder = true );
    bool GetSortColumn( unsigned* aColumn, bool* aAscendingOrder = nullptr );

    /// The data view showing the items.
    wxDataViewCtrl* GetDataView() const { return m_view; }
    wxWindow* GetView() const { return m_view; }

private:
    /// Send an event of the control for an item of the view.
    void sendEvent( wxEventType aType, const wxDataViewItem& aItem );

    wxDataViewCtrl*        m_view;
    wxTreeListModelCompat* m_model;
    long                   m_style;
};
