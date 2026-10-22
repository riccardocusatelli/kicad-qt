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

#include <wx/kiwx_treectrl.h>

#include <algorithm>
#include <cstdint>

#include <QTreeWidgetItemIterator>

#include <qt_strings.h>


wxTreeCtrl::wxTreeCtrl( wxWidgetArgCompat aParent, int aId, const wxPoint& aPos, const wxSize& aSize,
                        long aStyle, const wxValidator& aValidator, const wxString& aName ) :
        wxWindowCompat<QTreeWidget>( aParent ),
        m_style( aStyle ),
        m_root( nullptr )
{
    SetId( aId );

    setColumnCount( 1 );
    setHeaderHidden( true );
    setSelectionMode( ( aStyle & wxTR_MULTIPLE ) ? QAbstractItemView::ExtendedSelection
                                                 : QAbstractItemView::SingleSelection );
    setRootIsDecorated( aStyle & ( wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT | wxTR_TWIST_BUTTONS ) );
    setContextMenuPolicy( Qt::CustomContextMenu );

    if( aSize.x > 0 || aSize.y > 0 )
        resize( std::max( aSize.x, 0 ), std::max( aSize.y, 0 ) );

    QObject::connect( this, &QTreeWidget::currentItemChanged, this,
                      [this]( QTreeWidgetItem* aCurrent, QTreeWidgetItem* aPrevious )
                      {
                          sendEvent( wxEVT_TREE_SEL_CHANGED, aCurrent, aPrevious );
                      } );

    QObject::connect( this, &QTreeWidget::itemActivated, this,
                      [this]( QTreeWidgetItem* aItem, int )
                      {
                          sendEvent( wxEVT_TREE_ITEM_ACTIVATED, aItem );
                      } );

    QObject::connect( this, &QTreeWidget::itemExpanded, this,
                      [this]( QTreeWidgetItem* aItem )
                      {
                          sendEvent( wxEVT_TREE_ITEM_EXPANDED, aItem );
                      } );

    QObject::connect( this, &QTreeWidget::itemCollapsed, this,
                      [this]( QTreeWidgetItem* aItem )
                      {
                          sendEvent( wxEVT_TREE_ITEM_COLLAPSED, aItem );
                      } );

    QObject::connect( this, &QWidget::customContextMenuRequested, this,
                      [this]( const QPoint& aPos )
                      {
                          // The position is in the viewport: the events have it in the tree
                          QTreeWidgetItem* item = itemAt( aPos );
                          QPoint           pos = viewport()->mapTo( this, aPos );

                          sendEvent( wxEVT_TREE_ITEM_RIGHT_CLICK, item, nullptr, pos );
                          sendEvent( wxEVT_TREE_ITEM_MENU, item, nullptr, pos );
                      } );
}


wxTreeCtrl::~wxTreeCtrl()
{
    blockSignals( true );
}


QTreeWidgetItem* wxTreeCtrl::ToQtCompat( const wxTreeItemId& aItem ) const
{
    return static_cast<QTreeWidgetItem*>( aItem.GetID() );
}


QTreeWidgetItem* wxTreeCtrl::newItem( const wxString& aText, int aImage, wxTreeItemData* aData )
{
    QTreeWidgetItem* item = new QTreeWidgetItem( QStringList( ToQString( aText ) ) );

    if( aImage >= 0 && m_imageList && aImage < m_imageList->GetImageCount() )
        item->setIcon( 0, QIcon( m_imageList->GetBitmap( aImage ) ) );

    if( aData )
    {
        aData->SetId( ToItemCompat( item ) );
        m_data[item].reset( aData );
    }

    return item;
}


void wxTreeCtrl::forgetData( QTreeWidgetItem* aItem )
{
    m_data.erase( aItem );

    for( int i = 0; i < aItem->childCount(); ++i )
        forgetData( aItem->child( i ) );
}


