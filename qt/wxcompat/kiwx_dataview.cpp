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

#include <wx/kiwx_dataview.h>

#include <algorithm>
#include <set>
#include <unordered_map>

#include <QAbstractItemModel>
#include <QApplication>
#include <QHeaderView>
#include <QPainter>
#include <QPointer>
#include <QSignalBlocker>
#include <QStyledItemDelegate>

#include <qt_strings.h>


// The values of the variants ---------------------------------------------------------------

namespace
{
/// The data of a variant holding a wxDataViewIconText.
class ICON_TEXT_VARIANT_DATA : public wxVariantData
{
public:
    explicit ICON_TEXT_VARIANT_DATA( const wxDataViewIconText& aValue ) : m_value( aValue ) {}

    bool Eq( wxVariantData& aData ) const override
    {
        return aData.GetType() == GetType()
               && static_cast<ICON_TEXT_VARIANT_DATA&>( aData ).m_value == m_value;
    }

    wxString GetType() const override { return wxS( "wxDataViewIconText" ); }
    wxVariantData* Clone() const override { return new ICON_TEXT_VARIANT_DATA( m_value ); }

    wxDataViewIconText m_value;
};


/// The text of a value (the text of an icon and text).
wxString variantText( const wxVariant& aValue )
{
    if( aValue.IsNull() )
        return wxString();

    if( aValue.GetType() == wxS( "wxDataViewIconText" ) )
    {
        wxDataViewIconText iconText;
        iconText << aValue;
        return iconText.GetText();
    }

    if( aValue.GetType() == wxS( "bool" ) )
        return aValue.GetBool() ? wxS( "1" ) : wxS( "0" );

    return aValue.GetString();
}
} // namespace


wxDataViewIconText& operator<<( wxDataViewIconText& aValue, const wxVariant& aVariant )
{
    if( aVariant.GetType() == wxS( "wxDataViewIconText" ) )
        aValue = static_cast<ICON_TEXT_VARIANT_DATA*>( aVariant.GetData() )->m_value;
    else
        aValue = wxDataViewIconText( aVariant.IsNull() ? wxString() : aVariant.GetString() );

    return aValue;
}


wxVariant& operator<<( wxVariant& aVariant, const wxDataViewIconText& aValue )
{
    aVariant.SetData( new ICON_TEXT_VARIANT_DATA( aValue ) );
    return aVariant;
}


wxBitmap wxDataViewIconText::GetIcon() const
{
    wxSize size = m_icon.GetDefaultSize();
    return m_icon.GetBitmap( size );
}


wxFont wxDataViewItemAttr::GetEffectiveFont( const wxFont& aFont ) const
{
    wxFont font = aFont;

    if( m_bold )
        font.setBold( true );

    if( m_italic )
        font.setItalic( true );

    if( m_strikethrough )
        font.setStrikeOut( true );

    return font;
}


// The models of wxWidgets ------------------------------------------------------------------

bool wxDataViewModelNotifier::ItemsAdded( const wxDataViewItem& aParent,
                                          const wxDataViewItemArray& aItems )
{
    for( const wxDataViewItem& item : aItems )
    {
        if( !ItemAdded( aParent, item ) )
            return false;
    }

    return true;
}


bool wxDataViewModelNotifier::ItemsDeleted( const wxDataViewItem& aParent,
                                            const wxDataViewItemArray& aItems )
{
    for( const wxDataViewItem& item : aItems )
    {
        if( !ItemDeleted( aParent, item ) )
            return false;
    }

    return true;
}


bool wxDataViewModelNotifier::ItemsChanged( const wxDataViewItemArray& aItems )
{
    for( const wxDataViewItem& item : aItems )
    {
        if( !ItemChanged( item ) )
            return false;
    }

    return true;
}


wxDataViewModel::~wxDataViewModel()
{
    // The model owns its notifiers, as with wxWidgets
    std::vector<wxDataViewModelNotifier*> notifiers;
    notifiers.swap( m_notifiers );

    for( wxDataViewModelNotifier* notifier : notifiers )
        delete notifier;
}


void wxDataViewModel::AddNotifier( wxDataViewModelNotifier* aNotifier )
{
    aNotifier->SetOwner( this );
    m_notifiers.push_back( aNotifier );
}


void wxDataViewModel::RemoveNotifier( wxDataViewModelNotifier* aNotifier )
{
    auto it = std::find( m_notifiers.begin(), m_notifiers.end(), aNotifier );

    if( it != m_notifiers.end() )
    {
        m_notifiers.erase( it );
        delete aNotifier;
    }
}


#define NOTIFY( call )                                                                          \
    {                                                                                           \
        bool result = true;                                                                     \
        std::vector<wxDataViewModelNotifier*> notifiers = m_notifiers;                         \
                                                                                                \
        for( wxDataViewModelNotifier* notifier : notifiers )                                    \
            result &= notifier->call;                                                           \
                                                                                                \
        return result;                                                                          \
    }

bool wxDataViewModel::ItemAdded( const wxDataViewItem& aParent, const wxDataViewItem& aItem )
NOTIFY( ItemAdded( aParent, aItem ) )

bool wxDataViewModel::ItemsAdded( const wxDataViewItem& aParent, const wxDataViewItemArray& aItems )
NOTIFY( ItemsAdded( aParent, aItems ) )

bool wxDataViewModel::ItemDeleted( const wxDataViewItem& aParent, const wxDataViewItem& aItem )
NOTIFY( ItemDeleted( aParent, aItem ) )

bool wxDataViewModel::ItemsDeleted( const wxDataViewItem& aParent, const wxDataViewItemArray& aItems )
NOTIFY( ItemsDeleted( aParent, aItems ) )

bool wxDataViewModel::ItemChanged( const wxDataViewItem& aItem )
NOTIFY( ItemChanged( aItem ) )

bool wxDataViewModel::ItemsChanged( const wxDataViewItemArray& aItems )
NOTIFY( ItemsChanged( aItems ) )

bool wxDataViewModel::ValueChanged( const wxDataViewItem& aItem, unsigned int aCol )
NOTIFY( ValueChanged( aItem, aCol ) )

bool wxDataViewModel::Cleared()
NOTIFY( Cleared() )

bool wxDataViewModel::BeforeReset()
NOTIFY( BeforeReset() )

bool wxDataViewModel::AfterReset()
NOTIFY( AfterReset() )

#undef NOTIFY


void wxDataViewModel::Resort()
{
    std::vector<wxDataViewModelNotifier*> notifiers = m_notifiers;

    for( wxDataViewModelNotifier* notifier : notifiers )
        notifier->Resort();
}


int wxDataViewModel::Compare( const wxDataViewItem& aItem1, const wxDataViewItem& aItem2,
                              unsigned int aColumn, bool aAscending ) const
{
    wxVariant value1;
    wxVariant value2;
    GetValue( value1, aItem1, aColumn );
    GetValue( value2, aItem2, aColumn );

    if( !aAscending )
        std::swap( value1, value2 );

    if( int result = DoCompareValues( value1, value2 ) )
        return result;

    if( value1.GetType() == wxS( "long" ) || value1.GetType() == wxS( "double" ) )
    {
        double d1 = value1.GetDouble();
        double d2 = value2.GetDouble();
        return d1 < d2 ? -1 : d1 > d2 ? 1 : 0;
    }

    if( value1.GetType() == wxS( "bool" ) )
        return int( value1.GetBool() ) - int( value2.GetBool() );

    if( int result = variantText( value1 ).CmpNoCase( variantText( value2 ) ) )
        return result;

    // The order of the ids makes the sort stable
    uintptr_t id1 = uintptr_t( aItem1.GetID() );
    uintptr_t id2 = uintptr_t( aItem2.GetID() );
    return aAscending ? ( id1 < id2 ? -1 : id1 > id2 ) : ( id2 < id1 ? -1 : id2 > id1 );
}


// The list models: the ids of the items are the rows plus one ------------------------------

void wxDataViewListModel::RowInserted( unsigned int aBefore )
{
    resizeCompat( GetCount() + 1 );
    ItemAdded( wxDataViewItem(), GetItem( aBefore ) );
}


