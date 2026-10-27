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
 * @file kiwx_treectrl.h
 * The tree control of the wxWidgets compatibility layer (wxTreeCtrl, wxTreeItemId,
 * wxTreeItemData, wxTreeEvent) on a QTreeWidget: an item is a QTreeWidgetItem, the data of the
 * items belong to the tree.
 */

#ifndef KIWX_TREECTRL_H
#define KIWX_TREECTRL_H

#include <kiqt.h>

#include <memory>
#include <unordered_map>

#include <QTreeWidget>

#include <wx/kiwx_gui.h>
#include <wx/kiwx_listctrl.h>


// The styles of wxTreeCtrl
#define wxTR_NO_BUTTONS          0x0000
#define wxTR_HAS_BUTTONS         0x0001
#define wxTR_NO_LINES            0x0004
#define wxTR_LINES_AT_ROOT       0x0008
#define wxTR_TWIST_BUTTONS       0x0010
#define wxTR_SINGLE              0x0000
#define wxTR_MULTIPLE            0x0020
#define wxTR_HAS_VARIABLE_ROW_HEIGHT 0x0080
#define wxTR_EDIT_LABELS         0x0200
#define wxTR_ROW_LINES           0x0400
#define wxTR_HIDE_ROOT           0x0800
#define wxTR_FULL_ROW_HIGHLIGHT  0x2000
#define wxTR_DEFAULT_STYLE       wxTR_HAS_BUTTONS

// The results of wxTreeCtrl::HitTest()
#define wxTREE_HITTEST_ABOVE           0x0001
#define wxTREE_HITTEST_BELOW           0x0002
#define wxTREE_HITTEST_NOWHERE         0x0004
#define wxTREE_HITTEST_ONITEMBUTTON    0x0008
#define wxTREE_HITTEST_ONITEMICON      0x0010
#define wxTREE_HITTEST_ONITEMINDENT    0x0020
#define wxTREE_HITTEST_ONITEMLABEL     0x0040
#define wxTREE_HITTEST_ONITEMRIGHT     0x0080
#define wxTREE_HITTEST_ONITEMSTATEICON 0x0100
#define wxTREE_HITTEST_TOLEFT          0x0200
#define wxTREE_HITTEST_TORIGHT         0x0400
#define wxTREE_HITTEST_ONITEMUPPERPART 0x0800
#define wxTREE_HITTEST_ONITEMLOWERPART 0x1000
#define wxTREE_HITTEST_ONITEM ( wxTREE_HITTEST_ONITEMICON | wxTREE_HITTEST_ONITEMLABEL )


/// An item of a tree (wxTreeItemId): its QTreeWidgetItem.
class wxTreeItemId
{
public:
    wxTreeItemId() = default;
    wxTreeItemId( void* aId ) : m_id( aId ) {}

    bool IsOk() const { return m_id != nullptr; }
    void* GetID() const { return m_id; }
    void Unset() { m_id = nullptr; }
    operator const void*() const { return m_id; }

    bool operator==( const wxTreeItemId& aOther ) const { return m_id == aOther.m_id; }
    bool operator!=( const wxTreeItemId& aOther ) const { return m_id != aOther.m_id; }
    bool operator==( const void* aId ) const { return m_id == aId; }
    bool operator!=( const void* aId ) const { return m_id != aId; }

private:
    void* m_id = nullptr;
};

/// The position in the children of an item when they are enumerated (wxTreeItemIdValue).
using wxTreeItemIdValue = void*;


/// The data of an item, owned by the tree (wxTreeItemData).
class wxTreeItemData : public wxClientData
{
public:
    wxTreeItemData() = default;
    ~wxTreeItemData() override = default;

    const wxTreeItemId& GetId() const { return m_itemId; }
    void SetId( const wxTreeItemId& aId ) { m_itemId = aId; }

private:
    wxTreeItemId m_itemId;
};


/// An event of a tree (wxTreeEvent).
class KIQT_API wxTreeEvent : public wxNotifyEvent
{
public:
    explicit wxTreeEvent( wxEventType aType = wxEVT_NULL, int aId = 0,
                          const wxTreeItemId& aItem = wxTreeItemId() ) :
            wxNotifyEvent( aType, aId ),
            m_item( aItem )
    {
    }