void wxTreeCtrl::sendEvent( wxEventType aType, QTreeWidgetItem* aItem, QTreeWidgetItem* aOld,
                            const QPoint& aPos )
{
    wxTreeEvent event( aType, GetId(), ToItemCompat( aItem ) );
    event.SetOldItem( ToItemCompat( aOld ) );
    event.SetPoint( wxPoint( aPos.x(), aPos.y() ) );
    event.SetEventObject( this );
    ProcessEvent( event );
}


wxTreeItemId wxTreeCtrl::AddRoot( const wxString& aText, int aImage, int aSelImage,
                                  wxTreeItemData* aData )
{
    if( m_style & wxTR_HIDE_ROOT )
    {
        // The root is the invisible root of Qt: its children are the top level items
        m_root = invisibleRootItem();

        if( aData )
        {
            aData->SetId( ToItemCompat( m_root ) );
            m_data[m_root].reset( aData );
        }
    }
    else
    {
        m_root = newItem( aText, aImage, aData );
        addTopLevelItem( m_root );
    }

    return ToItemCompat( m_root );
}


wxTreeItemId wxTreeCtrl::AppendItem( const wxTreeItemId& aParent, const wxString& aText, int aImage,
                                     int aSelImage, wxTreeItemData* aData )
{
    QTreeWidgetItem* parent = aParent.IsOk() ? ToQtCompat( aParent ) : invisibleRootItem();
    QTreeWidgetItem* item = newItem( aText, aImage, aData );
    parent->addChild( item );
    return ToItemCompat( item );
}


wxTreeItemId wxTreeCtrl::PrependItem( const wxTreeItemId& aParent, const wxString& aText, int aImage,
                                      int aSelImage, wxTreeItemData* aData )
{
    return InsertItem( aParent, size_t( 0 ), aText, aImage, aSelImage, aData );
}


wxTreeItemId wxTreeCtrl::InsertItem( const wxTreeItemId& aParent, const wxTreeItemId& aPrevious,
                                     const wxString& aText, int aImage, int aSelImage,
                                     wxTreeItemData* aData )
{
    QTreeWidgetItem* parent = aParent.IsOk() ? ToQtCompat( aParent ) : invisibleRootItem();
    int              index = aPrevious.IsOk() ? parent->indexOfChild( ToQtCompat( aPrevious ) ) + 1 : 0;

    QTreeWidgetItem* item = newItem( aText, aImage, aData );
    parent->insertChild( index, item );
    return ToItemCompat( item );
}


wxTreeItemId wxTreeCtrl::InsertItem( const wxTreeItemId& aParent, size_t aBefore,
                                     const wxString& aText, int aImage, int aSelImage,
                                     wxTreeItemData* aData )
{
    QTreeWidgetItem* parent = aParent.IsOk() ? ToQtCompat( aParent ) : invisibleRootItem();
    int              index = int( std::min<size_t>( aBefore, size_t( parent->childCount() ) ) );

    QTreeWidgetItem* item = newItem( aText, aImage, aData );
    parent->insertChild( index, item );
    return ToItemCompat( item );
}


void wxTreeCtrl::Delete( const wxTreeItemId& aItem )
{
    QTreeWidgetItem* item = ToQtCompat( aItem );

    if( !item )
        return;

    if( item == m_root )
    {
        DeleteAllItems();
        return;
    }

    forgetData( item );
    delete item;
}


void wxTreeCtrl::DeleteChildren( const wxTreeItemId& aItem )
{
    QTreeWidgetItem* item = ToQtCompat( aItem );

    if( !item )
        return;

    for( QTreeWidgetItem* child : item->takeChildren() )
    {
        forgetData( child );
        delete child;
    }
}


void wxTreeCtrl::DeleteAllItems()
{
    m_data.clear();
    clear();
    m_root = nullptr;
}


wxTreeItemId wxTreeCtrl::GetRootItem() const
{
    return ToItemCompat( m_root );
}


