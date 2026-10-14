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
 * @file kiwx_dataview.h
 * The data view controls of the wxWidgets compatibility layer of the Qt build (wxcompat):
 * wxDataViewCtrl on a QTreeView, its model of wxWidgets (wxDataViewModel: a tree or a list of
 * items) seen by Qt through an adapter, the columns and their renderers (text, toggle, icon and
 * text, bitmap, custom), and wxDataViewListCtrl with its store.
 */

#ifndef KIWX_DATAVIEW_H
#define KIWX_DATAVIEW_H

#include <kiqt.h>

#include <vector>

#include <QTreeView>

#include <wx/variant.h>
#include <wx/vector.h>
#include <wx/kiwx_gui.h>

class KIWX_DATAVIEW_MODEL;
class wxDataViewCtrl;
class wxDataViewColumn;
class wxDataViewModel;


enum wxDataViewCellMode
{
    wxDATAVIEW_CELL_INERT,
    wxDATAVIEW_CELL_ACTIVATABLE,
    wxDATAVIEW_CELL_EDITABLE
};

enum wxDataViewCellRenderState
{
    wxDATAVIEW_CELL_SELECTED = 1,
    wxDATAVIEW_CELL_PRELIT = 2,
    wxDATAVIEW_CELL_INSENSITIVE = 4,
    wxDATAVIEW_CELL_FOCUSED = 8
};

enum wxDataViewColumnFlags
{
    wxDATAVIEW_COL_RESIZABLE = 1,
    wxDATAVIEW_COL_SORTABLE = 2,
    wxDATAVIEW_COL_REORDERABLE = 4,
    wxDATAVIEW_COL_HIDDEN = 8
};

#define wxDVC_DEFAULT_WIDTH 80
#define wxDVC_TOGGLE_DEFAULT_WIDTH 30
#define wxDVR_DEFAULT_ALIGNMENT -1
#define wxDVC_DEFAULT_MINWIDTH 30

#define wxDV_SINGLE 0x0000
#define wxDV_MULTIPLE 0x0001
#define wxDV_NO_HEADER 0x0002
#define wxDV_HORIZ_RULES 0x0004
#define wxDV_VERT_RULES 0x0008
#define wxDV_ROW_LINES 0x0010
#define wxDV_VARIABLE_LINE_HEIGHT 0x0020

#define wxDATAVIEW_COL_WIDTH_AUTOSIZE -1
#define wxCOL_WIDTH_AUTOSIZE -1
#define wxCOL_WIDTH_DEFAULT -1


/// An item of a model (wxDataViewItem): an opaque id, null for the root.
class wxDataViewItem
{
public:
    wxDataViewItem() = default;
    explicit wxDataViewItem( void* aId ) : m_id( aId ) {}

    bool IsOk() const { return m_id != nullptr; }
    void* GetID() const { return m_id; }
    void Unset() { m_id = nullptr; }
    operator const void*() const { return m_id; }

    bool operator==( const wxDataViewItem& aOther ) const { return m_id == aOther.m_id; }
    bool operator!=( const wxDataViewItem& aOther ) const { return m_id != aOther.m_id; }
    bool operator<( const wxDataViewItem& aOther ) const { return m_id < aOther.m_id; }

private:
    void* m_id = nullptr;
};


/// An array of items (wxDataViewItemArray).
class wxDataViewItemArray : public std::vector<wxDataViewItem>
{
public:
    using std::vector<wxDataViewItem>::vector;

    size_t GetCount() const { return size(); }
    size_t Count() const { return size(); }
    bool IsEmpty() const { return empty(); }
    void Add( const wxDataViewItem& aItem ) { push_back( aItem ); }
    void Clear() { clear(); }
    void Empty() { clear(); }
    void Alloc( size_t aCount ) { reserve( aCount ); }
    wxDataViewItem& Item( size_t aIndex ) { return ( *this )[aIndex]; }
    const wxDataViewItem& Item( size_t aIndex ) const { return ( *this )[aIndex]; }
    wxDataViewItem& Last() { return back(); }

    int Index( const wxDataViewItem& aItem ) const
    {
        for( size_t i = 0; i < size(); ++i )
        {
            if( ( *this )[i] == aItem )
                return int( i );
        }

        return wxNOT_FOUND;
    }

    void Remove( const wxDataViewItem& aItem )
    {
        int i = Index( aItem );

        if( i != wxNOT_FOUND )
            erase( begin() + i );
    }
};


/// The attributes of a cell (wxDataViewItemAttr).
class KIQT_API wxDataViewItemAttr
{
public:
    void SetColour( const wxColour& aColour ) { m_colour = aColour; }
    void SetBold( bool aBold ) { m_bold = aBold; }
    void SetItalic( bool aItalic ) { m_italic = aItalic; }
    void SetStrikethrough( bool aStrike ) { m_strikethrough = aStrike; }
    void SetBackgroundColour( const wxColour& aColour ) { m_background = aColour; }

