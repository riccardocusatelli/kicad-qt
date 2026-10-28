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
 * @file kiwx_listctrl.h
 * The list controls of the wxWidgets compatibility layer of the Qt build (wxcompat): wxListCtrl
 * and wxListView (the report view: rows and columns) on a QTreeView, with the virtual lists
 * (wxLC_VIRTUAL: OnGetItemText()), the check boxes and the image lists.
 */

#ifndef KIWX_LISTCTRL_H
#define KIWX_LISTCTRL_H

#include <kiqt.h>

#include <vector>

#include <QTreeView>

#include <wx/kiwx_gui.h>

class KIWX_LIST_MODEL;

#define wxLC_VRULES 0x0001
#define wxLC_HRULES 0x0002
#define wxLC_ICON 0x0004
#define wxLC_SMALL_ICON 0x0008
#define wxLC_LIST 0x0010
#define wxLC_REPORT 0x0020
#define wxLC_ALIGN_TOP 0x0040
#define wxLC_ALIGN_LEFT 0x0080
#define wxLC_AUTOARRANGE 0x0100
#define wxLC_VIRTUAL 0x0200
#define wxLC_EDIT_LABELS 0x0400
#define wxLC_NO_HEADER 0x0800
#define wxLC_NO_SORT_HEADER 0x1000
#define wxLC_SINGLE_SEL 0x2000
#define wxLC_SORT_ASCENDING 0x4000
#define wxLC_SORT_DESCENDING 0x8000

#define wxLIST_MASK_STATE 0x0001
#define wxLIST_MASK_TEXT 0x0002
#define wxLIST_MASK_IMAGE 0x0004
#define wxLIST_MASK_DATA 0x0008
#define wxLIST_MASK_WIDTH 0x0010
#define wxLIST_MASK_FORMAT 0x0020

#define wxLIST_STATE_DONTCARE 0x0000
#define wxLIST_STATE_DROPHILITED 0x0001
#define wxLIST_STATE_FOCUSED 0x0002
#define wxLIST_STATE_SELECTED 0x0004
#define wxLIST_STATE_CUT 0x0008

#define wxLIST_NEXT_ABOVE 0
#define wxLIST_NEXT_ALL 1
#define wxLIST_NEXT_BELOW 2
#define wxLIST_NEXT_LEFT 3
#define wxLIST_NEXT_RIGHT 4

#define wxLIST_AUTOSIZE -1
#define wxLIST_AUTOSIZE_USEHEADER -2

#define wxLIST_HITTEST_ABOVE 0x0001
#define wxLIST_HITTEST_BELOW 0x0002
#define wxLIST_HITTEST_NOWHERE 0x0004
#define wxLIST_HITTEST_ONITEMICON 0x0020
#define wxLIST_HITTEST_ONITEMLABEL 0x0080
#define wxLIST_HITTEST_ONITEMRIGHT 0x0100
#define wxLIST_HITTEST_ONITEMSTATEICON 0x0200
#define wxLIST_HITTEST_TOLEFT 0x0400
#define wxLIST_HITTEST_TORIGHT 0x0800
#define wxLIST_HITTEST_ONITEM ( wxLIST_HITTEST_ONITEMICON | wxLIST_HITTEST_ONITEMLABEL \
                                | wxLIST_HITTEST_ONITEMSTATEICON )

#define wxLIST_FIND_UP 0
#define wxLIST_FIND_DOWN 1

enum wxListColumnFormat
{
    wxLIST_FORMAT_LEFT,
    wxLIST_FORMAT_RIGHT,
    wxLIST_FORMAT_CENTRE,
    wxLIST_FORMAT_CENTER = wxLIST_FORMAT_CENTRE
};

#define wxIMAGE_LIST_NORMAL 0
#define wxIMAGE_LIST_SMALL 1
#define wxIMAGE_LIST_STATE 2


/// A list of images of the same size (wxImageList).
class KIQT_API wxImageList
{
public:
    wxImageList() = default;
    wxImageList( int aWidth, int aHeight, bool aMask = true, int aInitialCount = 1 ) :
            m_size( aWidth, aHeight )
    {
    }

