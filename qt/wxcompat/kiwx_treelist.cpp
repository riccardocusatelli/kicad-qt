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

#include <wx/treelist.h>

#include <algorithm>

#include <QFontMetrics>
#include <QVBoxLayout>


/// The items of a wxTreeListCtrl: a tree of nodes, with the texts of the columns.
class wxTreeListModelCompat : public wxDataViewModel
{
public:
    struct NODE
    {
        NODE*                              parent = nullptr;
        std::vector<std::unique_ptr<NODE>> children;
        std::vector<wxString>              texts;
        std::unique_ptr<wxClientData>      data;
        wxCheckBoxState                    checked = wxCHK_UNCHECKED;
    };

    wxDataViewItem ToItem( const NODE* aNode ) const
    {
        return aNode == &m_root ? wxDataViewItem() : wxDataViewItem( const_cast<NODE*>( aNode ) );
    }

    NODE* ToNode( const wxDataViewItem& aItem ) const
    {
        return aItem.IsOk() ? static_cast<NODE*>( aItem.GetID() ) : const_cast<NODE*>( &m_root );
    }

    static NODE* ToNode( const wxTreeListItem& aItem ) { return static_cast<NODE*>( aItem.GetID() ); }

    void GetValue( wxVariant& aVariant, const wxDataViewItem& aItem,
                   unsigned int aCol ) const override
    {
        const NODE* node = ToNode( aItem );
        aVariant = aCol < node->texts.size() ? node->texts[aCol] : wxString();
    }

    bool SetValue( const wxVariant& aVariant, const wxDataViewItem& aItem,
                   unsigned int aCol ) override
    {
        NODE* node = ToNode( aItem );

        if( node->texts.size() <= aCol )
            node->texts.resize( aCol + 1 );

        node->texts[aCol] = aVariant.GetString();
        return true;
    }

    wxDataViewItem GetParent( const wxDataViewItem& aItem ) const override
    {
        const NODE* node = ToNode( aItem );
        return node->parent ? ToItem( node->parent ) : wxDataViewItem();
    }

    bool IsContainer( const wxDataViewItem& aItem ) const override
    {
        return !aItem.IsOk() || !ToNode( aItem )->children.empty();
    }

    bool HasContainerColumns( const wxDataViewItem& aItem ) const override { return true; }

    unsigned int GetChildren( const wxDataViewItem& aItem,
                              wxDataViewItemArray& aChildren ) const override
    {
        for( const std::unique_ptr<NODE>& child : ToNode( aItem )->children )
            aChildren.push_back( ToItem( child.get() ) );

        return unsigned( ToNode( aItem )->children.size() );
    }

    unsigned int GetColumnCount() const override { return m_columns; }
    wxString GetColumnType( unsigned int aCol ) const override { return wxS( "string" ); }

    /// Remove the text of a column from all the nodes.
    void EraseColumn( NODE* aNode, unsigned aColumn )
    {
        if( aColumn < aNode->texts.size() )
            aNode->texts.erase( aNode->texts.begin() + aColumn );

        for( const std::unique_ptr<NODE>& child : aNode->children )
            EraseColumn( child.get(), aColumn );
    }

    NODE     m_root;
    unsigned m_columns = 0;
};

using NODE = wxTreeListModelCompat::NODE;