    bool HasColour() const { return m_colour.IsOk(); }
    const wxColour& GetColour() const { return m_colour; }
    bool HasFont() const { return m_bold || m_italic || m_strikethrough; }
    bool GetBold() const { return m_bold; }
    bool GetItalic() const { return m_italic; }
    bool GetStrikethrough() const { return m_strikethrough; }
    bool HasBackgroundColour() const { return m_background.IsOk(); }
    const wxColour& GetBackgroundColour() const { return m_background; }
    bool IsDefault() const { return !( HasColour() || HasFont() || HasBackgroundColour() ); }

    /// The font with the attributes.
    wxFont GetEffectiveFont( const wxFont& aFont ) const;

private:
    wxColour m_colour;
    wxColour m_background;
    bool     m_bold = false;
    bool     m_italic = false;
    bool     m_strikethrough = false;
};


/// A text with an icon (wxDataViewIconText), a value of the variants.
class KIQT_API wxDataViewIconText : public wxObject
{
public:
    wxDataViewIconText( const wxString& aText = wxEmptyString,
                        const wxBitmapBundle& aIcon = wxBitmapBundle() ) :
            m_text( aText ),
            m_icon( aIcon )
    {
    }

    void SetText( const wxString& aText ) { m_text = aText; }
    wxString GetText() const { return m_text; }
    void SetIcon( const wxBitmapBundle& aIcon ) { m_icon = aIcon; }
    const wxBitmapBundle& GetBitmapBundle() const { return m_icon; }
    wxBitmap GetIcon() const;

    bool operator==( const wxDataViewIconText& aOther ) const { return m_text == aOther.m_text; }
    bool operator!=( const wxDataViewIconText& aOther ) const { return !( *this == aOther ); }

private:
    wxString       m_text;
    wxBitmapBundle m_icon;
};

KIQT_API wxDataViewIconText& operator<<( wxDataViewIconText& aValue, const wxVariant& aVariant );
KIQT_API wxVariant& operator<<( wxVariant& aVariant, const wxDataViewIconText& aValue );


/// The listener of the changes of a model (wxDataViewModelNotifier).
class KIQT_API wxDataViewModelNotifier
{
public:
    wxDataViewModelNotifier() = default;
    virtual ~wxDataViewModelNotifier() = default;

    virtual bool ItemAdded( const wxDataViewItem& aParent, const wxDataViewItem& aItem ) = 0;
    virtual bool ItemDeleted( const wxDataViewItem& aParent, const wxDataViewItem& aItem ) = 0;
    virtual bool ItemChanged( const wxDataViewItem& aItem ) = 0;
    virtual bool ItemsAdded( const wxDataViewItem& aParent, const wxDataViewItemArray& aItems );
    virtual bool ItemsDeleted( const wxDataViewItem& aParent, const wxDataViewItemArray& aItems );
    virtual bool ItemsChanged( const wxDataViewItemArray& aItems );
    virtual bool ValueChanged( const wxDataViewItem& aItem, unsigned int aCol ) = 0;
    virtual bool Cleared() = 0;
    virtual bool BeforeReset() { return true; }
    virtual bool AfterReset() { return Cleared(); }
    virtual void Resort() = 0;

    void SetOwner( wxDataViewModel* aOwner ) { m_owner = aOwner; }
    wxDataViewModel* GetOwner() const { return m_owner; }

private:
    wxDataViewModel* m_owner = nullptr;
};


/// The model of a data view (wxDataViewModel): a tree of items, reference counted.
class KIQT_API wxDataViewModel : public wxRefCounter
{
public:
    wxDataViewModel() = default;

    virtual void GetValue( wxVariant& aVariant, const wxDataViewItem& aItem,
                           unsigned int aCol ) const = 0;
    virtual bool HasValue( const wxDataViewItem& aItem, unsigned aCol ) const
    {
        return aCol == 0 || !IsContainer( aItem ) || HasContainerColumns( aItem );
    }

    virtual bool SetValue( const wxVariant& aVariant, const wxDataViewItem& aItem,
                           unsigned int aCol ) = 0;

    bool ChangeValue( const wxVariant& aVariant, const wxDataViewItem& aItem, unsigned int aCol )
    {
        return SetValue( aVariant, aItem, aCol ) && ValueChanged( aItem, aCol );
    }

    virtual bool GetAttr( const wxDataViewItem& aItem, unsigned int aCol,
                          wxDataViewItemAttr& aAttr ) const
    {
        return false;
    }

    virtual bool IsEnabled( const wxDataViewItem& aItem, unsigned int aCol ) const { return true; }

    virtual wxDataViewItem GetParent( const wxDataViewItem& aItem ) const = 0;
    virtual bool IsContainer( const wxDataViewItem& aItem ) const = 0;
    virtual bool HasContainerColumns( const wxDataViewItem& aItem ) const { return false; }
    virtual unsigned int GetChildren( const wxDataViewItem& aItem,
                                      wxDataViewItemArray& aChildren ) const = 0;