    int Add( const wxBitmap& aBitmap );
    int Add( const wxBitmap& aBitmap, const wxBitmap& aMask ) { return Add( aBitmap ); }
    int Add( const wxBitmap& aBitmap, const wxColour& aMaskColour ) { return Add( aBitmap ); }
    bool Replace( int aIndex, const wxBitmap& aBitmap );
    bool Remove( int aIndex );
    bool RemoveAll() { m_images.clear(); return true; }
    int GetImageCount() const { return int( m_images.size() ); }
    wxBitmap GetBitmap( int aIndex ) const;
    wxSize GetSize() const { return m_size; }
    bool GetSize( int aIndex, int& aWidth, int& aHeight ) const;

private:
    wxSize                m_size;
    std::vector<wxBitmap> m_images;
};


/// The colours and the font of an item of a virtual list (wxListCtrl::OnGetItemAttr()).
class wxListItemAttr
{
public:
    wxListItemAttr() = default;
    wxListItemAttr( const wxColour& aText, const wxColour& aBackground, const wxFont& aFont ) :
            m_text( aText ),
            m_background( aBackground ),
            m_font( aFont )
    {
    }

    void SetTextColour( const wxColour& aColour ) { m_text = aColour; }
    void SetBackgroundColour( const wxColour& aColour ) { m_background = aColour; }
    void SetFont( const wxFont& aFont ) { m_font = aFont; }

    bool HasTextColour() const { return m_text.IsOk(); }
    bool HasBackgroundColour() const { return m_background.IsOk(); }
    bool HasFont() const { return m_font.IsOk(); }

    const wxColour& GetTextColour() const { return m_text; }
    const wxColour& GetBackgroundColour() const { return m_background; }
    const wxFont& GetFont() const { return m_font; }

private:
    wxColour m_text;
    wxColour m_background;
    wxFont   m_font;
};


/// The comparison of the data of two items of wxListCtrl::SortItems().
typedef int ( *wxListCtrlCompare )( wxIntPtr aItem1, wxIntPtr aItem2, wxIntPtr aSortData );


/// An item (or a column) of a list (wxListItem).
class KIQT_API wxListItem
{
public:
    wxListItem() = default;

    void Clear() { *this = wxListItem(); }

    void SetId( long aId ) { m_itemId = aId; }
    long GetId() const { return m_itemId; }
    void SetColumn( int aColumn ) { m_col = aColumn; }
    int GetColumn() const { return m_col; }
    void SetMask( long aMask ) { m_mask = aMask; }
    long GetMask() const { return m_mask; }

    void SetText( const wxString& aText ) { m_text = aText; m_mask |= wxLIST_MASK_TEXT; }
    const wxString& GetText() const { return m_text; }
    void SetImage( int aImage ) { m_image = aImage; m_mask |= wxLIST_MASK_IMAGE; }
    int GetImage() const { return m_image; }
    void SetData( long aData ) { m_data = wxUIntPtr( aData ); m_mask |= wxLIST_MASK_DATA; }
    void SetData( void* aData ) { m_data = wxUIntPtr( aData ); m_mask |= wxLIST_MASK_DATA; }
    wxUIntPtr GetData() const { return m_data; }
    void SetWidth( int aWidth ) { m_width = aWidth; m_mask |= wxLIST_MASK_WIDTH; }
    int GetWidth() const { return m_width; }
    void SetAlign( wxListColumnFormat aAlign ) { m_format = aAlign; m_mask |= wxLIST_MASK_FORMAT; }
    wxListColumnFormat GetAlign() const { return m_format; }

    void SetState( long aState ) { m_state = aState; m_stateMask |= aState; m_mask |= wxLIST_MASK_STATE; }
    void SetStateMask( long aMask ) { m_stateMask = aMask; }
    long GetState() const { return m_state; }

    void SetFont( const wxFont& aFont ) { m_font = aFont; }
    wxFont GetFont() const { return m_font; }
    void SetTextColour( const wxColour& aColour ) { m_textColour = aColour; }
    wxColour GetTextColour() const { return m_textColour; }
    void SetBackgroundColour( const wxColour& aColour ) { m_backgroundColour = aColour; }
    wxColour GetBackgroundColour() const { return m_backgroundColour; }