wxTreeListCtrl::wxTreeListCtrl( wxWidgetArgCompat aParent, int aId, const wxPoint& aPos,
                                const wxSize& aSize, long aStyle, const wxString& aName ) :
        wxWindowCompat<QWidget>( aParent.get() ),
        m_style( aStyle )
{
    SetId( aId );

    long viewStyle = 0;

    if( aStyle & wxTL_MULTIPLE )
        viewStyle |= wxDV_MULTIPLE;

    if( aStyle & wxTL_NO_HEADER )
        viewStyle |= wxDV_NO_HEADER;

    m_view = new wxDataViewCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, viewStyle );
    m_model = new wxTreeListModelCompat;
    m_view->AssociateModel( m_model );
    m_model->DecRef();     // the view keeps it

    QVBoxLayout* layout = new QVBoxLayout( this );
    layout->setContentsMargins( 0, 0, 0, 0 );
    layout->addWidget( m_view );

    // The events of the view are the events of the control, for its items
    for( auto [viewType, type] :
         { std::pair( wxEVT_DATAVIEW_SELECTION_CHANGED, wxEVT_TREELIST_SELECTION_CHANGED ),
           std::pair( wxEVT_DATAVIEW_ITEM_EXPANDING, wxEVT_TREELIST_ITEM_EXPANDING ),
           std::pair( wxEVT_DATAVIEW_ITEM_EXPANDED, wxEVT_TREELIST_ITEM_EXPANDED ),
           std::pair( wxEVT_DATAVIEW_ITEM_ACTIVATED, wxEVT_TREELIST_ITEM_ACTIVATED ),
           std::pair( wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, wxEVT_TREELIST_ITEM_CONTEXT_MENU ) } )
    {
        wxEventType treeListType = type;

        m_view->Bind( viewType,
                      [this, treeListType]( wxDataViewEvent& aEvent )
                      {
                          sendEvent( treeListType, aEvent.GetItem() );
                      } );
    }
}


wxTreeListCtrl::~wxTreeListCtrl() = default;


void wxTreeListCtrl::sendEvent( wxEventType aType, const wxDataViewItem& aItem )
{
    wxTreeListEvent event( aType, GetId(), aItem.IsOk() ? wxTreeListItem( aItem.GetID() )
                                                        : wxTreeListItem() );
    event.SetEventObject( this );
    ProcessEvent( event );
}


int wxTreeListCtrl::AppendColumn( const wxString& aTitle, int aWidth, wxAlignment aAlign,
                                  int aFlags )
{
    unsigned column = m_model->m_columns++;

    m_view->AppendTextColumn( aTitle, column, wxDATAVIEW_CELL_INERT, aWidth, aAlign, aFlags );
    return int( column );
}


unsigned wxTreeListCtrl::GetColumnCount() const
{
    return m_model->m_columns;
}


bool wxTreeListCtrl::DeleteColumn( unsigned aColumn )
{
    if( aColumn >= m_model->m_columns )
        return false;

    m_view->DeleteColumn( m_view->GetColumn( aColumn ) );
    m_model->EraseColumn( &m_model->m_root, aColumn );
    m_model->m_columns--;
    return true;
}


void wxTreeListCtrl::ClearColumns()
{
    m_view->ClearColumns();

    while( m_model->m_columns > 0 )
        m_model->EraseColumn( &m_model->m_root, --m_model->m_columns );
}


void wxTreeListCtrl::SetColumnWidth( unsigned aColumn, int aWidth )
{
    if( wxDataViewColumn* column = m_view->GetColumn( aColumn ) )
        column->SetWidth( aWidth );
}


int wxTreeListCtrl::GetColumnWidth( unsigned aColumn ) const
{
    wxDataViewColumn* column = m_view->GetColumn( aColumn );
    return column ? column->GetWidth() : 0;
}


int wxTreeListCtrl::WidthFor( const wxString& aText ) const
{
    // The text and the margins of a cell
    return m_view->fontMetrics().horizontalAdvance( ToQString( aText ) ) + 16;
}


wxTreeListItem wxTreeListCtrl::InsertItem( wxTreeListItem aParent, wxTreeListItem aPrevious,
                                           const wxString& aText, int aImageClosed,
                                           int aImageOpened, wxClientData* aData )
{
    NODE* parent = aParent.IsOk() ? m_model->ToNode( aParent ) : &m_model->m_root;
    auto  node = std::make_unique<NODE>();

    node->parent = parent;
    node->texts.push_back( aText );
    node->data.reset( aData );

    // After the previous item, first without it
    auto pos = parent->children.begin();

    if( aPrevious.IsOk() )
    {
        pos = std::find_if( parent->children.begin(), parent->children.end(),
                            [&]( const std::unique_ptr<NODE>& aChild )
                            {
                                return aChild.get() == aPrevious.GetID();
                            } );

        if( pos != parent->children.end() )
            ++pos;
    }

    NODE* added = parent->children.insert( pos, std::move( node ) )->get();
    m_model->ItemAdded( m_model->ToItem( parent ), m_model->ToItem( added ) );
    return wxTreeListItem( added );
}