    // The notifications of the changes
    bool ItemAdded( const wxDataViewItem& aParent, const wxDataViewItem& aItem );
    bool ItemsAdded( const wxDataViewItem& aParent, const wxDataViewItemArray& aItems );
    bool ItemDeleted( const wxDataViewItem& aParent, const wxDataViewItem& aItem );
    bool ItemsDeleted( const wxDataViewItem& aParent, const wxDataViewItemArray& aItems );
    bool ItemChanged( const wxDataViewItem& aItem );
    bool ItemsChanged( const wxDataViewItemArray& aItems );
    bool ValueChanged( const wxDataViewItem& aItem, unsigned int aCol );
    bool Cleared();
    bool BeforeReset();
    bool AfterReset();
    virtual void Resort();

    void AddNotifier( wxDataViewModelNotifier* aNotifier );
    void RemoveNotifier( wxDataViewModelNotifier* aNotifier );

    virtual int Compare( const wxDataViewItem& aItem1, const wxDataViewItem& aItem2,
                         unsigned int aColumn, bool aAscending ) const;
    virtual bool HasDefaultCompare() const { return false; }
    virtual bool IsListModel() const { return false; }
    virtual bool IsVirtualListModel() const { return false; }

    virtual unsigned int GetColumnCount() const { return 0; }
    virtual wxString GetColumnType( unsigned int aCol ) const { return wxString(); }

protected:
    ~wxDataViewModel() override;

    virtual int DoCompareValues( const wxVariant& aValue1, const wxVariant& aValue2 ) const
    {
        return 0;
    }

private:
    std::vector<wxDataViewModelNotifier*> m_notifiers;
};


/// A model of a list (wxDataViewListModel): the items are its rows.
class KIQT_API wxDataViewListModel : public wxDataViewModel
{
public:
    virtual void GetValueByRow( wxVariant& aVariant, unsigned aRow, unsigned aCol ) const = 0;
    virtual bool SetValueByRow( const wxVariant& aVariant, unsigned aRow, unsigned aCol ) = 0;
    virtual bool GetAttrByRow( unsigned aRow, unsigned aCol, wxDataViewItemAttr& aAttr ) const
    {
        return false;
    }

    virtual bool IsEnabledByRow( unsigned int aRow, unsigned int aCol ) const { return true; }
    virtual unsigned GetRow( const wxDataViewItem& aItem ) const = 0;
    virtual unsigned int GetCount() const = 0;
    virtual wxDataViewItem GetItem( unsigned int aRow ) const = 0;

    wxDataViewItem GetParent( const wxDataViewItem& aItem ) const override { return wxDataViewItem(); }
    bool IsContainer( const wxDataViewItem& aItem ) const override { return !aItem.IsOk(); }

    void GetValue( wxVariant& aVariant, const wxDataViewItem& aItem, unsigned int aCol ) const override
    {
        GetValueByRow( aVariant, GetRow( aItem ), aCol );
    }

    bool SetValue( const wxVariant& aVariant, const wxDataViewItem& aItem, unsigned int aCol ) override
    {
        return SetValueByRow( aVariant, GetRow( aItem ), aCol );
    }

    bool GetAttr( const wxDataViewItem& aItem, unsigned int aCol,
                  wxDataViewItemAttr& aAttr ) const override
    {
        return GetAttrByRow( GetRow( aItem ), aCol, aAttr );
    }

    bool IsEnabled( const wxDataViewItem& aItem, unsigned int aCol ) const override
    {
        return IsEnabledByRow( GetRow( aItem ), aCol );
    }

    bool IsListModel() const override { return true; }

    void RowPrepended() { RowInserted( 0 ); }
    void RowAppended() { RowInserted( GetCount() - 1 ); }
    void RowInserted( unsigned int aBefore );
    void RowDeleted( unsigned int aRow );
    void RowsDeleted( const wxArrayInt& aRows );
    void RowChanged( unsigned int aRow );
    void RowValueChanged( unsigned int aRow, unsigned int aCol );
    void Reset( unsigned int aNewSize );

protected:
    /// The number of the rows changed (the stores of the rows of the derived models).
    virtual void resizeCompat( unsigned int aNewSize ) {}
};


/// A list model of rows numbered from 0 (wxDataViewIndexListModel).
class KIQT_API wxDataViewIndexListModel : public wxDataViewListModel
{
public:
    explicit wxDataViewIndexListModel( unsigned int aInitialSize = 0 ) : m_count( aInitialSize ) {}

    unsigned GetRow( const wxDataViewItem& aItem ) const override;
    wxDataViewItem GetItem( unsigned int aRow ) const override;
    unsigned int GetChildren( const wxDataViewItem& aItem,
                              wxDataViewItemArray& aChildren ) const override;
    unsigned int GetCount() const override { return m_count; }

protected:
    void resizeCompat( unsigned int aNewSize ) override { m_count = aNewSize; }