void wxDataViewListModel::RowDeleted( unsigned int aRow )
{
    wxDataViewItem item = GetItem( aRow );
    resizeCompat( GetCount() > 0 ? GetCount() - 1 : 0 );
    ItemDeleted( wxDataViewItem(), item );
}


void wxDataViewListModel::RowsDeleted( const wxArrayInt& aRows )
{
    wxDataViewItemArray items;

    for( int row : aRows )
        items.push_back( GetItem( row ) );

    resizeCompat( GetCount() >= aRows.size() ? unsigned( GetCount() - aRows.size() ) : 0 );
    ItemsDeleted( wxDataViewItem(), items );
}


void wxDataViewListModel::RowChanged( unsigned int aRow )
{
    ItemChanged( GetItem( aRow ) );
}


void wxDataViewListModel::RowValueChanged( unsigned int aRow, unsigned int aCol )
{
    ValueChanged( GetItem( aRow ), aCol );
}


void wxDataViewListModel::Reset( unsigned int aNewSize )
{
    BeforeReset();
    resizeCompat( aNewSize );
    AfterReset();
}


unsigned wxDataViewIndexListModel::GetRow( const wxDataViewItem& aItem ) const
{
    return aItem.IsOk() ? unsigned( uintptr_t( aItem.GetID() ) - 1 ) : 0;
}


wxDataViewItem wxDataViewIndexListModel::GetItem( unsigned int aRow ) const
{
    return wxDataViewItem( reinterpret_cast<void*>( uintptr_t( aRow ) + 1 ) );
}


unsigned int wxDataViewIndexListModel::GetChildren( const wxDataViewItem& aItem,
                                                    wxDataViewItemArray& aChildren ) const
{
    aChildren.clear();

    if( aItem.IsOk() )
        return 0;

    for( unsigned int row = 0; row < GetCount(); ++row )
        aChildren.push_back( GetItem( row ) );

    return GetCount();
}


// The model of Qt of a data view -----------------------------------------------------------

/// The items of the model of wxWidgets of a data view, for its QTreeView.
class KIWX_DATAVIEW_MODEL : public QAbstractItemModel
{
public:
    explicit KIWX_DATAVIEW_MODEL( wxDataViewCtrl* aCtrl ) :
            QAbstractItemModel( aCtrl ),
            m_ctrl( aCtrl )
    {
    }

    wxDataViewModel* model() const { return m_ctrl->m_model; }

    bool isList() const { return model() && model()->IsListModel(); }

    wxDataViewListModel* listModel() const { return static_cast<wxDataViewListModel*>( model() ); }

    /// The children of an item of a tree (loaded once, sorted as the control).
    const std::vector<void*>& children( void* aParent ) const
    {
        auto it = m_children.find( aParent );

        if( it != m_children.end() )
            return it->second;

        wxDataViewItemArray items;
        model()->GetChildren( wxDataViewItem( aParent ), items );
        sortItems( items );

        std::vector<void*>& result = m_children[aParent];

        for( const wxDataViewItem& item : items )
            result.push_back( item.GetID() );

        return result;
    }

    void sortItems( wxDataViewItemArray& aItems ) const
    {
        wxDataViewColumn* column = m_ctrl->GetSortingColumn();
        wxDataViewModel*  wxModel = model();

        if( column )
        {
            unsigned int col = column->GetModelColumn();
            bool         ascending = column->IsSortOrderAscending();

            std::stable_sort( aItems.begin(), aItems.end(),
                              [&]( const wxDataViewItem& a, const wxDataViewItem& b )
                              {
                                  return wxModel->Compare( a, b, col, ascending ) < 0;
                              } );
        }
        else if( wxModel->HasDefaultCompare() )
        {
            std::stable_sort( aItems.begin(), aItems.end(),
                              [&]( const wxDataViewItem& a, const wxDataViewItem& b )
                              {
                                  return wxModel->Compare( a, b, unsigned( -1 ), true ) < 0;
                              } );
        }
    }

    void clearCache() { m_children.clear(); }

    /// Forget the children of an item and of its descendants.
    void forget( void* aItem )
    {
        auto it = m_children.find( aItem );

        if( it == m_children.end() )
            return;

        std::vector<void*> children = it->second;
        m_children.erase( it );

        for( void* child : children )
            forget( child );
    }

    QModelIndex indexOf( const wxDataViewItem& aItem, int aColumn = 0 ) const
    {
        if( !aItem.IsOk() || !model() )
            return QModelIndex();

        if( isList() )
        {
            unsigned row = listModel()->GetRow( aItem );
            return row < listModel()->GetCount() ? createIndex( int( row ), aColumn, aItem.GetID() )
                                                 : QModelIndex();
        }

        wxDataViewItem            parent = model()->GetParent( aItem );
        const std::vector<void*>& siblings = children( parent.GetID() );
        auto                      it = std::find( siblings.begin(), siblings.end(), aItem.GetID() );

        if( it == siblings.end() )
            return QModelIndex();

        return createIndex( int( it - siblings.begin() ), aColumn, aItem.GetID() );
    }

    // QAbstractItemModel

    QModelIndex index( int aRow, int aColumn, const QModelIndex& aParent = QModelIndex() ) const override
    {
        if( !model() || aRow < 0 || aColumn < 0 || aColumn >= columnCount() )
            return QModelIndex();

        if( isList() )
        {
            if( aParent.isValid() || unsigned( aRow ) >= listModel()->GetCount() )
                return QModelIndex();

            return createIndex( aRow, aColumn, listModel()->GetItem( aRow ).GetID() );
        }

        const std::vector<void*>& items = children( aParent.isValid() ? aParent.internalPointer()
                                                                       : nullptr );

        if( aRow >= int( items.size() ) )
            return QModelIndex();

        return createIndex( aRow, aColumn, items[aRow] );
    }

    QModelIndex parent( const QModelIndex& aIndex ) const override
    {
        if( !aIndex.isValid() || !model() || isList() )
            return QModelIndex();

        return indexOf( model()->GetParent( wxDataViewItem( aIndex.internalPointer() ) ) );
    }

    int rowCount( const QModelIndex& aParent = QModelIndex() ) const override
    {
        if( !model() || aParent.column() > 0 )
            return 0;

        if( isList() )
            return aParent.isValid() ? 0 : int( listModel()->GetCount() );

        void* parent = aParent.isValid() ? aParent.internalPointer() : nullptr;

        if( parent && !model()->IsContainer( wxDataViewItem( parent ) ) )
            return 0;

        return int( children( parent ).size() );
    }

    bool hasChildren( const QModelIndex& aParent = QModelIndex() ) const override
    {
        if( !model() || aParent.column() > 0 )
            return false;

        if( !aParent.isValid() )
            return rowCount() > 0;

        if( isList() )
            return false;

        return model()->IsContainer( wxDataViewItem( aParent.internalPointer() ) );
    }

    int columnCount( const QModelIndex& aParent = QModelIndex() ) const override
    {
        return int( m_ctrl->m_columns.size() );
    }

    QVariant data( const QModelIndex& aIndex, int aRole ) const override
    {
        if( !aIndex.isValid() || !model() || aIndex.column() >= columnCount() )
            return QVariant();

        wxDataViewColumn*   column = m_ctrl->m_columns[aIndex.column()];
        wxDataViewRenderer* renderer = column->GetRenderer();
        unsigned int        col = column->GetModelColumn();
        wxDataViewItem      item( aIndex.internalPointer() );

        if( aRole == Qt::TextAlignmentRole )
            return int( alignment( column ) );

        if( !model()->HasValue( item, col ) )
            return QVariant();

        if( aRole == Qt::FontRole || aRole == Qt::ForegroundRole || aRole == Qt::BackgroundRole )
        {
            wxDataViewItemAttr attr;

            if( !model()->GetAttr( item, col, attr ) )
                return QVariant();

            if( aRole == Qt::FontRole && attr.HasFont() )
                return QFont( attr.GetEffectiveFont( m_ctrl->GetFont() ) );

            if( aRole == Qt::ForegroundRole && attr.HasColour() )
                return QColor( attr.GetColour() );

            if( aRole == Qt::BackgroundRole && attr.HasBackgroundColour() )
                return QColor( attr.GetBackgroundColour() );

            return QVariant();
        }

        if( aRole != Qt::DisplayRole && aRole != Qt::EditRole && aRole != Qt::DecorationRole
            && aRole != Qt::CheckStateRole && aRole != Qt::ToolTipRole )
        {
            return QVariant();
        }

        wxVariant value;
        model()->GetValue( value, item, col );

        switch( renderer->KindCompat() )
        {
        case wxDataViewRenderer::TOGGLE:
            if( aRole == Qt::CheckStateRole && !value.IsNull() )
                return value.GetBool() ? Qt::Checked : Qt::Unchecked;

            return QVariant();

        case wxDataViewRenderer::ICONTEXT:
            if( aRole == Qt::DecorationRole && value.GetType() == wxS( "wxDataViewIconText" ) )
            {
                wxDataViewIconText iconText;
                iconText << value;

                if( iconText.GetBitmapBundle().IsOk() )
                    return QIcon( iconText.GetBitmapBundle() );
            }

            break;

        case wxDataViewRenderer::BITMAP:
            return QVariant();

        case wxDataViewRenderer::CUSTOM:
            // Drawn by the renderer (the delegate)
            return aRole == Qt::ToolTipRole ? QVariant() : QVariant();

        default:
            break;
        }

        if( aRole == Qt::DisplayRole || aRole == Qt::EditRole )
            return ToQString( variantText( value ) );

        return QVariant();
    }