    wxTreeItemId GetItem() const { return m_item; }
    void SetItem( const wxTreeItemId& aItem ) { m_item = aItem; }
    wxTreeItemId GetOldItem() const { return m_oldItem; }
    void SetOldItem( const wxTreeItemId& aItem ) { m_oldItem = aItem; }
    wxPoint GetPoint() const { return m_point; }
    void SetPoint( const wxPoint& aPoint ) { m_point = aPoint; }
    const wxString& GetLabel() const { return m_label; }
    void SetLabel( const wxString& aLabel ) { m_label = aLabel; }
    bool IsEditCancelled() const { return m_editCancelled; }
    void SetEditCanceled( bool aCancelled ) { m_editCancelled = aCancelled; }
    int GetKeyCode() const { return m_keyCode; }
    void SetKeyCode( int aKeyCode ) { m_keyCode = aKeyCode; }

private:
    wxTreeItemId m_item;
    wxTreeItemId m_oldItem;
    wxPoint      m_point;
    wxString     m_label;
    bool         m_editCancelled = false;
    int          m_keyCode = 0;
};

#define wxTreeEventHandler( func ) ( &func )


/**
 * A tree (wxTreeCtrl) on a QTreeWidget, with one column.  With wxTR_HIDE_ROOT the root is the
 * invisible root item of Qt.  The events: wxEVT_TREE_SEL_CHANGING/CHANGED (the current item
 * changes), wxEVT_TREE_ITEM_ACTIVATED, wxEVT_TREE_ITEM_RIGHT_CLICK, wxEVT_TREE_ITEM_MENU,
 * wxEVT_TREE_ITEM_EXPANDED/COLLAPSED.
 */
class KIQT_API wxTreeCtrl : public wxWindowCompat<QTreeWidget>
{
public:
    explicit wxTreeCtrl( wxWidgetArgCompat aParent = nullptr, int aId = wxID_ANY,
                         const wxPoint& aPos = wxDefaultPosition, const wxSize& aSize = wxDefaultSize,
                         long aStyle = wxTR_DEFAULT_STYLE,
                         const wxValidator& aValidator = wxDefaultValidator,
                         const wxString& aName = wxEmptyString );
    ~wxTreeCtrl() override;

    // The items

    wxTreeItemId AddRoot( const wxString& aText, int aImage = -1, int aSelImage = -1,
                          wxTreeItemData* aData = nullptr );
    wxTreeItemId AppendItem( const wxTreeItemId& aParent, const wxString& aText, int aImage = -1,
                             int aSelImage = -1, wxTreeItemData* aData = nullptr );
    wxTreeItemId PrependItem( const wxTreeItemId& aParent, const wxString& aText, int aImage = -1,
                              int aSelImage = -1, wxTreeItemData* aData = nullptr );

    /// Insert an item after @a aPrevious (first if @a aPrevious isn't valid).
    wxTreeItemId InsertItem( const wxTreeItemId& aParent, const wxTreeItemId& aPrevious,
                             const wxString& aText, int aImage = -1, int aSelImage = -1,
                             wxTreeItemData* aData = nullptr );
    wxTreeItemId InsertItem( const wxTreeItemId& aParent, size_t aBefore, const wxString& aText,
                             int aImage = -1, int aSelImage = -1, wxTreeItemData* aData = nullptr );

    void Delete( const wxTreeItemId& aItem );
    void DeleteChildren( const wxTreeItemId& aItem );
    void DeleteAllItems();

    wxTreeItemId GetRootItem() const;
    size_t GetCount() const;
    bool IsEmpty() const { return !GetRootItem().IsOk(); }

    wxString GetItemText( const wxTreeItemId& aItem ) const;
    void SetItemText( const wxTreeItemId& aItem, const wxString& aText );

    wxTreeItemData* GetItemData( const wxTreeItemId& aItem ) const;
    void SetItemData( const wxTreeItemId& aItem, wxTreeItemData* aData );

    void SetItemImage( const wxTreeItemId& aItem, int aImage, int aWhich = 0 );
    void SetItemBold( const wxTreeItemId& aItem, bool aBold = true );
    void SetItemTextColour( const wxTreeItemId& aItem, const wxColour& aColour );
    void SetItemBackgroundColour( const wxTreeItemId& aItem, const wxColour& aColour );
    void SetItemFont( const wxTreeItemId& aItem, const wxFont& aFont );
    wxFont GetItemFont( const wxTreeItemId& aItem ) const;
    void SetItemHasChildren( const wxTreeItemId& aItem, bool aHasChildren = true );

    // The navigation