wxTreeListItem wxTreeListCtrl::AppendItem( wxTreeListItem aParent, const wxString& aText,
                                           int aImageClosed, int aImageOpened,
                                           wxClientData* aData )
{
    NODE* parent = aParent.IsOk() ? m_model->ToNode( aParent ) : &m_model->m_root;

    wxTreeListItem last = parent->children.empty() ? wxTreeListItem()
                                                   : wxTreeListItem( parent->children.back().get() );

    if( !last.IsOk() )
        return InsertItem( aParent, wxTreeListItem(), aText, aImageClosed, aImageOpened, aData );

    return InsertItem( aParent, last, aText, aImageClosed, aImageOpened, aData );
}


wxTreeListItem wxTreeListCtrl::PrependItem( wxTreeListItem aParent, const wxString& aText,
                                            int aImageClosed, int aImageOpened,
                                            wxClientData* aData )
{
    return InsertItem( aParent, wxTreeListItem(), aText, aImageClosed, aImageOpened, aData );
}


void wxTreeListCtrl::DeleteItem( wxTreeListItem aItem )
{
    NODE* node = m_model->ToNode( aItem );

    if( !node || node == &m_model->m_root )
        return;

    NODE* parent = node->parent;

    auto it = std::find_if( parent->children.begin(), parent->children.end(),
                            [&]( const std::unique_ptr<NODE>& aChild )
                            {
                                return aChild.get() == node;
                            } );

    if( it == parent->children.end() )
        return;

    // The view forgets the item before it is deleted
    std::unique_ptr<NODE> deleted = std::move( *it );
    parent->children.erase( it );
    m_model->ItemDeleted( m_model->ToItem( parent ), m_model->ToItem( deleted.get() ) );
}


void wxTreeListCtrl::DeleteAllItems()
{
    m_model->m_root.children.clear();
    m_model->Cleared();
}


wxTreeListItem wxTreeListCtrl::GetRootItem() const
{
    return wxTreeListItem( &m_model->m_root );
}


wxTreeListItem wxTreeListCtrl::GetItemParent( wxTreeListItem aItem ) const
{
    NODE* node = m_model->ToNode( aItem );
    return node && node->parent ? wxTreeListItem( node->parent ) : wxTreeListItem();
}


wxTreeListItem wxTreeListCtrl::GetFirstChild( wxTreeListItem aItem ) const
{
    NODE* node = m_model->ToNode( aItem );
    return node && !node->children.empty() ? wxTreeListItem( node->children.front().get() )
                                           : wxTreeListItem();
}


wxTreeListItem wxTreeListCtrl::GetNextSibling( wxTreeListItem aItem ) const
{
    NODE* node = m_model->ToNode( aItem );

    if( !node || !node->parent )
        return wxTreeListItem();

    std::vector<std::unique_ptr<NODE>>& siblings = node->parent->children;

    for( size_t i = 0; i + 1 < siblings.size(); ++i )
    {
        if( siblings[i].get() == node )
            return wxTreeListItem( siblings[i + 1].get() );
    }

    return wxTreeListItem();
}


wxTreeListItem wxTreeListCtrl::GetNextItem( wxTreeListItem aItem ) const
{
    // Depth first: the first child, else the next sibling of the item or of an ancestor
    if( wxTreeListItem child = GetFirstChild( aItem ); child.IsOk() )
        return child;

    for( wxTreeListItem item = aItem; item.IsOk() && item != GetRootItem();
         item = GetItemParent( item ) )
    {
        if( wxTreeListItem next = GetNextSibling( item ); next.IsOk() )
            return next;
    }

    return wxTreeListItem();
}