    bool setData( const QModelIndex& aIndex, const QVariant& aValue, int aRole ) override
    {
        if( !aIndex.isValid() || !model() )
            return false;

        wxDataViewColumn* column = m_ctrl->m_columns[aIndex.column()];
        wxDataViewItem    item( aIndex.internalPointer() );
        wxVariant         value;

        if( aRole == Qt::CheckStateRole )
            value = wxVariant( aValue.toInt() == Qt::Checked );
        else if( aRole == Qt::EditRole )
            value = wxVariant( ToWxString( aValue.toString() ) );
        else
            return false;

        if( !model()->ChangeValue( value, item, column->GetModelColumn() ) )
            return false;

        m_ctrl->SendEventCompat( wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, item, aIndex.column(), &value );
        return true;
    }

    Qt::ItemFlags flags( const QModelIndex& aIndex ) const override
    {
        if( !aIndex.isValid() || !model() )
            return Qt::NoItemFlags;

        wxDataViewColumn*   column = m_ctrl->m_columns[aIndex.column()];
        wxDataViewRenderer* renderer = column->GetRenderer();
        wxDataViewItem      item( aIndex.internalPointer() );
        Qt::ItemFlags       result = Qt::ItemIsSelectable;

        if( model()->IsEnabled( item, column->GetModelColumn() ) )
            result |= Qt::ItemIsEnabled;

        if( renderer->GetMode() != wxDATAVIEW_CELL_INERT
            && renderer->KindCompat() == wxDataViewRenderer::TOGGLE )
        {
            result |= Qt::ItemIsUserCheckable;
        }

        if( renderer->GetMode() == wxDATAVIEW_CELL_EDITABLE
            && renderer->KindCompat() == wxDataViewRenderer::TEXT )
        {
            result |= Qt::ItemIsEditable;
        }

        if( isList() || !model()->IsContainer( item ) )
            result |= Qt::ItemNeverHasChildren;

        return result;
    }

    QVariant headerData( int aSection, Qt::Orientation aOrientation, int aRole ) const override
    {
        if( aOrientation != Qt::Horizontal || aSection >= columnCount() )
            return QVariant();

        wxDataViewColumn* column = m_ctrl->m_columns[aSection];

        if( aRole == Qt::DisplayRole )
            return ToQString( column->GetTitle() );

        if( aRole == Qt::TextAlignmentRole )
            return int( alignment( column ) );

        return QVariant();
    }

    static Qt::Alignment alignment( wxDataViewColumn* aColumn )
    {
        int align = aColumn->GetRenderer()->GetAlignment();

        if( align == wxDVR_DEFAULT_ALIGNMENT )
            align = aColumn->GetAlignment();

        Qt::Alignment result = Qt::AlignVCenter;

        if( align & wxALIGN_RIGHT )
            result |= Qt::AlignRight;
        else if( align & wxALIGN_CENTER_HORIZONTAL )
            result |= Qt::AlignHCenter;
        else
            result |= Qt::AlignLeft;

        return result;
    }

    // The notifications of the model of wxWidgets

    void itemAdded( const wxDataViewItem& aParent, const wxDataViewItem& aItem )
    {
        if( isList() )
        {
            unsigned row = listModel()->GetRow( aItem );
            beginInsertRows( QModelIndex(), int( row ), int( row ) );
            endInsertRows();
            return;
        }

        // Children not loaded yet: the view will ask for them
        if( !m_children.count( aParent.GetID() ) )
            return;

        QModelIndex parentIndex = indexOf( aParent );

        // Only the parents seen by the view (the root, or an item with an index)
        if( aParent.IsOk() && !parentIndex.isValid() )
        {
            forget( aParent.GetID() );
            return;
        }

        wxDataViewItemArray items;
        model()->GetChildren( aParent, items );
        sortItems( items );

        auto it = std::find( items.begin(), items.end(), aItem );
        int  row = it == items.end() ? int( items.size() ) - 1 : int( it - items.begin() );

        beginInsertRows( parentIndex, row, row );

        std::vector<void*>& cache = m_children[aParent.GetID()];
        cache.clear();

        for( const wxDataViewItem& item : items )
            cache.push_back( item.GetID() );

        endInsertRows();
    }

    void itemDeleted( const wxDataViewItem& aParent, const wxDataViewItem& aItem )
    {
        if( isList() )
        {
            // The rows after the deleted one have new ids
            resetPreserving( false );
            return;
        }

        auto cache = m_children.find( aParent.GetID() );

        if( cache == m_children.end() )
            return;

        auto it = std::find( cache->second.begin(), cache->second.end(), aItem.GetID() );

        if( it == cache->second.end() )
            return;

        int         row = int( it - cache->second.begin() );
        QModelIndex parentIndex = aParent.IsOk() ? indexOf( aParent ) : QModelIndex();

        if( aParent.IsOk() && !parentIndex.isValid() )
        {
            forget( aParent.GetID() );
            return;
        }

        beginRemoveRows( parentIndex, row, row );
        cache = m_children.find( aParent.GetID() );
        cache->second.erase( cache->second.begin() + row );
        forget( aItem.GetID() );
        endRemoveRows();
    }

    void itemChanged( const wxDataViewItem& aItem )
    {
        QModelIndex first = indexOf( aItem, 0 );

        if( first.isValid() && columnCount() > 0 )
            emit dataChanged( first, indexOf( aItem, columnCount() - 1 ) );
    }

    void beforeReset()
    {
        if( !m_resetting )
        {
            m_resetting = true;
            beginResetModel();
        }
    }

    void afterReset()
    {
        if( !m_resetting )
            beginResetModel();

        m_children.clear();
        endResetModel();
        m_resetting = false;
    }

    /**
     * A reset of the model that keeps the expanded items, the selection and the current item
     * (a new order of the items).  The items must all still exist.
     */
    void resetPreserving( bool aKeepState )
    {
        std::vector<void*>  expanded;
        wxDataViewItemArray selection;
        wxDataViewItem      current;

        if( aKeepState && !isList() )
        {
            for( const auto& [parent, items] : m_children )
            {
                for( void* item : items )
                {
                    QModelIndex index = indexOf( wxDataViewItem( item ) );

                    if( index.isValid() && m_ctrl->isExpanded( index ) )
                        expanded.push_back( item );
                }
            }

            m_ctrl->GetSelections( selection );
            current = m_ctrl->GetCurrentItem();
        }

        beginResetModel();
        m_children.clear();
        endResetModel();

        if( aKeepState )
        {
            for( void* item : expanded )
                m_ctrl->expand( indexOf( wxDataViewItem( item ) ) );

            m_ctrl->SetSelections( selection );

            if( current.IsOk() )
                m_ctrl->SetCurrentItem( current );
        }
    }