size_t wxTreeCtrl::GetCount() const
{
    size_t count = 0;

    for( QTreeWidgetItemIterator it( const_cast<wxTreeCtrl*>( this ) ); *it; ++it )
        ++count;

    return count;
}


wxString wxTreeCtrl::GetItemText( const wxTreeItemId& aItem ) const
{
    QTreeWidgetItem* item = ToQtCompat( aItem );
    return item ? ToWxString( item->text( 0 ) ) : wxString();
}


void wxTreeCtrl::SetItemText( const wxTreeItemId& aItem, const wxString& aText )
{
    if( QTreeWidgetItem* item = ToQtCompat( aItem ) )
        item->setText( 0, ToQString( aText ) );
}


wxTreeItemData* wxTreeCtrl::GetItemData( const wxTreeItemId& aItem ) const
{
    auto it = m_data.find( ToQtCompat( aItem ) );
    return it != m_data.end() ? it->second.get() : nullptr;
}


void wxTreeCtrl::SetItemData( const wxTreeItemId& aItem, wxTreeItemData* aData )
{
    QTreeWidgetItem* item = ToQtCompat( aItem );

    if( !item )
        return;

    if( aData )
    {
        aData->SetId( aItem );
        m_data[item].reset( aData );
    }
    else
    {
        m_data.erase( item );
    }
}


void wxTreeCtrl::SetItemImage( const wxTreeItemId& aItem, int aImage, int aWhich )
{
    QTreeWidgetItem* item = ToQtCompat( aItem );

    if( !item )
        return;

    if( aImage >= 0 && m_imageList && aImage < m_imageList->GetImageCount() )
        item->setIcon( 0, QIcon( m_imageList->GetBitmap( aImage ) ) );
    else
        item->setIcon( 0, QIcon() );
}


void wxTreeCtrl::SetItemBold( const wxTreeItemId& aItem, bool aBold )
{
    if( QTreeWidgetItem* item = ToQtCompat( aItem ) )
    {
        QFont font = item->font( 0 );
        font.setBold( aBold );
        item->setFont( 0, font );
    }
}


void wxTreeCtrl::SetItemTextColour( const wxTreeItemId& aItem, const wxColour& aColour )
{
    if( QTreeWidgetItem* item = ToQtCompat( aItem ) )
        item->setForeground( 0, aColour.IsOk() ? QBrush( QColor( aColour ) ) : QBrush() );
}


void wxTreeCtrl::SetItemBackgroundColour( const wxTreeItemId& aItem, const wxColour& aColour )
{
    if( QTreeWidgetItem* item = ToQtCompat( aItem ) )
        item->setBackground( 0, aColour.IsOk() ? QBrush( QColor( aColour ) ) : QBrush() );
}


void wxTreeCtrl::SetItemFont( const wxTreeItemId& aItem, const wxFont& aFont )
{
    if( QTreeWidgetItem* item = ToQtCompat( aItem ) )
        item->setFont( 0, QFont( aFont ) );
}


void wxTreeCtrl::SetItemHasChildren( const wxTreeItemId& aItem, bool aHasChildren )
{
    if( QTreeWidgetItem* item = ToQtCompat( aItem ) )
    {
        item->setChildIndicatorPolicy( aHasChildren ? QTreeWidgetItem::ShowIndicator
                                                    : QTreeWidgetItem::DontShowIndicatorWhenChildless );
    }
}


wxTreeItemId wxTreeCtrl::GetItemParent( const wxTreeItemId& aItem ) const
{
    QTreeWidgetItem* item = ToQtCompat( aItem );

    if( !item || item == m_root )
        return wxTreeItemId();

    QTreeWidgetItem* parent = item->parent();

    // The top level items are the children of the hidden root
    if( !parent && m_root == invisibleRootItem() )
        parent = m_root;

    return ToItemCompat( parent );
}


wxTreeItemId wxTreeCtrl::GetFirstChild( const wxTreeItemId& aItem, wxTreeItemIdValue& aCookie ) const
{
    aCookie = reinterpret_cast<wxTreeItemIdValue>( std::intptr_t( 0 ) );
    return GetNextChild( aItem, aCookie );
}