    wxTreeItemId GetItemParent( const wxTreeItemId& aItem ) const;
    wxTreeItemId GetFirstChild( const wxTreeItemId& aItem, wxTreeItemIdValue& aCookie ) const;
    wxTreeItemId GetNextChild( const wxTreeItemId& aItem, wxTreeItemIdValue& aCookie ) const;
    wxTreeItemId GetLastChild( const wxTreeItemId& aItem ) const;
    wxTreeItemId GetNextSibling( const wxTreeItemId& aItem ) const;
    wxTreeItemId GetPrevSibling( const wxTreeItemId& aItem ) const;
    wxTreeItemId GetFirstVisibleItem() const;
    wxTreeItemId GetNextVisible( const wxTreeItemId& aItem ) const;
    size_t GetChildrenCount( const wxTreeItemId& aItem, bool aRecursively = true ) const;
    bool ItemHasChildren( const wxTreeItemId& aItem ) const;

    // The selection and the state of the items

    wxTreeItemId GetSelection() const;
    size_t GetSelections( std::vector<wxTreeItemId>& aSelections ) const;
    wxTreeItemId GetFocusedItem() const { return GetSelection(); }

    /// The current item, without changing the selection.
    void SetFocusedItem( const wxTreeItemId& aItem );
    void SelectItem( const wxTreeItemId& aItem, bool aSelect = true );
    void Unselect();
    void UnselectAll();
    bool IsSelected( const wxTreeItemId& aItem ) const;

    void Expand( const wxTreeItemId& aItem );
    void Collapse( const wxTreeItemId& aItem );
    void Toggle( const wxTreeItemId& aItem );
    void ExpandAll() { expandAll(); }
    void ExpandAllChildren( const wxTreeItemId& aItem );
    void CollapseAll() { collapseAll(); }
    bool IsExpanded( const wxTreeItemId& aItem ) const;
    bool IsVisible( const wxTreeItemId& aItem ) const;
    void EnsureVisible( const wxTreeItemId& aItem );
    void ScrollTo( const wxTreeItemId& aItem ) { EnsureVisible( aItem ); }

    /// The item at a point of the tree (its client coordinates).
    wxTreeItemId HitTest( const wxPoint& aPoint, int& aFlags ) const;
    wxTreeItemId HitTest( const wxPoint& aPoint ) const
    {
        int flags = 0;
        return HitTest( aPoint, flags );
    }

    bool GetBoundingRect( const wxTreeItemId& aItem, wxRect& aRect, bool aTextOnly = false ) const;

    void SetImageList( wxImageList* aImageList );
    void AssignImageList( wxImageList* aImageList );
    wxImageList* GetImageList() const { return m_imageList; }

    /// The images of the items, as bundles (in place of an image list).
    void SetImages( const std::vector<wxBitmapBundle>& aImages ) { m_images = aImages; }

    /// Sort the children of an item with OnCompareItems().
    void SortChildren( const wxTreeItemId& aItem );

    /// The order of two items of SortChildren(): their texts by default.
    virtual int OnCompareItems( const wxTreeItemId& aItem1, const wxTreeItemId& aItem2 );

    /// Edit the text of an item in the tree.
    void EditLabel( const wxTreeItemId& aItem );

    /// The editor of the text of an item while it is edited (else nullptr).
    QWidget* GetEditControl() const;

    /// End the edition of the text of an item (and keep the text unless @a aDiscard).
    void EndEditLabel( const wxTreeItemId& aItem, bool aDiscard = false );

    void SetIndent( unsigned int aIndent ) { setIndentation( int( aIndent ) ); }
    unsigned int GetIndent() const { return unsigned( indentation() ); }

    /// The item of a QTreeWidgetItem, and the reverse.
    static wxTreeItemId ToItemCompat( QTreeWidgetItem* aItem ) { return wxTreeItemId( aItem ); }
    QTreeWidgetItem* ToQtCompat( const wxTreeItemId& aItem ) const;

private:
    QTreeWidgetItem* newItem( const wxString& aText, int aImage, wxTreeItemData* aData );

    /// The icon of an image of the image list or of the bundles.
    QIcon imageCompat( int aImage ) const;

    /// The item is deleted: forget the data of the item and of its children.
    void forgetData( QTreeWidgetItem* aItem );

    void sendEvent( wxEventType aType, QTreeWidgetItem* aItem, QTreeWidgetItem* aOld = nullptr,
                    const QPoint& aPos = QPoint() );

    long                                                               m_style;
    QTreeWidgetItem*                                                   m_root;
    std::unordered_map<QTreeWidgetItem*, std::unique_ptr<wxTreeItemData>> m_data;
    wxImageList*                                                       m_imageList = nullptr;
    std::vector<wxBitmapBundle>                                        m_images;
    std::unique_ptr<wxImageList>                                       m_ownedImageList;
};

#endif // KIWX_TREECTRL_H