    friend class wxDataViewListModel;
    friend class wxDataViewListStore;
    unsigned int m_count;
};


/// A list model with many rows (wxDataViewVirtualListModel).
class KIQT_API wxDataViewVirtualListModel : public wxDataViewIndexListModel
{
public:
    using wxDataViewIndexListModel::wxDataViewIndexListModel;

    bool IsVirtualListModel() const override { return true; }
};


/// The drawing and the edition of the cells of a column (wxDataViewRenderer).
class KIQT_API wxDataViewRenderer : public wxObject
{
public:
    wxDataViewRenderer( const wxString& aVariantType,
                        wxDataViewCellMode aMode = wxDATAVIEW_CELL_INERT,
                        int aAlign = wxDVR_DEFAULT_ALIGNMENT ) :
            m_variantType( aVariantType ),
            m_mode( aMode ),
            m_align( aAlign )
    {
    }

    ~wxDataViewRenderer() override = default;

    virtual bool SetValue( const wxVariant& aValue ) { m_value = aValue; return true; }
    virtual bool GetValue( wxVariant& aValue ) const { aValue = m_value; return true; }

    wxString GetVariantType() const { return m_variantType; }
    wxDataViewCellMode GetMode() const { return m_mode; }
    void SetMode( wxDataViewCellMode aMode ) { m_mode = aMode; }
    int GetAlignment() const { return m_align; }
    void SetAlignment( int aAlign ) { m_align = aAlign; }
    void EnableEllipsize( int aMode = 0 ) {}
    void DisableEllipsize() {}

    virtual void SetAttr( const wxDataViewItemAttr& aAttr ) { m_attr = aAttr; }
    const wxDataViewItemAttr& GetAttr() const { return m_attr; }
    virtual void SetEnabled( bool aEnabled ) { m_enabled = aEnabled; }
    bool GetEnabled() const { return m_enabled; }

    wxDataViewColumn* GetOwner() const { return m_owner; }
    void SetOwner( wxDataViewColumn* aOwner ) { m_owner = aOwner; }

    virtual bool IsCustomRenderer() const { return false; }

    /// The kind of the renderer, for the model of Qt.
    enum KIND_COMPAT { TEXT, TOGGLE, ICONTEXT, BITMAP, CUSTOM };
    virtual KIND_COMPAT KindCompat() const { return TEXT; }

protected:
    wxString           m_variantType;
    wxDataViewCellMode m_mode;
    int                m_align;
    wxVariant          m_value;
    wxDataViewItemAttr m_attr;
    bool               m_enabled = true;
    wxDataViewColumn*  m_owner = nullptr;
};


class KIQT_API wxDataViewTextRenderer : public wxDataViewRenderer
{
public:
    explicit wxDataViewTextRenderer( const wxString& aVariantType = wxString( "string" ),
                                     wxDataViewCellMode aMode = wxDATAVIEW_CELL_INERT,
                                     int aAlign = wxDVR_DEFAULT_ALIGNMENT ) :
            wxDataViewRenderer( aVariantType, aMode, aAlign )
    {
    }

    void EnableMarkup( bool aEnable = true ) {}
};


class KIQT_API wxDataViewToggleRenderer : public wxDataViewRenderer
{
public:
    explicit wxDataViewToggleRenderer( const wxString& aVariantType = wxString( "bool" ),
                                       wxDataViewCellMode aMode = wxDATAVIEW_CELL_INERT,
                                       int aAlign = wxDVR_DEFAULT_ALIGNMENT ) :
            wxDataViewRenderer( aVariantType, aMode, aAlign )
    {
    }

    void ShowAsRadio() {}
    KIND_COMPAT KindCompat() const override { return TOGGLE; }
};


class KIQT_API wxDataViewIconTextRenderer : public wxDataViewRenderer
{
public:
    explicit wxDataViewIconTextRenderer( const wxString& aVariantType = wxString( "wxDataViewIconText" ),
                                         wxDataViewCellMode aMode = wxDATAVIEW_CELL_INERT,
                                         int aAlign = wxDVR_DEFAULT_ALIGNMENT ) :
            wxDataViewRenderer( aVariantType, aMode, aAlign )
    {
    }

    KIND_COMPAT KindCompat() const override { return ICONTEXT; }
};


class KIQT_API wxDataViewBitmapRenderer : public wxDataViewRenderer
{
public:
    explicit wxDataViewBitmapRenderer( const wxString& aVariantType = wxString( "wxBitmapBundle" ),
                                       wxDataViewCellMode aMode = wxDATAVIEW_CELL_INERT,
                                       int aAlign = wxDVR_DEFAULT_ALIGNMENT ) :
            wxDataViewRenderer( aVariantType, aMode, aAlign )
    {
    }

    KIND_COMPAT KindCompat() const override { return BITMAP; }
};