wxTreeItemId wxTreeCtrl::GetNextChild( const wxTreeItemId& aItem, wxTreeItemIdValue& aCookie ) const
{
    QTreeWidgetItem* item = ToQtCompat( aItem );
    int              index = int( reinterpret_cast<std::intptr_t>( aCookie ) );

    if( !item || index >= item->childCount() )
        return wxTreeItemId();

    aCookie = reinterpret_cast<wxTreeItemIdValue>( std::intptr_t( index + 1 ) );
    return ToItemCompat( item->child( index ) );
}


wxTreeItemId wxTreeCtrl::GetLastChild( const wxTreeItemId& aItem ) const
{
    QTreeWidgetItem* item = ToQtCompat( aItem );

    if( !item || item->childCount() == 0 )
        return wxTreeItemId();

    return ToItemCompat( item->child( item->childCount() - 1 ) );
}


wxTreeItemId wxTreeCtrl::GetNextSibling( const wxTreeItemId& aItem ) const
{
    QTreeWidgetItem* parent = ToQtCompat( GetItemParent( aItem ) );

    if( !parent )
        return wxTreeItemId();

    return ToItemCompat( parent->child( parent->indexOfChild( ToQtCompat( aItem ) ) + 1 ) );
}


wxTreeItemId wxTreeCtrl::GetPrevSibling( const wxTreeItemId& aItem ) const
{
    QTreeWidgetItem* parent = ToQtCompat( GetItemParent( aItem ) );

    if( !parent )
        return wxTreeItemId();

    int index = parent->indexOfChild( ToQtCompat( aItem ) );
    return index > 0 ? ToItemCompat( parent->child( index - 1 ) ) : wxTreeItemId();
}


wxTreeItemId wxTreeCtrl::GetFirstVisibleItem() const
{
    if( QTreeWidgetItem* item = itemAt( 0, 0 ) )
        return ToItemCompat( item );

    return ToItemCompat( topLevelItem( 0 ) );
}


wxTreeItemId wxTreeCtrl::GetNextVisible( const wxTreeItemId& aItem ) const
{
    QTreeWidgetItem* item = ToQtCompat( aItem );
    return item ? ToItemCompat( itemBelow( item ) ) : wxTreeItemId();
}


size_t wxTreeCtrl::GetChildrenCount( const wxTreeItemId& aItem, bool aRecursively ) const
{
    QTreeWidgetItem* item = ToQtCompat( aItem );

    if( !item )
        return 0;

    size_t count = size_t( item->childCount() );

    if( aRecursively )
    {
        for( int i = 0; i < item->childCount(); ++i )
            count += GetChildrenCount( ToItemCompat( item->child( i ) ), true );
    }

    return count;
}


bool wxTreeCtrl::ItemHasChildren( const wxTreeItemId& aItem ) const
{
    QTreeWidgetItem* item = ToQtCompat( aItem );

    return item && ( item->childCount() > 0
                     || item->childIndicatorPolicy() == QTreeWidgetItem::ShowIndicator );
}


wxTreeItemId wxTreeCtrl::GetSelection() const
{
    QList<QTreeWidgetItem*> selected = selectedItems();

    if( selected.isEmpty() )
        return wxTreeItemId();

    return ToItemCompat( selected.contains( currentItem() ) ? currentItem() : selected.first() );
}


size_t wxTreeCtrl::GetSelections( std::vector<wxTreeItemId>& aSelections ) const
{
    aSelections.clear();

    for( QTreeWidgetItem* item : selectedItems() )
        aSelections.push_back( ToItemCompat( item ) );

    return aSelections.size();
}


void wxTreeCtrl::SelectItem( const wxTreeItemId& aItem, bool aSelect )
{
    QTreeWidgetItem* item = ToQtCompat( aItem );

    if( !item || item == invisibleRootItem() )
        return;

    if( aSelect )
    {
        setCurrentItem( item );
        item->setSelected( true );
    }
    else
    {
        item->setSelected( false );
    }
}