    // Public as the ones of wxWidgets
    long               m_itemId = 0;
    int                m_col = 0;
    long               m_mask = 0;
    wxString           m_text;
    int                m_image = -1;
    wxUIntPtr          m_data = 0;
    int                m_width = -1;
    wxListColumnFormat m_format = wxLIST_FORMAT_LEFT;
    long               m_state = 0;
    long               m_stateMask = 0;
    wxFont             m_font;
    wxColour           m_textColour;
    wxColour           m_backgroundColour;
};


/// An event of a list (wxListEvent).
class KIQT_API wxListEvent : public wxNotifyEvent
{
public:
    explicit wxListEvent( wxEventType aType = wxEVT_NULL, int aId = 0 ) :
            wxNotifyEvent( aType, aId )
    {
    }

    long GetIndex() const { return m_itemIndex; }
    int GetColumn() const { return m_col; }
    const wxListItem& GetItem() const { return m_item; }
    wxString GetLabel() const { return m_item.GetText(); }
    wxString GetText() const { return m_item.GetText(); }
    wxUIntPtr GetData() const { return m_item.GetData(); }
    int GetImage() const { return m_item.GetImage(); }
    int GetKeyCode() const { return m_code; }
    wxPoint GetPoint() const { return m_pointDrag; }
    bool IsEditCancelled() const { return false; }

    // The ones of the controls of the layer
    void SetItemCompat( const wxListItem& aItem )
    {
        m_item = aItem;
        m_itemIndex = aItem.GetId();
    }

    void SetColumnCompat( int aColumn ) { m_col = aColumn; }
    void SetKeyCodeCompat( int aKeyCode ) { m_code = aKeyCode; }
    void SetPointCompat( const wxPoint& aPoint ) { m_pointDrag = aPoint; }

    // The data of the event (public in wxWidgets)
    int        m_code = 0;
    long       m_oldItemIndex = -1;
    long       m_itemIndex = -1;
    int        m_col = -1;
    wxPoint    m_pointDrag;
    wxListItem m_item;
};

#define wxListEventHandler( func ) ( &func )


/// A list control (wxListCtrl): the report view of wxWidgets.
class KIQT_API wxListCtrl : public wxWindowCompat<QTreeView>
{
public:
    explicit wxListCtrl( wxWidgetArgCompat aParent = nullptr, int aId = wxID_ANY,
                         const wxPoint& aPos = wxDefaultPosition, const wxSize& aSize = wxDefaultSize,
                         long aStyle = wxLC_REPORT, const wxValidator& aValidator = wxDefaultValidator,
                         const wxString& aName = wxEmptyString );
    ~wxListCtrl() override;

    // The columns

    long InsertColumn( long aCol, const wxString& aHeading, int aFormat = wxLIST_FORMAT_LEFT,
                       int aWidth = wxLIST_AUTOSIZE );
    long InsertColumn( long aCol, const wxListItem& aInfo );
    long AppendColumn( const wxString& aHeading, wxListColumnFormat aFormat = wxLIST_FORMAT_LEFT,
                       int aWidth = -1 );
    bool DeleteColumn( int aCol );
    bool DeleteAllColumns();
    int GetColumnCount() const;
    bool GetColumn( int aCol, wxListItem& aItem ) const;
    bool SetColumn( int aCol, const wxListItem& aItem );
    bool SetColumnWidth( int aCol, int aWidth );
    int GetColumnWidth( int aCol ) const;

    // The items

    long InsertItem( long aIndex, const wxString& aLabel, int aImageIndex = -1 );
    long InsertItem( long aIndex, int aImageIndex ) { return InsertItem( aIndex, wxEmptyString, aImageIndex ); }
    long InsertItem( const wxListItem& aInfo );
    bool SetItem( const wxListItem& aInfo );
    long SetItem( long aIndex, int aCol, const wxString& aLabel, int aImageId = -1 );
    bool GetItem( wxListItem& aInfo ) const;
    wxString GetItemText( long aItem, int aCol = 0 ) const;
    void SetItemText( long aItem, const wxString& aText ) { SetItem( aItem, 0, aText ); }
    bool SetItemData( long aItem, long aData );
    bool SetItemPtrData( long aItem, wxUIntPtr aData );
    wxUIntPtr GetItemData( long aItem ) const;
    bool SetItemImage( long aItem, int aImage, int aSelImage = -1 );
    bool SetItemColumnImage( long aItem, long aColumn, int aImage );
    void SetItemFont( long aItem, const wxFont& aFont );
    wxFont GetItemFont( long aItem ) const;
    void SetItemTextColour( long aItem, const wxColour& aColour );
    wxColour GetItemTextColour( long aItem ) const;
    void SetItemBackgroundColour( long aItem, const wxColour& aColour );
    bool DeleteItem( long aItem );
    bool DeleteAllItems();
    void ClearAll();
    int GetItemCount() const;
    bool IsEmpty() const { return GetItemCount() == 0; }