/// A renderer drawing its cells (wxDataViewCustomRenderer).
class KIQT_API wxDataViewCustomRenderer : public wxDataViewRenderer
{
public:
    explicit wxDataViewCustomRenderer( const wxString& aVariantType = wxString( "string" ),
                                       wxDataViewCellMode aMode = wxDATAVIEW_CELL_INERT,
                                       int aAlign = wxDVR_DEFAULT_ALIGNMENT ) :
            wxDataViewRenderer( aVariantType, aMode, aAlign )
    {
    }

    virtual bool Render( wxRect aCell, wxDC* aDC, int aState ) = 0;
    virtual wxSize GetSize() const = 0;

    virtual bool ActivateCell( const wxRect& aCell, wxDataViewModel* aModel,
                               const wxDataViewItem& aItem, unsigned int aCol,
                               const wxMouseEvent* aMouseEvent )
    {
        return false;
    }

    /// Draw a text in a cell, in the colours of its attributes and state.
    virtual void RenderText( const wxString& aText, int aXOffset, wxRect aCell, wxDC* aDC,
                             int aState );
    virtual void RenderBackground( wxDC* aDC, const wxRect& aRect );

    wxDC* GetDC() { return m_dc; }
    bool IsCustomRenderer() const override { return true; }
    KIND_COMPAT KindCompat() const override { return CUSTOM; }

    /// The DC of the cell being drawn (the delegate of the control).
    void SetDCCompat( wxDC* aDC ) { m_dc = aDC; }

protected:
    wxSize GetTextExtent( const wxString& aText ) const;

private:
    wxDC* m_dc = nullptr;
};


/// A column of a data view (wxDataViewColumn).
class KIQT_API wxDataViewColumn
{
public:
    wxDataViewColumn( const wxString& aTitle, wxDataViewRenderer* aRenderer,
                      unsigned int aModelColumn, int aWidth = wxDVC_DEFAULT_WIDTH,
                      wxAlignment aAlign = wxALIGN_CENTER, int aFlags = wxDATAVIEW_COL_RESIZABLE );
    wxDataViewColumn( const wxBitmapBundle& aBitmap, wxDataViewRenderer* aRenderer,
                      unsigned int aModelColumn, int aWidth = wxDVC_DEFAULT_WIDTH,
                      wxAlignment aAlign = wxALIGN_CENTER, int aFlags = wxDATAVIEW_COL_RESIZABLE );
    ~wxDataViewColumn();

    wxDataViewColumn( const wxDataViewColumn& ) = delete;
    wxDataViewColumn& operator=( const wxDataViewColumn& ) = delete;

    wxString GetTitle() const { return m_title; }
    void SetTitle( const wxString& aTitle );
    wxDataViewRenderer* GetRenderer() const { return m_renderer; }
    unsigned int GetModelColumn() const { return m_modelColumn; }
    wxDataViewCtrl* GetOwner() const { return m_owner; }
    void SetOwner( wxDataViewCtrl* aOwner ) { m_owner = aOwner; }

    int GetWidth() const;
    void SetWidth( int aWidth );
    int GetMinWidth() const { return m_minWidth; }
    void SetMinWidth( int aWidth ) { m_minWidth = aWidth; }
    wxAlignment GetAlignment() const { return m_align; }
    void SetAlignment( wxAlignment aAlign ) { m_align = aAlign; }
    int GetFlags() const { return m_flags; }
    void SetFlags( int aFlags ) { m_flags = aFlags; }
    void SetFlag( int aFlag ) { m_flags |= aFlag; }

    bool IsSortable() const { return m_flags & wxDATAVIEW_COL_SORTABLE; }
    void SetSortable( bool aSortable );
    bool IsResizeable() const { return m_flags & wxDATAVIEW_COL_RESIZABLE; }
    void SetResizeable( bool aResizable );
    bool IsReorderable() const { return m_flags & wxDATAVIEW_COL_REORDERABLE; }
    void SetReorderable( bool aReorderable ) {}
    bool IsHidden() const { return m_flags & wxDATAVIEW_COL_HIDDEN; }
    void SetHidden( bool aHidden );
    bool IsShown() const { return !IsHidden(); }

    bool IsSortKey() const { return m_sortKey; }
    bool IsSortOrderAscending() const { return m_ascending; }
    void SetSortOrder( bool aAscending );
    void UnsetAsSortKey();

    void SetBitmap( const wxBitmapBundle& aBitmap ) { m_bitmap = aBitmap; }

    /// The column was given its sort state by the control (a click on its header).
    void setSortCompat( bool aKey, bool aAscending ) { m_sortKey = aKey; m_ascending = aAscending; }

private:
    friend class wxDataViewCtrl;

    wxString            m_title;
    wxBitmapBundle      m_bitmap;
    wxDataViewRenderer* m_renderer;
    unsigned int        m_modelColumn;
    int                 m_width;
    int                 m_minWidth;
    wxAlignment         m_align;
    int                 m_flags;
    bool                m_sortKey;
    bool                m_ascending;
    wxDataViewCtrl*     m_owner;
};