const wxString& wxTreeListCtrl::GetItemText( wxTreeListItem aItem, unsigned aColumn ) const
{
    static const wxString s_empty;
    NODE*                 node = m_model->ToNode( aItem );

    return node && aColumn < node->texts.size() ? node->texts[aColumn] : s_empty;
}


void wxTreeListCtrl::SetItemText( wxTreeListItem aItem, unsigned aColumn, const wxString& aText )
{
    NODE* node = m_model->ToNode( aItem );

    if( !node )
        return;

    if( node->texts.size() <= aColumn )
        node->texts.resize( aColumn + 1 );

    node->texts[aColumn] = aText;
    m_model->ValueChanged( m_model->ToItem( node ), aColumn );
}


wxClientData* wxTreeListCtrl::GetItemData( wxTreeListItem aItem ) const
{
    NODE* node = m_model->ToNode( aItem );
    return node ? node->data.get() : nullptr;
}


void wxTreeListCtrl::SetItemData( wxTreeListItem aItem, wxClientData* aData )
{
    if( NODE* node = m_model->ToNode( aItem ) )
        node->data.reset( aData );
}


void wxTreeListCtrl::Expand( wxTreeListItem aItem )
{
    m_view->Expand( m_model->ToItem( m_model->ToNode( aItem ) ) );
}


void wxTreeListCtrl::Collapse( wxTreeListItem aItem )
{
    m_view->Collapse( m_model->ToItem( m_model->ToNode( aItem ) ) );
}


bool wxTreeListCtrl::IsExpanded( wxTreeListItem aItem ) const
{
    return m_view->IsExpanded( m_model->ToItem( m_model->ToNode( aItem ) ) );
}


unsigned wxTreeListCtrl::GetSelections( wxTreeListItems& aSelections ) const
{
    wxDataViewItemArray items;
    m_view->GetSelections( items );

    aSelections.clear();

    for( const wxDataViewItem& item : items )
        aSelections.emplace_back( item.GetID() );

    return unsigned( aSelections.size() );
}


wxTreeListItem wxTreeListCtrl::GetSelection() const
{
    wxDataViewItem item = m_view->GetSelection();
    return item.IsOk() ? wxTreeListItem( item.GetID() ) : wxTreeListItem();
}


void wxTreeListCtrl::Select( wxTreeListItem aItem )
{
    m_view->Select( m_model->ToItem( m_model->ToNode( aItem ) ) );
}


void wxTreeListCtrl::Unselect( wxTreeListItem aItem )
{
    m_view->Unselect( m_model->ToItem( m_model->ToNode( aItem ) ) );
}


bool wxTreeListCtrl::IsSelected( wxTreeListItem aItem ) const
{
    return m_view->IsSelected( m_model->ToItem( m_model->ToNode( aItem ) ) );
}


void wxTreeListCtrl::SelectAll()
{
    m_view->SelectAll();
}


void wxTreeListCtrl::UnselectAll()
{
    m_view->UnselectAll();
}


void wxTreeListCtrl::EnsureVisible( wxTreeListItem aItem )
{
    m_view->EnsureVisible( m_model->ToItem( m_model->ToNode( aItem ) ) );
}


void wxTreeListCtrl::CheckItem( wxTreeListItem aItem, wxCheckBoxState aState )
{
    if( NODE* node = m_model->ToNode( aItem ) )
        node->checked = aState;
}


wxCheckBoxState wxTreeListCtrl::GetCheckedState( wxTreeListItem aItem ) const
{
    NODE* node = m_model->ToNode( aItem );
    return node ? node->checked : wxCHK_UNCHECKED;
}


void wxTreeListCtrl::SetSortColumn( unsigned aColumn, bool aAscendingOrder )
{
}


bool wxTreeListCtrl::GetSortColumn( unsigned* aColumn, bool* aAscendingOrder )
{
    return false;
}