    using QAbstractItemModel::beginResetModel;
    using QAbstractItemModel::endResetModel;
    using QAbstractItemModel::beginInsertColumns;
    using QAbstractItemModel::endInsertColumns;
    using QAbstractItemModel::beginRemoveColumns;
    using QAbstractItemModel::endRemoveColumns;

private:
    wxDataViewCtrl*                                      m_ctrl;
    mutable std::unordered_map<void*, std::vector<void*>> m_children;
    bool                                                 m_resetting = false;
};


namespace
{
/// The notifier of a model to the model of Qt of a control (deleted by the model).
class KIWX_DATAVIEW_NOTIFIER : public wxDataViewModelNotifier
{
public:
    explicit KIWX_DATAVIEW_NOTIFIER( KIWX_DATAVIEW_MODEL* aModel ) : m_model( aModel ) {}

    bool ItemAdded( const wxDataViewItem& aParent, const wxDataViewItem& aItem ) override
    {
        if( m_model )
            m_model->itemAdded( aParent, aItem );

        return true;
    }

    bool ItemDeleted( const wxDataViewItem& aParent, const wxDataViewItem& aItem ) override
    {
        if( m_model )
            m_model->itemDeleted( aParent, aItem );

        return true;
    }

    bool ItemChanged( const wxDataViewItem& aItem ) override
    {
        if( m_model )
            m_model->itemChanged( aItem );

        return true;
    }

    bool ValueChanged( const wxDataViewItem& aItem, unsigned int aCol ) override
    {
        return ItemChanged( aItem );
    }

    bool Cleared() override
    {
        if( m_model )
            m_model->afterReset();

        return true;
    }

    bool BeforeReset() override
    {
        if( m_model )
            m_model->beforeReset();

        return true;
    }

    bool AfterReset() override { return Cleared(); }

    void Resort() override
    {
        if( m_model )
            m_model->resetPreserving( true );
    }

private:
    QPointer<KIWX_DATAVIEW_MODEL> m_model;
};


/// The cells of the custom renderers, drawn by them.
class KIWX_DATAVIEW_DELEGATE : public QStyledItemDelegate
{
public:
    explicit KIWX_DATAVIEW_DELEGATE( wxDataViewCtrl* aCtrl ) :
            QStyledItemDelegate( aCtrl ),
            m_ctrl( aCtrl )
    {
    }

    wxDataViewCustomRenderer* customRenderer( const QModelIndex& aIndex ) const
    {
        wxDataViewColumn* column = m_ctrl->GetColumn( unsigned( aIndex.column() ) );

        if( !column || !column->GetRenderer()->IsCustomRenderer() )
            return nullptr;

        return static_cast<wxDataViewCustomRenderer*>( column->GetRenderer() );
    }

    /// Give the value and the attributes of a cell to its renderer.
    void prepare( wxDataViewRenderer* aRenderer, const QModelIndex& aIndex ) const
    {
        wxDataViewModel*  model = m_ctrl->GetModel();
        wxDataViewColumn* column = m_ctrl->GetColumn( unsigned( aIndex.column() ) );
        wxDataViewItem    item( aIndex.internalPointer() );
        unsigned int      col = column->GetModelColumn();
        wxVariant         value;

        if( model->HasValue( item, col ) )
            model->GetValue( value, item, col );

        wxDataViewItemAttr attr;
        model->GetAttr( item, col, attr );

        aRenderer->SetValue( value );
        aRenderer->SetAttr( attr );
        aRenderer->SetEnabled( model->IsEnabled( item, col ) );
    }

    void paint( QPainter* aPainter, const QStyleOptionViewItem& aOption,
                const QModelIndex& aIndex ) const override
    {
        wxDataViewCustomRenderer* renderer = m_ctrl->GetModel() ? customRenderer( aIndex ) : nullptr;

        if( !renderer )
        {
            QStyledItemDelegate::paint( aPainter, aOption, aIndex );
            return;
        }

        // The background of the cell (the selection), then the renderer
        QStyleOptionViewItem option = aOption;
        initStyleOption( &option, aIndex );
        option.text.clear();
        option.icon = QIcon();

        const QWidget* widget = option.widget;
        QStyle*        style = widget ? widget->style() : QApplication::style();
        style->drawControl( QStyle::CE_ItemViewItem, &option, aPainter, widget );

        prepare( renderer, aIndex );

        int state = 0;

        if( aOption.state & QStyle::State_Selected )
            state |= wxDATAVIEW_CELL_SELECTED;

        if( aOption.state & QStyle::State_HasFocus )
            state |= wxDATAVIEW_CELL_FOCUSED;

        if( !( aOption.state & QStyle::State_Enabled ) )
            state |= wxDATAVIEW_CELL_INSENSITIVE;

        aPainter->save();

        wxDC   dc( aPainter, m_ctrl->viewport() );
        QRect  r = aOption.rect;
        wxRect rect( r.x(), r.y(), r.width(), r.height() );

        dc.SetFont( m_ctrl->GetFont() );
        dc.SetClippingRegion( rect );
        renderer->SetDCCompat( &dc );
        renderer->Render( rect, &dc, state );
        renderer->SetDCCompat( nullptr );

        aPainter->restore();
    }

    QSize sizeHint( const QStyleOptionViewItem& aOption, const QModelIndex& aIndex ) const override
    {
        wxDataViewCustomRenderer* renderer = m_ctrl->GetModel() ? customRenderer( aIndex ) : nullptr;

        if( !renderer )
            return QStyledItemDelegate::sizeHint( aOption, aIndex );

        prepare( renderer, aIndex );
        wxSize size = renderer->GetSize();
        return QSize( std::max( size.x, 1 ), std::max( size.y, 1 ) );
    }

private:
    wxDataViewCtrl* m_ctrl;
};
} // namespace


// The renderers ----------------------------------------------------------------------------

void wxDataViewCustomRenderer::RenderText( const wxString& aText, int aXOffset, wxRect aCell,
                                           wxDC* aDC, int aState )
{
    if( !aDC )
        return;

    wxDataViewCtrl* ctrl = GetOwner() ? GetOwner()->GetOwner() : nullptr;
    QPalette        palette = ctrl ? ctrl->palette() : QApplication::palette();
    wxColour        colour = palette.color( QPalette::Text );

    if( aState & wxDATAVIEW_CELL_SELECTED )
        colour = palette.color( QPalette::HighlightedText );
    else if( GetAttr().HasColour() )
        colour = GetAttr().GetColour();

    if( !GetEnabled() )
        colour = palette.color( QPalette::Disabled, QPalette::Text );

    wxFont font = aDC->GetFont();

    if( GetAttr().HasFont() )
        aDC->SetFont( GetAttr().GetEffectiveFont( font ) );

    aDC->SetTextForeground( colour );

    wxRect rect = aCell;
    rect.x += aXOffset;
    rect.width -= aXOffset;
    aDC->DrawLabel( aText, rect, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL );
    aDC->SetFont( font );
}


void wxDataViewCustomRenderer::RenderBackground( wxDC* aDC, const wxRect& aRect )
{
    if( !aDC || !GetAttr().HasBackgroundColour() )
        return;

    wxDCBrushChanger brush( *aDC, wxBrush( GetAttr().GetBackgroundColour() ) );
    wxDCPenChanger   pen( *aDC, *wxTRANSPARENT_PEN );
    aDC->DrawRectangle( aRect );
}


wxSize wxDataViewCustomRenderer::GetTextExtent( const wxString& aText ) const
{
    wxDataViewCtrl* ctrl = GetOwner() ? GetOwner()->GetOwner() : nullptr;
    QFontMetrics    metrics = ctrl ? ctrl->fontMetrics() : QFontMetrics( QApplication::font() );
    QSize           size = metrics.size( Qt::TextSingleLine, ToQString( aText ) );
    return wxSize( size.width(), size.height() );
}


// The columns ------------------------------------------------------------------------------

wxDataViewColumn::wxDataViewColumn( const wxString& aTitle, wxDataViewRenderer* aRenderer,
                                    unsigned int aModelColumn, int aWidth, wxAlignment aAlign,
                                    int aFlags ) :
        m_title( aTitle ),
        m_renderer( aRenderer ),
        m_modelColumn( aModelColumn ),
        m_width( aWidth ),
        m_minWidth( 0 ),
        m_align( aAlign ),
        m_flags( aFlags ),
        m_sortKey( false ),
        m_ascending( true ),
        m_owner( nullptr )
{
    if( m_renderer )
        m_renderer->SetOwner( this );
}