/// An event of a data view (wxDataViewEvent).
class KIQT_API wxDataViewEvent : public wxNotifyEvent
{
public:
    explicit wxDataViewEvent( wxEventType aType = wxEVT_NULL, int aId = 0 ) :
            wxNotifyEvent( aType, aId )
    {
    }

    wxDataViewEvent( wxEventType aType, wxDataViewCtrl* aCtrl, const wxDataViewItem& aItem );
    wxDataViewEvent( wxEventType aType, wxDataViewCtrl* aCtrl, wxDataViewColumn* aColumn,
                     const wxDataViewItem& aItem = wxDataViewItem() );

    wxDataViewItem GetItem() const { return m_item; }
    void SetItem( const wxDataViewItem& aItem ) { m_item = aItem; }
    int GetColumn() const { return m_column; }
    void SetColumn( int aColumn ) { m_column = aColumn; }
    wxDataViewColumn* GetDataViewColumn() const { return m_dataViewColumn; }
    void SetDataViewColumn( wxDataViewColumn* aColumn ) { m_dataViewColumn = aColumn; }
    wxDataViewModel* GetModel() const { return m_model; }
    void SetModel( wxDataViewModel* aModel ) { m_model = aModel; }
    const wxVariant& GetValue() const { return m_value; }
    void SetValue( const wxVariant& aValue ) { m_value = aValue; }
    bool IsEditCancelled() const { return m_editCancelled; }
    wxPoint GetPosition() const { return m_position; }
    void SetPosition( int aX, int aY ) { m_position = wxPoint( aX, aY ); }
    int GetCacheFrom() const { return 0; }
    int GetCacheTo() const { return 0; }

    // Drag and drop: not done by the data views of the layer
    void SetDataObject( class wxDataObject* aObject ) { m_dataObject = aObject; }
    class wxDataObject* GetDataObject() const { return m_dataObject; }
    void SetDataFormat( const class wxDataFormat& aFormat ) {}
    void SetDragFlags( int aFlags ) {}
    void SetDropEffect( int aEffect ) {}
    int GetDropEffect() const { return 0; }
    int GetProposedDropIndex() const { return -1; }

private:
    wxDataViewItem      m_item;
    int                 m_column = -1;
    wxDataViewColumn*   m_dataViewColumn = nullptr;
    wxDataViewModel*    m_model = nullptr;
    wxVariant           m_value;
    bool                m_editCancelled = false;
    wxPoint             m_position = wxPoint( -1, -1 );
    class wxDataObject* m_dataObject = nullptr;
};

#define wxDataViewEventHandler( func ) ( &func )


/// A data view (wxDataViewCtrl): the items of its model in a tree (or a list) with columns.
class KIQT_API wxDataViewCtrl : public wxWindowCompat<QTreeView>
{
public:
    explicit wxDataViewCtrl( wxWidgetArgCompat aParent = nullptr, int aId = wxID_ANY,
                             const wxPoint& aPos = wxDefaultPosition,
                             const wxSize& aSize = wxDefaultSize, long aStyle = 0,
                             const wxValidator& aValidator = wxDefaultValidator,
                             const wxString& aName = wxEmptyString );
    ~wxDataViewCtrl() override;

    /// The model of the control (the control keeps a reference to it).
    virtual bool AssociateModel( wxDataViewModel* aModel );
    wxDataViewModel* GetModel() const { return m_model; }

    // The columns

    virtual bool AppendColumn( wxDataViewColumn* aColumn );
    virtual bool PrependColumn( wxDataViewColumn* aColumn );
    virtual bool InsertColumn( unsigned int aPos, wxDataViewColumn* aColumn );
    virtual bool DeleteColumn( wxDataViewColumn* aColumn );
    virtual bool ClearColumns();
    unsigned int GetColumnCount() const { return unsigned( m_columns.size() ); }
    wxDataViewColumn* GetColumn( unsigned int aPos ) const;
    int GetColumnPosition( const wxDataViewColumn* aColumn ) const;
    wxDataViewColumn* GetSortingColumn() const;
    wxDataViewColumn* GetCurrentColumn() const;

    wxDataViewColumn* AppendTextColumn( const wxString& aLabel, unsigned int aModelColumn,
                                        wxDataViewCellMode aMode = wxDATAVIEW_CELL_INERT,
                                        int aWidth = -1, wxAlignment aAlign = wxALIGN_NOT,
                                        int aFlags = wxDATAVIEW_COL_RESIZABLE );
    wxDataViewColumn* AppendToggleColumn( const wxString& aLabel, unsigned int aModelColumn,
                                          wxDataViewCellMode aMode = wxDATAVIEW_CELL_INERT,
                                          int aWidth = wxDVC_TOGGLE_DEFAULT_WIDTH,
                                          wxAlignment aAlign = wxALIGN_CENTER,
                                          int aFlags = wxDATAVIEW_COL_RESIZABLE );
    wxDataViewColumn* AppendIconTextColumn( const wxString& aLabel, unsigned int aModelColumn,
                                            wxDataViewCellMode aMode = wxDATAVIEW_CELL_INERT,
                                            int aWidth = -1, wxAlignment aAlign = wxALIGN_NOT,
                                            int aFlags = wxDATAVIEW_COL_RESIZABLE );
    wxDataViewColumn* AppendBitmapColumn( const wxString& aLabel, unsigned int aModelColumn,
                                          wxDataViewCellMode aMode = wxDATAVIEW_CELL_INERT,
                                          int aWidth = -1, wxAlignment aAlign = wxALIGN_CENTER,
                                          int aFlags = wxDATAVIEW_COL_RESIZABLE );