void wxTreeCtrl::Unselect()
{
    clearSelection();
}


void wxTreeCtrl::UnselectAll()
{
    clearSelection();
}


bool wxTreeCtrl::IsSelected( const wxTreeItemId& aItem ) const
{
    QTreeWidgetItem* item = ToQtCompat( aItem );
    return item && item->isSelected();
}


void wxTreeCtrl::Expand( const wxTreeItemId& aItem )
{
    if( QTreeWidgetItem* item = ToQtCompat( aItem ) )
        item->setExpanded( true );
}


void wxTreeCtrl::Collapse( const wxTreeItemId& aItem )
{
    if( QTreeWidgetItem* item = ToQtCompat( aItem ) )
        item->setExpanded( false );
}


void wxTreeCtrl::Toggle( const wxTreeItemId& aItem )
{
    if( QTreeWidgetItem* item = ToQtCompat( aItem ) )
        item->setExpanded( !item->isExpanded() );
}


void wxTreeCtrl::ExpandAllChildren( const wxTreeItemId& aItem )
{
    QTreeWidgetItem* item = ToQtCompat( aItem );

    if( !item )
        return;

    item->setExpanded( true );

    for( int i = 0; i < item->childCount(); ++i )
        ExpandAllChildren( ToItemCompat( item->child( i ) ) );
}


bool wxTreeCtrl::IsExpanded( const wxTreeItemId& aItem ) const
{
    QTreeWidgetItem* item = ToQtCompat( aItem );
    return item && ( item == invisibleRootItem() || item->isExpanded() );
}


bool wxTreeCtrl::IsVisible( const wxTreeItemId& aItem ) const
{
    QTreeWidgetItem* item = ToQtCompat( aItem );

    if( !item || item->isHidden() )
        return false;

    return viewport()->rect().intersects( visualItemRect( item ) );
}


void wxTreeCtrl::EnsureVisible( const wxTreeItemId& aItem )
{
    QTreeWidgetItem* item = ToQtCompat( aItem );

    if( !item || item == invisibleRootItem() )
        return;

    for( QTreeWidgetItem* parent = item->parent(); parent; parent = parent->parent() )
        parent->setExpanded( true );

    scrollToItem( item );
}


wxTreeItemId wxTreeCtrl::HitTest( const wxPoint& aPoint, int& aFlags ) const
{
    QPoint           pos = viewport()->mapFrom( this, QPoint( aPoint.x, aPoint.y ) );
    QTreeWidgetItem* item = itemAt( pos );

    aFlags = item ? wxTREE_HITTEST_ONITEMLABEL : wxTREE_HITTEST_NOWHERE;

    if( item )
    {
        QRect rect = visualItemRect( item );
        aFlags |= ( pos.y() < rect.center().y() ) ? wxTREE_HITTEST_ONITEMUPPERPART
                                                  : wxTREE_HITTEST_ONITEMLOWERPART;
    }

    return ToItemCompat( item );
}


bool wxTreeCtrl::GetBoundingRect( const wxTreeItemId& aItem, wxRect& aRect, bool aTextOnly ) const
{
    QTreeWidgetItem* item = ToQtCompat( aItem );

    if( !item || item == invisibleRootItem() )
        return false;

    QRect rect = visualItemRect( item );

    if( rect.isEmpty() )
        return false;

    rect.translate( viewport()->pos() );
    aRect = wxRect( rect.x(), rect.y(), rect.width(), rect.height() );
    return true;
}


void wxTreeCtrl::SetImageList( wxImageList* aImageList )
{
    m_ownedImageList.reset();
    m_imageList = aImageList;
}


void wxTreeCtrl::AssignImageList( wxImageList* aImageList )
{
    m_ownedImageList.reset( aImageList );
    m_imageList = aImageList;
}