wxDataViewColumn::wxDataViewColumn( const wxBitmapBundle& aBitmap, wxDataViewRenderer* aRenderer,
                                    unsigned int aModelColumn, int aWidth, wxAlignment aAlign,
                                    int aFlags ) :
        wxDataViewColumn( wxEmptyString, aRenderer, aModelColumn, aWidth, aAlign, aFlags )
{
    m_bitmap = aBitmap;
}


wxDataViewColumn::~wxDataViewColumn()
{
    delete m_renderer;
}


void wxDataViewColumn::SetTitle( const wxString& aTitle )
{
    m_title = aTitle;

    if( m_owner )
    {
        int pos = m_owner->GetColumnPosition( this );
        emit m_owner->GetQtModelCompat()->headerDataChanged( Qt::Horizontal, pos, pos );
    }
}


int wxDataViewColumn::GetWidth() const
{
    if( m_owner )
    {
        int pos = m_owner->GetColumnPosition( this );

        if( pos >= 0 )
            return m_owner->columnWidth( pos );
    }

    return m_width;
}


void wxDataViewColumn::SetWidth( int aWidth )
{
    m_width = aWidth;

    if( m_owner )
        m_owner->applyColumn( unsigned( m_owner->GetColumnPosition( this ) ) );
}


void wxDataViewColumn::SetSortable( bool aSortable )
{
    m_flags = aSortable ? ( m_flags | wxDATAVIEW_COL_SORTABLE ) : ( m_flags & ~wxDATAVIEW_COL_SORTABLE );
}


void wxDataViewColumn::SetResizeable( bool aResizable )
{
    m_flags = aResizable ? ( m_flags | wxDATAVIEW_COL_RESIZABLE ) : ( m_flags & ~wxDATAVIEW_COL_RESIZABLE );

    if( m_owner )
        m_owner->applyColumn( unsigned( m_owner->GetColumnPosition( this ) ) );
}


void wxDataViewColumn::SetHidden( bool aHidden )
{
    m_flags = aHidden ? ( m_flags | wxDATAVIEW_COL_HIDDEN ) : ( m_flags & ~wxDATAVIEW_COL_HIDDEN );

    if( m_owner )
        m_owner->applyColumn( unsigned( m_owner->GetColumnPosition( this ) ) );
}


void wxDataViewColumn::SetSortOrder( bool aAscending )
{
    if( m_owner )
    {
        // One sort key
        for( unsigned int i = 0; i < m_owner->GetColumnCount(); ++i )
            m_owner->GetColumn( i )->setSortCompat( false, true );
    }

    m_sortKey = true;
    m_ascending = aAscending;

    if( m_owner )
    {
        m_owner->header()->setSortIndicatorShown( true );
        m_owner->header()->setSortIndicator( m_owner->GetColumnPosition( this ),
                                            aAscending ? Qt::AscendingOrder : Qt::DescendingOrder );

        if( m_owner->GetModel() )
            m_owner->GetQtModelCompat()->resetPreserving( true );
    }
}


void wxDataViewColumn::UnsetAsSortKey()
{
    m_sortKey = false;

    if( m_owner )
    {
        m_owner->header()->setSortIndicatorShown( false );

        if( m_owner->GetModel() )
            m_owner->GetQtModelCompat()->resetPreserving( true );
    }
}


// The events -------------------------------------------------------------------------------

wxDataViewEvent::wxDataViewEvent( wxEventType aType, wxDataViewCtrl* aCtrl,
                                  const wxDataViewItem& aItem ) :
        wxNotifyEvent( aType, aCtrl ? aCtrl->GetId() : 0 ),
        m_item( aItem ),
        m_model( aCtrl ? aCtrl->GetModel() : nullptr )
{
    SetEventObject( aCtrl );
}


wxDataViewEvent::wxDataViewEvent( wxEventType aType, wxDataViewCtrl* aCtrl,
                                  wxDataViewColumn* aColumn, const wxDataViewItem& aItem ) :
        wxDataViewEvent( aType, aCtrl, aItem )
{
    m_dataViewColumn = aColumn;
    m_column = aColumn && aCtrl ? aCtrl->GetColumnPosition( aColumn ) : -1;
}


// wxDataViewCtrl ---------------------------------------------------------------------------

wxDataViewCtrl::wxDataViewCtrl( wxWidgetArgCompat aParent, int aId, const wxPoint& aPos,
                                const wxSize& aSize, long aStyle, const wxValidator& aValidator,
                                const wxString& aName ) :
        wxWindowCompat<QTreeView>( aParent.get() ),
        m_model( nullptr ),
        m_qtModel( new KIWX_DATAVIEW_MODEL( this ) )
{
    SetId( aId );
    wxWindow::SetWindowStyleFlag( aStyle );

    if( !aName.IsEmpty() )
        SetName( aName );

    if( aSize.x > 0 || aSize.y > 0 )
        SetMinSize( aSize );

    setModel( m_qtModel );
    setItemDelegate( new KIWX_DATAVIEW_DELEGATE( this ) );
    setSelectionBehavior( QAbstractItemView::SelectRows );
    setSelectionMode( ( aStyle & wxDV_MULTIPLE ) ? QAbstractItemView::ExtendedSelection
                                                 : QAbstractItemView::SingleSelection );
    setHeaderHidden( aStyle & wxDV_NO_HEADER );
    setAlternatingRowColors( aStyle & wxDV_ROW_LINES );
    setUniformRowHeights( !( aStyle & wxDV_VARIABLE_LINE_HEIGHT ) );
    setAllColumnsShowFocus( true );
    setEditTriggers( QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed );
    header()->setStretchLastSection( true );
    header()->setSectionsMovable( false );

    // The events of wxWidgets
    QObject::connect( selectionModel(), &QItemSelectionModel::selectionChanged, this,
                      [this]() { SendEventCompat( wxEVT_DATAVIEW_SELECTION_CHANGED, GetSelection() ); } );

    QObject::connect( this, &QAbstractItemView::activated, this,
                      [this]( const QModelIndex& aIndex )
                      {
                          SendEventCompat( wxEVT_DATAVIEW_ITEM_ACTIVATED, ItemCompat( aIndex ),
                                           aIndex.column() );
                      } );

    QObject::connect( this, &QTreeView::expanded, this,
                      [this]( const QModelIndex& aIndex )
                      {
                          SendEventCompat( wxEVT_DATAVIEW_ITEM_EXPANDED, ItemCompat( aIndex ) );
                      } );

    QObject::connect( this, &QTreeView::collapsed, this,
                      [this]( const QModelIndex& aIndex )
                      {
                          SendEventCompat( wxEVT_DATAVIEW_ITEM_COLLAPSED, ItemCompat( aIndex ) );
                      } );

    setContextMenuPolicy( Qt::CustomContextMenu );
    QObject::connect( this, &QWidget::customContextMenuRequested, this,
                      [this]( const QPoint& aPos )
                      {
                          QModelIndex     index = indexAt( aPos );
                          wxDataViewEvent event( wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, this,
                                                 GetColumn( unsigned( std::max( index.column(), 0 ) ) ),
                                                 ItemCompat( index ) );
                          QPoint pos = viewport()->mapTo( this, aPos );
                          event.SetPosition( pos.x(), pos.y() );
                          ProcessEvent( event );
                      } );

    QObject::connect( header(), &QHeaderView::sectionClicked, this,
                      [this]( int aSection )
                      {
                          wxDataViewColumn* column = GetColumn( unsigned( aSection ) );

                          if( !column )
                              return;

                          SendEventCompat( wxEVT_DATAVIEW_COLUMN_HEADER_CLICK, wxDataViewItem(),
                                           aSection );

                          // The sortable columns sort the items
                          if( column->IsSortable() )
                          {
                              bool ascending = column->IsSortKey() ? !column->IsSortOrderAscending()
                                                                   : true;
                              column->SetSortOrder( ascending );
                              SendEventCompat( wxEVT_DATAVIEW_COLUMN_SORTED, wxDataViewItem(),
                                               aSection );
                          }
                      } );

    header()->setContextMenuPolicy( Qt::CustomContextMenu );
    QObject::connect( header(), &QWidget::customContextMenuRequested, this,
                      [this]( const QPoint& aPos )
                      {
                          SendEventCompat( wxEVT_DATAVIEW_COLUMN_HEADER_RIGHT_CLICK, wxDataViewItem(),
                                           header()->logicalIndexAt( aPos ) );
                      } );
}