    void SetExpanderColumn( wxDataViewColumn* aColumn ) {}
    wxDataViewColumn* GetExpanderColumn() const { return GetColumn( 0 ); }
    void SetIndent( int aIndent ) { setIndentation( aIndent ); }
    int GetIndent() const { return indentation(); }
    bool SetRowHeight( int aHeight );
    bool EnableDragSource( const class wxDataFormat& aFormat ) { return false; }
    bool EnableDropTarget( const class wxDataFormat& aFormat ) { return false; }
    bool EnableDropTargets( const std::vector<class wxDataFormat>& aFormats ) { return false; }
    bool SetHeaderAttr( const class wxItemAttr& aAttr ) { return false; }
    bool SetAlternateRowColour( const wxColour& aColour ) { setAlternatingRowColors( true ); return true; }

    // The items

    wxDataViewItem GetCurrentItem() const;
    void SetCurrentItem( const wxDataViewItem& aItem );
    wxDataViewItem GetSelection() const;
    int GetSelections( wxDataViewItemArray& aSelections ) const;
    void SetSelections( const wxDataViewItemArray& aSelections );
    int GetSelectedItemsCount() const;
    bool HasSelection() const { return GetSelectedItemsCount() > 0; }
    void Select( const wxDataViewItem& aItem );
    void Unselect( const wxDataViewItem& aItem );
    bool IsSelected( const wxDataViewItem& aItem ) const;
    void SelectAll();
    void UnselectAll();

    void Expand( const wxDataViewItem& aItem );
    void ExpandChildren( const wxDataViewItem& aItem );
    void ExpandAncestors( const wxDataViewItem& aItem );
    void Collapse( const wxDataViewItem& aItem );
    bool IsExpanded( const wxDataViewItem& aItem ) const;
    void EnsureVisible( const wxDataViewItem& aItem, const wxDataViewColumn* aColumn = nullptr );
    void HitTest( const wxPoint& aPoint, wxDataViewItem& aItem, wxDataViewColumn*& aColumn ) const;
    wxRect GetItemRect( const wxDataViewItem& aItem, const wxDataViewColumn* aColumn = nullptr ) const;
    int GetCountPerPage() const;
    wxDataViewItem GetTopItem() const;

    void EditItem( const wxDataViewItem& aItem, const wxDataViewColumn* aColumn );

    virtual void DoSetToolTipText( const wxString& aTip ) { SetToolTip( aTip ); }

    /// The model of Qt of the control.
    KIWX_DATAVIEW_MODEL* GetQtModelCompat() const { return m_qtModel; }

    /// The index of Qt of an item (in the first column), invalid for the root.
    QModelIndex IndexCompat( const wxDataViewItem& aItem, int aColumn = 0 ) const;
    wxDataViewItem ItemCompat( const QModelIndex& aIndex ) const;

    /// Send an event of the control (its item, column, value...).
    bool SendEventCompat( wxEventType aType, const wxDataViewItem& aItem, int aColumn = -1,
                          const wxVariant* aValue = nullptr );

private:
    void insertColumn( unsigned int aPos, wxDataViewColumn* aColumn );
    void applyColumn( unsigned int aPos );

    friend class wxDataViewColumn;
    friend class KIWX_DATAVIEW_MODEL;

    wxDataViewModel*               m_model;
    KIWX_DATAVIEW_MODEL*           m_qtModel;
    wxDataViewModelNotifier*       m_notifierCompat = nullptr;
    std::vector<wxDataViewColumn*> m_columns;
};


/// The store of the rows of a wxDataViewListCtrl (wxDataViewListStore).
class KIQT_API wxDataViewListStore : public wxDataViewIndexListModel
{
public:
    wxDataViewListStore() = default;
    ~wxDataViewListStore() override = default;

    void PrependColumn( const wxString& aVariantType ) { m_columnTypes.insert( m_columnTypes.begin(), aVariantType ); }
    void InsertColumn( unsigned int aPos, const wxString& aVariantType );
    void AppendColumn( const wxString& aVariantType ) { m_columnTypes.push_back( aVariantType ); }
    unsigned int GetColumnCount() const override { return unsigned( m_columnTypes.size() ); }
    wxString GetColumnType( unsigned int aCol ) const override;