    /// The number of the items of a virtual list (wxLC_VIRTUAL).
    void SetItemCount( long aCount );
    void RefreshItem( long aItem );
    void RefreshItems( long aFrom, long aTo );

    // The selection and the states

    long GetNextItem( long aItem, int aGeometry = wxLIST_NEXT_ALL,
                      int aState = wxLIST_STATE_DONTCARE ) const;
    int GetItemState( long aItem, long aStateMask ) const;
    bool SetItemState( long aItem, long aState, long aStateMask );
    int GetSelectedItemCount() const;
    bool EnsureVisible( long aItem );
    long FindItem( long aStart, const wxString& aText, bool aPartial = false ) const;
    long FindItem( long aStart, wxUIntPtr aData ) const;
    long GetTopItem() const;
    int GetCountPerPage() const;
    bool GetItemRect( long aItem, wxRect& aRect, int aCode = 0 ) const;
    long HitTest( const wxPoint& aPoint, int& aFlags, long* aSubItem = nullptr ) const;

    // The check boxes

    bool EnableCheckBoxes( bool aEnable = true );
    bool HasCheckBoxes() const { return m_checkBoxes; }
    void CheckItem( long aItem, bool aCheck = true );
    bool IsItemChecked( long aItem ) const;

    // The images

    void SetImageList( wxImageList* aImageList, int aWhich );
    void AssignImageList( wxImageList* aImageList, int aWhich );
    wxImageList* GetImageList( int aWhich ) const;
    void SetNormalImages( const std::vector<wxBitmapBundle>& aImages );
    void SetSmallImages( const std::vector<wxBitmapBundle>& aImages );

    void SetSingleStyle( long aStyle, bool aAdd = true );
    void SetWindowStyleFlag( long aStyle ) override;

    bool IsVirtual() const { return HasFlag( wxLC_VIRTUAL ); }

    /// Sort the items with the comparison of their data (the item data of SetItemData()).
    bool SortItems( wxListCtrlCompare aCompare, wxIntPtr aSortData );

    /// The texts and images of a virtual list: override them.
    virtual wxString OnGetItemText( long aItem, long aColumn ) const { return wxString(); }
    virtual int OnGetItemImage( long aItem ) const { return -1; }
    virtual wxListItemAttr* OnGetItemAttr( long aItem ) const { return nullptr; }
    virtual int OnGetItemColumnImage( long aItem, long aColumn ) const
    {
        return aColumn == 0 ? OnGetItemImage( aItem ) : -1;
    }

    /// The image of an index of the image list of the items (or of the images of the list).
    wxBitmap ImageCompat( int aIndex ) const;

protected:
    void keyPressEvent( QKeyEvent* aEvent ) override;

private:
    friend class KIWX_LIST_MODEL;

    void sendEvent( wxEventType aType, long aItem, int aColumn = -1 );
    void applyStyle();

    KIWX_LIST_MODEL*            m_model;
    bool                        m_checkBoxes;
    wxImageList*                m_imageList;
    bool                        m_ownImageList;
    std::vector<wxBitmapBundle> m_images;
};


/// A list control with the API of selections (wxListView).
class KIQT_API wxListView : public wxListCtrl
{
public:
    using wxListCtrl::wxListCtrl;

    void Select( long aItem, bool aSelect = true );
    void Focus( long aItem );
    long GetFocusedItem() const;
    long GetFirstSelected() const { return GetNextSelected( -1 ); }
    long GetNextSelected( long aItem ) const { return GetNextItem( aItem, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED ); }
    bool IsSelected( long aItem ) const { return GetItemState( aItem, wxLIST_STATE_SELECTED ) != 0; }
    void SetColumnImage( int aCol, int aImage ) {}
    void ClearColumnImage( int aCol ) {}
};

#endif // KIWX_LISTCTRL_H