wxDataViewCtrl::~wxDataViewCtrl()
{
    // The model of Qt goes before the model of wxWidgets
    setModel( nullptr );

    if( m_model )
    {
        m_model->RemoveNotifier( m_notifierCompat );
        m_model->DecRef();
        m_model = nullptr;
    }

    for( wxDataViewColumn* column : m_columns )
        delete column;

    m_columns.clear();
}


bool wxDataViewCtrl::AssociateModel( wxDataViewModel* aModel )
{
    m_qtModel->beginResetModel();

    if( m_model )
    {
        m_model->RemoveNotifier( m_notifierCompat );
        m_notifierCompat = nullptr;
        m_model->DecRef();
    }

    m_model = aModel;
    m_qtModel->clearCache();

    if( m_model )
    {
        m_model->IncRef();
        m_notifierCompat = new KIWX_DATAVIEW_NOTIFIER( m_qtModel );
        m_model->AddNotifier( m_notifierCompat );
        setRootIsDecorated( !m_model->IsListModel() );
    }

    m_qtModel->endResetModel();

    for( unsigned int i = 0; i < m_columns.size(); ++i )
        applyColumn( i );

    return true;
}


void wxDataViewCtrl::insertColumn( unsigned int aPos, wxDataViewColumn* aColumn )
{
    aPos = std::min<unsigned>( aPos, unsigned( m_columns.size() ) );
    aColumn->SetOwner( this );

    m_qtModel->beginInsertColumns( QModelIndex(), int( aPos ), int( aPos ) );
    m_columns.insert( m_columns.begin() + aPos, aColumn );
    m_qtModel->endInsertColumns();

    applyColumn( aPos );
}


void wxDataViewCtrl::applyColumn( unsigned int aPos )
{
    if( aPos >= m_columns.size() )
        return;

    wxDataViewColumn* column = m_columns[aPos];
    int               width = column->m_width;

    setColumnHidden( int( aPos ), column->IsHidden() );
    header()->setSectionResizeMode( int( aPos ), column->IsResizeable() ? QHeaderView::Interactive
                                                                        : QHeaderView::Fixed );

    if( width > 0 )
        setColumnWidth( int( aPos ), std::max( width, column->GetMinWidth() ) );
    else if( m_model )
        resizeColumnToContents( int( aPos ) );
}


bool wxDataViewCtrl::AppendColumn( wxDataViewColumn* aColumn )
{
    insertColumn( unsigned( m_columns.size() ), aColumn );
    return true;
}


bool wxDataViewCtrl::PrependColumn( wxDataViewColumn* aColumn )
{
    insertColumn( 0, aColumn );
    return true;
}


bool wxDataViewCtrl::InsertColumn( unsigned int aPos, wxDataViewColumn* aColumn )
{
    insertColumn( aPos, aColumn );
    return true;
}


bool wxDataViewCtrl::DeleteColumn( wxDataViewColumn* aColumn )
{
    int pos = GetColumnPosition( aColumn );

    if( pos < 0 )
        return false;

    m_qtModel->beginRemoveColumns( QModelIndex(), pos, pos );
    m_columns.erase( m_columns.begin() + pos );
    m_qtModel->endRemoveColumns();

    delete aColumn;
    return true;
}


bool wxDataViewCtrl::ClearColumns()
{
    if( m_columns.empty() )
        return true;

    m_qtModel->beginRemoveColumns( QModelIndex(), 0, int( m_columns.size() ) - 1 );
    std::vector<wxDataViewColumn*> columns;
    columns.swap( m_columns );
    m_qtModel->endRemoveColumns();

    for( wxDataViewColumn* column : columns )
        delete column;

    return true;
}


wxDataViewColumn* wxDataViewCtrl::GetColumn( unsigned int aPos ) const
{
    return aPos < m_columns.size() ? m_columns[aPos] : nullptr;
}


int wxDataViewCtrl::GetColumnPosition( const wxDataViewColumn* aColumn ) const
{
    auto it = std::find( m_columns.begin(), m_columns.end(), aColumn );
    return it == m_columns.end() ? -1 : int( it - m_columns.begin() );
}


wxDataViewColumn* wxDataViewCtrl::GetSortingColumn() const
{
    for( wxDataViewColumn* column : m_columns )
    {
        if( column->IsSortKey() )
            return column;
    }

    return nullptr;
}


wxDataViewColumn* wxDataViewCtrl::GetCurrentColumn() const
{
    return currentIndex().isValid() ? GetColumn( unsigned( currentIndex().column() ) ) : nullptr;
}


wxDataViewColumn* wxDataViewCtrl::AppendTextColumn( const wxString& aLabel, unsigned int aModelColumn,
                                                    wxDataViewCellMode aMode, int aWidth,
                                                    wxAlignment aAlign, int aFlags )
{
    wxDataViewColumn* column = new wxDataViewColumn(
            aLabel, new wxDataViewTextRenderer( wxS( "string" ), aMode ), aModelColumn, aWidth,
            aAlign == wxALIGN_NOT ? wxALIGN_LEFT : aAlign, aFlags );
    AppendColumn( column );
    return column;
}


wxDataViewColumn* wxDataViewCtrl::AppendToggleColumn( const wxString& aLabel, unsigned int aModelColumn,
                                                      wxDataViewCellMode aMode, int aWidth,
                                                      wxAlignment aAlign, int aFlags )
{
    wxDataViewColumn* column = new wxDataViewColumn(
            aLabel, new wxDataViewToggleRenderer( wxS( "bool" ), aMode ), aModelColumn, aWidth,
            aAlign, aFlags );
    AppendColumn( column );
    return column;
}


wxDataViewColumn* wxDataViewCtrl::AppendIconTextColumn( const wxString& aLabel,
                                                        unsigned int aModelColumn,
                                                        wxDataViewCellMode aMode, int aWidth,
                                                        wxAlignment aAlign, int aFlags )
{
    wxDataViewColumn* column = new wxDataViewColumn(
            aLabel, new wxDataViewIconTextRenderer( wxS( "wxDataViewIconText" ), aMode ),
            aModelColumn, aWidth, aAlign == wxALIGN_NOT ? wxALIGN_LEFT : aAlign, aFlags );
    AppendColumn( column );
    return column;
}


wxDataViewColumn* wxDataViewCtrl::AppendBitmapColumn( const wxString& aLabel, unsigned int aModelColumn,
                                                      wxDataViewCellMode aMode, int aWidth,
                                                      wxAlignment aAlign, int aFlags )
{
    wxDataViewColumn* column = new wxDataViewColumn(
            aLabel, new wxDataViewBitmapRenderer( wxS( "wxBitmapBundle" ), aMode ), aModelColumn,
            aWidth, aAlign, aFlags );
    AppendColumn( column );
    return column;
}


bool wxDataViewCtrl::SetRowHeight( int aHeight )
{
    setUniformRowHeights( true );
    setStyleSheet( QStringLiteral( "QTreeView::item { height: %1px; }" ).arg( aHeight ) );
    return true;
}


QModelIndex wxDataViewCtrl::IndexCompat( const wxDataViewItem& aItem, int aColumn ) const
{
    return m_qtModel->indexOf( aItem, aColumn );
}


wxDataViewItem wxDataViewCtrl::ItemCompat( const QModelIndex& aIndex ) const
{
    return aIndex.isValid() ? wxDataViewItem( aIndex.internalPointer() ) : wxDataViewItem();
}


bool wxDataViewCtrl::SendEventCompat( wxEventType aType, const wxDataViewItem& aItem, int aColumn,
                                      const wxVariant* aValue )
{
    wxDataViewEvent event( aType, this, aColumn >= 0 ? GetColumn( unsigned( aColumn ) ) : nullptr,
                           aItem );

    if( aColumn >= 0 )
        event.SetColumn( aColumn );

    if( aValue )
        event.SetValue( *aValue );

    ProcessEvent( event );
    return event.IsAllowed();
}