    void AppendItem( const std::vector<wxVariant>& aValues, wxUIntPtr aData = 0 );
    void PrependItem( const std::vector<wxVariant>& aValues, wxUIntPtr aData = 0 );
    void InsertItem( unsigned int aRow, const std::vector<wxVariant>& aValues, wxUIntPtr aData = 0 );
    void DeleteItem( unsigned int aRow );
    void DeleteAllItems();
    void ClearColumns() { m_columnTypes.clear(); }

    unsigned int GetItemCount() const { return unsigned( m_rows.size() ); }
    void SetItemData( const wxDataViewItem& aItem, wxUIntPtr aData );
    wxUIntPtr GetItemData( const wxDataViewItem& aItem ) const;

    void GetValueByRow( wxVariant& aValue, unsigned int aRow, unsigned int aCol ) const override;
    bool SetValueByRow( const wxVariant& aValue, unsigned int aRow, unsigned int aCol ) override;

private:
    struct ROW
    {
        std::vector<wxVariant> values;
        wxUIntPtr              data = 0;
    };

    std::vector<wxString> m_columnTypes;
    std::vector<ROW>      m_rows;
};


/// A data view of a list of rows of values (wxDataViewListCtrl).
class KIQT_API wxDataViewListCtrl : public wxDataViewCtrl
{
public:
    explicit wxDataViewListCtrl( wxWidgetArgCompat aParent = nullptr, int aId = wxID_ANY,
                                 const wxPoint& aPos = wxDefaultPosition,
                                 const wxSize& aSize = wxDefaultSize, long aStyle = wxDV_ROW_LINES,
                                 const wxValidator& aValidator = wxDefaultValidator );
    ~wxDataViewListCtrl() override;

    wxDataViewListStore* GetStore() { return m_store; }
    const wxDataViewListStore* GetStore() const { return m_store; }

    int ItemToRow( const wxDataViewItem& aItem ) const;
    wxDataViewItem RowToItem( int aRow ) const;
    int GetSelectedRow() const;
    void SelectRow( unsigned aRow );
    void UnselectRow( unsigned aRow );
    bool IsRowSelected( unsigned aRow ) const;

    bool AppendColumn( wxDataViewColumn* aColumn ) override;
    bool AppendColumn( wxDataViewColumn* aColumn, const wxString& aVariantType );
    bool PrependColumn( wxDataViewColumn* aColumn ) override;
    bool InsertColumn( unsigned int aPos, wxDataViewColumn* aColumn ) override;
    bool ClearColumns() override;

    wxDataViewColumn* AppendTextColumn( const wxString& aLabel,
                                        wxDataViewCellMode aMode = wxDATAVIEW_CELL_INERT,
                                        int aWidth = -1, wxAlignment aAlign = wxALIGN_LEFT,
                                        int aFlags = wxDATAVIEW_COL_RESIZABLE );
    wxDataViewColumn* AppendToggleColumn( const wxString& aLabel,
                                          wxDataViewCellMode aMode = wxDATAVIEW_CELL_ACTIVATABLE,
                                          int aWidth = wxDVC_TOGGLE_DEFAULT_WIDTH,
                                          wxAlignment aAlign = wxALIGN_LEFT,
                                          int aFlags = wxDATAVIEW_COL_RESIZABLE );
    wxDataViewColumn* AppendIconTextColumn( const wxString& aLabel,
                                            wxDataViewCellMode aMode = wxDATAVIEW_CELL_INERT,
                                            int aWidth = -1, wxAlignment aAlign = wxALIGN_LEFT,
                                            int aFlags = wxDATAVIEW_COL_RESIZABLE );

    void AppendItem( const std::vector<wxVariant>& aValues, wxUIntPtr aData = 0 );
    void PrependItem( const std::vector<wxVariant>& aValues, wxUIntPtr aData = 0 );
    void InsertItem( unsigned int aRow, const std::vector<wxVariant>& aValues, wxUIntPtr aData = 0 );
    void DeleteItem( unsigned aRow );
    void DeleteAllItems();
    unsigned int GetItemCount() const;
    wxUIntPtr GetItemData( const wxDataViewItem& aItem ) const;
    void SetItemData( const wxDataViewItem& aItem, wxUIntPtr aData );

    void SetValue( const wxVariant& aValue, unsigned int aRow, unsigned int aCol );
    void GetValue( wxVariant& aValue, unsigned int aRow, unsigned int aCol );
    void SetTextValue( const wxString& aValue, unsigned int aRow, unsigned int aCol );
    wxString GetTextValue( unsigned int aRow, unsigned int aCol ) const;
    void SetToggleValue( bool aValue, unsigned int aRow, unsigned int aCol );
    bool GetToggleValue( unsigned int aRow, unsigned int aCol ) const;

private:
    wxDataViewListStore* m_store;
};

#endif // KIWX_DATAVIEW_H