wxDataViewItem wxDataViewCtrl::GetCurrentItem() const
{
    return ItemCompat( currentIndex() );
}


void wxDataViewCtrl::SetCurrentItem( const wxDataViewItem& aItem )
{
    QModelIndex index = IndexCompat( aItem );

    if( index.isValid() )
        selectionModel()->setCurrentIndex( index, QItemSelectionModel::NoUpdate );
}


wxDataViewItem wxDataViewCtrl::GetSelection() const
{
    QModelIndexList rows = selectionModel()->selectedRows();
    return rows.isEmpty() ? wxDataViewItem() : ItemCompat( rows.front() );
}


int wxDataViewCtrl::GetSelections( wxDataViewItemArray& aSelections ) const
{
    aSelections.clear();

    for( const QModelIndex& index : selectionModel()->selectedRows() )
        aSelections.push_back( ItemCompat( index ) );

    return int( aSelections.size() );
}


void wxDataViewCtrl::SetSelections( const wxDataViewItemArray& aSelections )
{
    QItemSelection selection;

    for( const wxDataViewItem& item : aSelections )
    {
        ExpandAncestors( item );
        QModelIndex index = IndexCompat( item );

        if( index.isValid() )
            selection.select( index, index );
    }

    // No selection events, as with wxWidgets
    QSignalBlocker blocker( selectionModel() );
    selectionModel()->select( selection, QItemSelectionModel::ClearAndSelect
                                                 | QItemSelectionModel::Rows );
    viewport()->update();
}


int wxDataViewCtrl::GetSelectedItemsCount() const
{
    return int( selectionModel()->selectedRows().size() );
}


void wxDataViewCtrl::Select( const wxDataViewItem& aItem )
{
    ExpandAncestors( aItem );
    QModelIndex index = IndexCompat( aItem );

    if( !index.isValid() )
        return;

    QSignalBlocker blocker( selectionModel() );
    QItemSelectionModel::SelectionFlags flags =
            selectionMode() == QAbstractItemView::SingleSelection ? QItemSelectionModel::ClearAndSelect
                                                                  : QItemSelectionModel::Select;
    selectionModel()->select( index, flags | QItemSelectionModel::Rows );
    selectionModel()->setCurrentIndex( index, QItemSelectionModel::NoUpdate );
    viewport()->update();
}


void wxDataViewCtrl::Unselect( const wxDataViewItem& aItem )
{
    QModelIndex index = IndexCompat( aItem );

    if( !index.isValid() )
        return;

    QSignalBlocker blocker( selectionModel() );
    selectionModel()->select( index, QItemSelectionModel::Deselect | QItemSelectionModel::Rows );
    viewport()->update();
}


bool wxDataViewCtrl::IsSelected( const wxDataViewItem& aItem ) const
{
    QModelIndex index = IndexCompat( aItem );
    return index.isValid() && selectionModel()->isRowSelected( index.row(), index.parent() );
}


void wxDataViewCtrl::SelectAll()
{
    QSignalBlocker blocker( selectionModel() );
    selectAll();
    viewport()->update();
}


void wxDataViewCtrl::UnselectAll()
{
    QSignalBlocker blocker( selectionModel() );
    clearSelection();
    viewport()->update();
}


void wxDataViewCtrl::Expand( const wxDataViewItem& aItem )
{
    ExpandAncestors( aItem );
    expand( IndexCompat( aItem ) );
}


void wxDataViewCtrl::ExpandChildren( const wxDataViewItem& aItem )
{
    QModelIndex index = IndexCompat( aItem );

    if( index.isValid() )
        expandRecursively( index );
    else if( !aItem.IsOk() )
        expandAll();
}


void wxDataViewCtrl::ExpandAncestors( const wxDataViewItem& aItem )
{
    if( !m_model || !aItem.IsOk() || m_model->IsListModel() )
        return;

    std::vector<wxDataViewItem> ancestors;

    for( wxDataViewItem parent = m_model->GetParent( aItem ); parent.IsOk();
         parent = m_model->GetParent( parent ) )
    {
        ancestors.push_back( parent );
    }

    for( auto it = ancestors.rbegin(); it != ancestors.rend(); ++it )
        expand( IndexCompat( *it ) );
}


void wxDataViewCtrl::Collapse( const wxDataViewItem& aItem )
{
    collapse( IndexCompat( aItem ) );
}


bool wxDataViewCtrl::IsExpanded( const wxDataViewItem& aItem ) const
{
    QModelIndex index = IndexCompat( aItem );
    return index.isValid() && isExpanded( index );
}


void wxDataViewCtrl::EnsureVisible( const wxDataViewItem& aItem, const wxDataViewColumn* aColumn )
{
    ExpandAncestors( aItem );
    QModelIndex index = IndexCompat( aItem, aColumn ? std::max( GetColumnPosition( aColumn ), 0 ) : 0 );

    if( index.isValid() )
        scrollTo( index );
}


void wxDataViewCtrl::HitTest( const wxPoint& aPoint, wxDataViewItem& aItem,
                              wxDataViewColumn*& aColumn ) const
{
    QModelIndex index = indexAt( viewport()->mapFrom( this, QPoint( aPoint.x, aPoint.y ) ) );

    aItem = ItemCompat( index );
    aColumn = index.isValid() ? GetColumn( unsigned( index.column() ) ) : nullptr;
}


wxRect wxDataViewCtrl::GetItemRect( const wxDataViewItem& aItem,
                                    const wxDataViewColumn* aColumn ) const
{
    QModelIndex index = IndexCompat( aItem, aColumn ? std::max( GetColumnPosition( aColumn ), 0 ) : 0 );

    if( !index.isValid() )
        return wxRect();

    QRect rect = visualRect( index );

    if( !aColumn )
    {
        for( int col = 1; col < int( m_columns.size() ); ++col )
            rect = rect.united( visualRect( index.siblingAtColumn( col ) ) );
    }

    return wxRect( rect.x(), rect.y(), rect.width(), rect.height() );
}


int wxDataViewCtrl::GetCountPerPage() const
{
    int rowHeight = sizeHintForRow( 0 );
    return rowHeight > 0 ? viewport()->height() / rowHeight : 0;
}


wxDataViewItem wxDataViewCtrl::GetTopItem() const
{
    return ItemCompat( indexAt( QPoint( 1, 1 ) ) );
}


void wxDataViewCtrl::EditItem( const wxDataViewItem& aItem, const wxDataViewColumn* aColumn )
{
    QModelIndex index = IndexCompat( aItem, aColumn ? std::max( GetColumnPosition( aColumn ), 0 ) : 0 );

    if( index.isValid() )
        edit( index );
}


// The list store and the list control ------------------------------------------------------

void wxDataViewListStore::InsertColumn( unsigned int aPos, const wxString& aVariantType )
{
    aPos = std::min<unsigned>( aPos, unsigned( m_columnTypes.size() ) );
    m_columnTypes.insert( m_columnTypes.begin() + aPos, aVariantType );
}


wxString wxDataViewListStore::GetColumnType( unsigned int aCol ) const
{
    return aCol < m_columnTypes.size() ? m_columnTypes[aCol] : wxString( wxS( "string" ) );
}


void wxDataViewListStore::AppendItem( const std::vector<wxVariant>& aValues, wxUIntPtr aData )
{
    InsertItem( unsigned( m_rows.size() ), aValues, aData );
}


void wxDataViewListStore::PrependItem( const std::vector<wxVariant>& aValues, wxUIntPtr aData )
{
    InsertItem( 0, aValues, aData );
}


void wxDataViewListStore::InsertItem( unsigned int aRow, const std::vector<wxVariant>& aValues,
                                      wxUIntPtr aData )
{
    aRow = std::min<unsigned>( aRow, unsigned( m_rows.size() ) );

    ROW row;
    row.values = aValues;
    row.data = aData;
    m_rows.insert( m_rows.begin() + aRow, row );

    // The model of the rows has one more (its notification)
    m_count = unsigned( m_rows.size() ) - 1;
    RowInserted( aRow );
}


void wxDataViewListStore::DeleteItem( unsigned int aRow )
{
    if( aRow >= m_rows.size() )
        return;

    m_rows.erase( m_rows.begin() + aRow );
    m_count = unsigned( m_rows.size() ) + 1;
    RowDeleted( aRow );
}


void wxDataViewListStore::DeleteAllItems()
{
    m_rows.clear();
    Reset( 0 );
}


void wxDataViewListStore::SetItemData( const wxDataViewItem& aItem, wxUIntPtr aData )
{
    unsigned row = GetRow( aItem );

    if( row < m_rows.size() )
        m_rows[row].data = aData;
}


wxUIntPtr wxDataViewListStore::GetItemData( const wxDataViewItem& aItem ) const
{
    unsigned row = GetRow( aItem );
    return row < m_rows.size() ? m_rows[row].data : 0;
}


void wxDataViewListStore::GetValueByRow( wxVariant& aValue, unsigned int aRow,
                                         unsigned int aCol ) const
{
    if( aRow < m_rows.size() && aCol < m_rows[aRow].values.size() )
        aValue = m_rows[aRow].values[aCol];
    else
        aValue = wxVariant();
}


bool wxDataViewListStore::SetValueByRow( const wxVariant& aValue, unsigned int aRow,
                                         unsigned int aCol )
{
    if( aRow >= m_rows.size() )
        return false;

    std::vector<wxVariant>& values = m_rows[aRow].values;

    if( aCol >= values.size() )
        values.resize( aCol + 1 );

    values[aCol] = aValue;
    return true;
}


wxDataViewListCtrl::wxDataViewListCtrl( wxWidgetArgCompat aParent, int aId, const wxPoint& aPos,
                                        const wxSize& aSize, long aStyle,
                                        const wxValidator& aValidator ) :
        wxDataViewCtrl( aParent, aId, aPos, aSize, aStyle, aValidator ),
        m_store( new wxDataViewListStore() )
{
    AssociateModel( m_store );
    m_store->DecRef();
}


wxDataViewListCtrl::~wxDataViewListCtrl() = default;


int wxDataViewListCtrl::ItemToRow( const wxDataViewItem& aItem ) const
{
    return aItem.IsOk() ? int( m_store->GetRow( aItem ) ) : wxNOT_FOUND;
}


wxDataViewItem wxDataViewListCtrl::RowToItem( int aRow ) const
{
    return aRow >= 0 && unsigned( aRow ) < m_store->GetCount() ? m_store->GetItem( unsigned( aRow ) )
                                                                : wxDataViewItem();
}


int wxDataViewListCtrl::GetSelectedRow() const
{
    wxDataViewItem item = GetSelection();
    return item.IsOk() ? ItemToRow( item ) : wxNOT_FOUND;
}


void wxDataViewListCtrl::SelectRow( unsigned aRow )
{
    Select( RowToItem( int( aRow ) ) );
}


void wxDataViewListCtrl::UnselectRow( unsigned aRow )
{
    Unselect( RowToItem( int( aRow ) ) );
}


bool wxDataViewListCtrl::IsRowSelected( unsigned aRow ) const
{
    return IsSelected( RowToItem( int( aRow ) ) );
}


bool wxDataViewListCtrl::AppendColumn( wxDataViewColumn* aColumn )
{
    return AppendColumn( aColumn, aColumn->GetRenderer()->GetVariantType() );
}


bool wxDataViewListCtrl::AppendColumn( wxDataViewColumn* aColumn, const wxString& aVariantType )
{
    m_store->AppendColumn( aVariantType );
    return wxDataViewCtrl::AppendColumn( aColumn );
}


bool wxDataViewListCtrl::PrependColumn( wxDataViewColumn* aColumn )
{
    m_store->PrependColumn( aColumn->GetRenderer()->GetVariantType() );
    return wxDataViewCtrl::PrependColumn( aColumn );
}


bool wxDataViewListCtrl::InsertColumn( unsigned int aPos, wxDataViewColumn* aColumn )
{
    m_store->InsertColumn( aPos, aColumn->GetRenderer()->GetVariantType() );
    return wxDataViewCtrl::InsertColumn( aPos, aColumn );
}


bool wxDataViewListCtrl::ClearColumns()
{
    m_store->ClearColumns();
    return wxDataViewCtrl::ClearColumns();
}


wxDataViewColumn* wxDataViewListCtrl::AppendTextColumn( const wxString& aLabel,
                                                        wxDataViewCellMode aMode, int aWidth,
                                                        wxAlignment aAlign, int aFlags )
{
    wxDataViewColumn* column = new wxDataViewColumn(
            aLabel, new wxDataViewTextRenderer( wxS( "string" ), aMode ), GetColumnCount(), aWidth,
            aAlign, aFlags );
    AppendColumn( column, wxS( "string" ) );
    return column;
}


wxDataViewColumn* wxDataViewListCtrl::AppendToggleColumn( const wxString& aLabel,
                                                          wxDataViewCellMode aMode, int aWidth,
                                                          wxAlignment aAlign, int aFlags )
{
    wxDataViewColumn* column = new wxDataViewColumn(
            aLabel, new wxDataViewToggleRenderer( wxS( "bool" ), aMode ), GetColumnCount(), aWidth,
            aAlign, aFlags );
    AppendColumn( column, wxS( "bool" ) );
    return column;
}


wxDataViewColumn* wxDataViewListCtrl::AppendIconTextColumn( const wxString& aLabel,
                                                            wxDataViewCellMode aMode, int aWidth,
                                                            wxAlignment aAlign, int aFlags )
{
    wxDataViewColumn* column = new wxDataViewColumn(
            aLabel, new wxDataViewIconTextRenderer( wxS( "wxDataViewIconText" ), aMode ),
            GetColumnCount(), aWidth, aAlign, aFlags );
    AppendColumn( column, wxS( "wxDataViewIconText" ) );
    return column;
}


void wxDataViewListCtrl::AppendItem( const std::vector<wxVariant>& aValues, wxUIntPtr aData )
{
    m_store->AppendItem( aValues, aData );
}


void wxDataViewListCtrl::PrependItem( const std::vector<wxVariant>& aValues, wxUIntPtr aData )
{
    m_store->PrependItem( aValues, aData );
}


void wxDataViewListCtrl::InsertItem( unsigned int aRow, const std::vector<wxVariant>& aValues,
                                     wxUIntPtr aData )
{
    m_store->InsertItem( aRow, aValues, aData );
}


void wxDataViewListCtrl::DeleteItem( unsigned aRow )
{
    m_store->DeleteItem( aRow );
}


void wxDataViewListCtrl::DeleteAllItems()
{
    m_store->DeleteAllItems();
}


unsigned int wxDataViewListCtrl::GetItemCount() const
{
    return m_store->GetItemCount();
}


wxUIntPtr wxDataViewListCtrl::GetItemData( const wxDataViewItem& aItem ) const
{
    return m_store->GetItemData( aItem );
}


void wxDataViewListCtrl::SetItemData( const wxDataViewItem& aItem, wxUIntPtr aData )
{
    m_store->SetItemData( aItem, aData );
}


void wxDataViewListCtrl::SetValue( const wxVariant& aValue, unsigned int aRow, unsigned int aCol )
{
    m_store->SetValueByRow( aValue, aRow, aCol );
    m_store->RowValueChanged( aRow, aCol );
}


void wxDataViewListCtrl::GetValue( wxVariant& aValue, unsigned int aRow, unsigned int aCol )
{
    m_store->GetValueByRow( aValue, aRow, aCol );
}


void wxDataViewListCtrl::SetTextValue( const wxString& aValue, unsigned int aRow, unsigned int aCol )
{
    SetValue( wxVariant( aValue ), aRow, aCol );
}


wxString wxDataViewListCtrl::GetTextValue( unsigned int aRow, unsigned int aCol ) const
{
    wxVariant value;
    m_store->GetValueByRow( value, aRow, aCol );
    return value.IsNull() ? wxString() : value.GetString();
}


void wxDataViewListCtrl::SetToggleValue( bool aValue, unsigned int aRow, unsigned int aCol )
{
    SetValue( wxVariant( aValue ), aRow, aCol );
}


bool wxDataViewListCtrl::GetToggleValue( unsigned int aRow, unsigned int aCol ) const
{
    wxVariant value;
    m_store->GetValueByRow( value, aRow, aCol );
    return !value.IsNull() && value.GetBool();
}
