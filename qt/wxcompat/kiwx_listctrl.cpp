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

#include <wx/kiwx_listctrl.h>

#include <algorithm>

#include <QAbstractTableModel>
#include <QHeaderView>
#include <QKeyEvent>
#include <QScrollBar>

#include <qt_strings.h>


// The image list ---------------------------------------------------------------------------

int wxImageList::Add( const wxBitmap& aBitmap )
{
    if( m_size.x <= 0 )
        m_size = aBitmap.GetLogicalSize();

    m_images.push_back( aBitmap );
    return int( m_images.size() ) - 1;
}


bool wxImageList::Replace( int aIndex, const wxBitmap& aBitmap )
{
    if( aIndex < 0 || aIndex >= GetImageCount() )
        return false;

    m_images[aIndex] = aBitmap;
    return true;
}


bool wxImageList::Remove( int aIndex )
{
    if( aIndex < 0 || aIndex >= GetImageCount() )
        return false;

    m_images.erase( m_images.begin() + aIndex );
    return true;
}


wxBitmap wxImageList::GetBitmap( int aIndex ) const
{
    return aIndex >= 0 && aIndex < GetImageCount() ? m_images[aIndex] : wxBitmap();
}


bool wxImageList::GetSize( int aIndex, int& aWidth, int& aHeight ) const
{
    aWidth = m_size.x;
    aHeight = m_size.y;
    return aIndex >= 0 && aIndex < GetImageCount();
}


// The model of a list ----------------------------------------------------------------------

/// The rows and columns of a list (or the texts of a virtual list).
class KIWX_LIST_MODEL : public QAbstractTableModel
{
public:
    struct CELL
    {
        QString text;
        int     image = -1;
    };

    struct ROW
    {
        std::vector<CELL> cells;
        wxUIntPtr         data = 0;
        bool              checked = false;
        wxFont            font;
        wxColour          foreground;
        wxColour          background;
    };

    struct COLUMN
    {
        QString            heading;
        wxListColumnFormat format = wxLIST_FORMAT_LEFT;
    };

    explicit KIWX_LIST_MODEL( wxListCtrl* aList ) : QAbstractTableModel( aList ), m_list( aList ) {}

    int rowCount( const QModelIndex& aParent = QModelIndex() ) const override
    {
        if( aParent.isValid() )
            return 0;

        return m_list->IsVirtual() ? int( m_virtualCount ) : int( m_rows.size() );
    }

    int columnCount( const QModelIndex& aParent = QModelIndex() ) const override
    {
        return aParent.isValid() ? 0 : std::max<int>( int( m_columns.size() ), 1 );
    }

    QVariant data( const QModelIndex& aIndex, int aRole ) const override
    {
        int row = aIndex.row();
        int col = aIndex.column();

        if( !aIndex.isValid() || row >= rowCount() )
            return QVariant();

        const ROW* entry = m_list->IsVirtual() ? nullptr : &m_rows[row];

        switch( aRole )
        {
        case Qt::DisplayRole:
            if( !entry )
                return ToQString( m_list->OnGetItemText( row, col ) );

            return col < int( entry->cells.size() ) ? entry->cells[col].text : QString();

        case Qt::DecorationRole:
        {
            int image = entry ? ( col < int( entry->cells.size() ) ? entry->cells[col].image : -1 )
                              : m_list->OnGetItemColumnImage( row, col );

            if( image < 0 )
                return QVariant();

            wxBitmap bitmap = m_list->ImageCompat( image );
            return bitmap.IsOk() ? QVariant( QIcon( bitmap ) ) : QVariant();
        }

        case Qt::CheckStateRole:
            if( col == 0 && m_list->HasCheckBoxes() && entry )
                return entry->checked ? Qt::Checked : Qt::Unchecked;

            return QVariant();

        case Qt::FontRole:
            if( entry && entry->font.IsOk() )
                return QFont( entry->font );

            return QVariant();

        case Qt::ForegroundRole:
            if( entry && entry->foreground.IsOk() )
                return QColor( entry->foreground );

            return QVariant();

        case Qt::BackgroundRole:
            if( entry && entry->background.IsOk() )
                return QColor( entry->background );

            return QVariant();

        case Qt::TextAlignmentRole:
            return int( alignment( col ) );

        default:
            return QVariant();
        }
    }

    bool setData( const QModelIndex& aIndex, const QVariant& aValue, int aRole ) override
    {
        if( aRole != Qt::CheckStateRole || m_list->IsVirtual() || !aIndex.isValid() )
            return false;

        ROW& row = m_rows[aIndex.row()];
        row.checked = aValue.toInt() == Qt::Checked;
        emit dataChanged( aIndex, aIndex );

        m_list->sendEvent( row.checked ? wxEVT_LIST_ITEM_CHECKED : wxEVT_LIST_ITEM_UNCHECKED,
                           aIndex.row() );
        return true;
    }

    Qt::ItemFlags flags( const QModelIndex& aIndex ) const override
    {
        Qt::ItemFlags result = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

        if( aIndex.column() == 0 && m_list->HasCheckBoxes() )
            result |= Qt::ItemIsUserCheckable;

        return result;
    }

    QVariant headerData( int aSection, Qt::Orientation aOrientation, int aRole ) const override
    {
        if( aOrientation != Qt::Horizontal || aSection >= int( m_columns.size() ) )
            return QVariant();

        if( aRole == Qt::DisplayRole )
            return m_columns[aSection].heading;

        if( aRole == Qt::TextAlignmentRole )
            return int( alignment( aSection ) );

        return QVariant();
    }

    Qt::Alignment alignment( int aCol ) const
    {
        wxListColumnFormat format = aCol < int( m_columns.size() ) ? m_columns[aCol].format
                                                                   : wxLIST_FORMAT_LEFT;

        switch( format )
        {
        case wxLIST_FORMAT_RIGHT:  return Qt::AlignRight | Qt::AlignVCenter;
        case wxLIST_FORMAT_CENTRE: return Qt::AlignHCenter | Qt::AlignVCenter;
        default:                   return Qt::AlignLeft | Qt::AlignVCenter;
        }
    }

    void cellChanged( long aRow, int aCol )
    {
        QModelIndex index = this->index( int( aRow ), std::max( aCol, 0 ) );
        emit dataChanged( index, aCol < 0 ? this->index( int( aRow ), columnCount() - 1 ) : index );
    }

    void reset()
    {
        beginResetModel();
        endResetModel();
    }

    using QAbstractTableModel::beginInsertRows;
    using QAbstractTableModel::endInsertRows;
    using QAbstractTableModel::beginRemoveRows;
    using QAbstractTableModel::endRemoveRows;
    using QAbstractTableModel::beginInsertColumns;
    using QAbstractTableModel::endInsertColumns;
    using QAbstractTableModel::beginRemoveColumns;
    using QAbstractTableModel::endRemoveColumns;
    using QAbstractTableModel::beginResetModel;
    using QAbstractTableModel::endResetModel;

    wxListCtrl*         m_list;
    std::vector<ROW>    m_rows;
    std::vector<COLUMN> m_columns;
    long                m_virtualCount = 0;
};


// wxListCtrl -------------------------------------------------------------------------------

wxListCtrl::wxListCtrl( wxWidgetArgCompat aParent, int aId, const wxPoint& aPos,
                        const wxSize& aSize, long aStyle, const wxValidator& aValidator,
                        const wxString& aName ) :
        wxWindowCompat<QTreeView>( aParent.get() ),
        m_model( new KIWX_LIST_MODEL( this ) ),
        m_checkBoxes( false ),
        m_imageList( nullptr ),
        m_ownImageList( false )
{
    SetId( aId );

    if( !aName.IsEmpty() )
        SetName( aName );

    setModel( m_model );
    setRootIsDecorated( false );
    setItemsExpandable( false );
    setUniformRowHeights( true );
    setAllColumnsShowFocus( true );
    setSelectionBehavior( QAbstractItemView::SelectRows );
    setEditTriggers( QAbstractItemView::NoEditTriggers );
    header()->setStretchLastSection( false );
    header()->setSectionsMovable( false );

    if( aSize.x > 0 || aSize.y > 0 )
        SetMinSize( aSize );

    wxWindow::SetWindowStyleFlag( aStyle );
    applyStyle();

    // The events of wxWidgets
    QObject::connect( selectionModel(), &QItemSelectionModel::selectionChanged, this,
                      [this]( const QItemSelection& aSelected, const QItemSelection& aDeselected )
                      {
                          for( const QModelIndex& index : aDeselected.indexes() )
                          {
                              if( index.column() == 0 )
                                  sendEvent( wxEVT_LIST_ITEM_DESELECTED, index.row() );
                          }

                          for( const QModelIndex& index : aSelected.indexes() )
                          {
                              if( index.column() == 0 )
                                  sendEvent( wxEVT_LIST_ITEM_SELECTED, index.row() );
                          }
                      } );

    QObject::connect( selectionModel(), &QItemSelectionModel::currentRowChanged, this,
                      [this]( const QModelIndex& aCurrent, const QModelIndex& )
                      {
                          if( aCurrent.isValid() )
                              sendEvent( wxEVT_LIST_ITEM_FOCUSED, aCurrent.row() );
                      } );

    QObject::connect( this, &QAbstractItemView::activated, this,
                      [this]( const QModelIndex& aIndex )
                      {
                          sendEvent( wxEVT_LIST_ITEM_ACTIVATED, aIndex.row(), aIndex.column() );
                      } );

    QObject::connect( header(), &QHeaderView::sectionClicked, this,
                      [this]( int aCol ) { sendEvent( wxEVT_LIST_COL_CLICK, -1, aCol ); } );

    setContextMenuPolicy( Qt::CustomContextMenu );
    QObject::connect( this, &QWidget::customContextMenuRequested, this,
                      [this]( const QPoint& aPos )
                      {
                          QModelIndex index = indexAt( aPos );
                          wxListEvent event( wxEVT_LIST_ITEM_RIGHT_CLICK, GetId() );
                          wxListItem  item;

                          item.SetId( index.isValid() ? index.row() : -1 );

                          if( index.isValid() )
                          {
                              item.SetText( GetItemText( index.row() ) );
                              item.SetData( reinterpret_cast<void*>( GetItemData( index.row() ) ) );
                          }

                          event.SetEventObject( this );
                          event.SetItemCompat( item );
                          event.SetPointCompat( wxPoint( aPos.x(), aPos.y() ) );
                          ProcessEvent( event );
                      } );
}


wxListCtrl::~wxListCtrl()
{
    if( m_ownImageList )
        delete m_imageList;
}


void wxListCtrl::applyStyle()
{
    long style = GetWindowStyleFlag();

    setHeaderHidden( ( style & wxLC_NO_HEADER ) || !( style & wxLC_REPORT ) );
    setSelectionMode( ( style & wxLC_SINGLE_SEL ) ? QAbstractItemView::SingleSelection
                                                  : QAbstractItemView::ExtendedSelection );
}


void wxListCtrl::SetWindowStyleFlag( long aStyle )
{
    wxWindow::SetWindowStyleFlag( aStyle );
    applyStyle();
}


void wxListCtrl::SetSingleStyle( long aStyle, bool aAdd )
{
    long style = GetWindowStyleFlag();
    SetWindowStyleFlag( aAdd ? ( style | aStyle ) : ( style & ~aStyle ) );
}


void wxListCtrl::sendEvent( wxEventType aType, long aItem, int aColumn )
{
    wxListEvent event( aType, GetId() );
    wxListItem  item;

    item.SetId( aItem );
    item.SetColumn( std::max( aColumn, 0 ) );

    if( aItem >= 0 && aItem < GetItemCount() )
    {
        item.SetText( GetItemText( aItem, std::max( aColumn, 0 ) ) );
        item.SetData( reinterpret_cast<void*>( GetItemData( aItem ) ) );
    }

    event.SetEventObject( this );
    event.SetItemCompat( item );
    event.SetColumnCompat( aColumn );
    ProcessEvent( event );
}


void wxListCtrl::keyPressEvent( QKeyEvent* aEvent )
{
    wxKeyEvent key( wxEVT_KEY_DOWN );
    wxFillKeyEventCompat( key, aEvent );

    wxListEvent event( wxEVT_LIST_KEY_DOWN, GetId() );
    wxListItem  item;

    item.SetId( currentIndex().isValid() ? currentIndex().row() : -1 );
    event.SetEventObject( this );
    event.SetItemCompat( item );
    event.SetKeyCodeCompat( key.GetKeyCode() );
    ProcessEvent( event );

    wxWindowCompat<QTreeView>::keyPressEvent( aEvent );
}


// The columns ------------------------------------------------------------------------------

long wxListCtrl::InsertColumn( long aCol, const wxString& aHeading, int aFormat, int aWidth )
{
    long col = std::clamp<long>( aCol, 0, long( m_model->m_columns.size() ) );

    // The first column replaces the column of a list without columns
    bool first = m_model->m_columns.empty();

    if( !first )
        m_model->beginInsertColumns( QModelIndex(), int( col ), int( col ) );

    KIWX_LIST_MODEL::COLUMN column;
    column.heading = ToQString( aHeading );
    column.format = wxListColumnFormat( aFormat );
    m_model->m_columns.insert( m_model->m_columns.begin() + col, column );

    for( KIWX_LIST_MODEL::ROW& row : m_model->m_rows )
    {
        if( long( row.cells.size() ) >= col )
            row.cells.insert( row.cells.begin() + col, KIWX_LIST_MODEL::CELL() );
    }

    if( first )
        m_model->reset();
    else
        m_model->endInsertColumns();

    SetColumnWidth( int( col ), aWidth );
    return col;
}


long wxListCtrl::InsertColumn( long aCol, const wxListItem& aInfo )
{
    long col = InsertColumn( aCol, aInfo.GetText(), aInfo.GetAlign(),
                             ( aInfo.GetMask() & wxLIST_MASK_WIDTH ) ? aInfo.GetWidth()
                                                                     : wxLIST_AUTOSIZE );
    return col;
}


long wxListCtrl::AppendColumn( const wxString& aHeading, wxListColumnFormat aFormat, int aWidth )
{
    return InsertColumn( GetColumnCount(), aHeading, aFormat, aWidth );
}


bool wxListCtrl::DeleteColumn( int aCol )
{
    if( aCol < 0 || aCol >= GetColumnCount() )
        return false;

    m_model->beginResetModel();
    m_model->m_columns.erase( m_model->m_columns.begin() + aCol );

    for( KIWX_LIST_MODEL::ROW& row : m_model->m_rows )
    {
        if( aCol < int( row.cells.size() ) )
            row.cells.erase( row.cells.begin() + aCol );
    }

    m_model->endResetModel();
    return true;
}


bool wxListCtrl::DeleteAllColumns()
{
    m_model->beginResetModel();
    m_model->m_columns.clear();

    for( KIWX_LIST_MODEL::ROW& row : m_model->m_rows )
        row.cells.clear();

    m_model->endResetModel();
    return true;
}


int wxListCtrl::GetColumnCount() const
{
    return int( m_model->m_columns.size() );
}


bool wxListCtrl::GetColumn( int aCol, wxListItem& aItem ) const
{
    if( aCol < 0 || aCol >= GetColumnCount() )
        return false;

    aItem.SetColumn( aCol );
    aItem.SetText( ToWxString( m_model->m_columns[aCol].heading ) );
    aItem.SetAlign( m_model->m_columns[aCol].format );
    aItem.SetWidth( GetColumnWidth( aCol ) );
    return true;
}


bool wxListCtrl::SetColumn( int aCol, const wxListItem& aItem )
{
    if( aCol < 0 || aCol >= GetColumnCount() )
        return false;

    if( aItem.GetMask() & wxLIST_MASK_TEXT )
        m_model->m_columns[aCol].heading = ToQString( aItem.GetText() );

    if( aItem.GetMask() & wxLIST_MASK_FORMAT )
        m_model->m_columns[aCol].format = aItem.GetAlign();

    emit m_model->headerDataChanged( Qt::Horizontal, aCol, aCol );

    if( aItem.GetMask() & wxLIST_MASK_WIDTH )
        SetColumnWidth( aCol, aItem.GetWidth() );

    return true;
}


bool wxListCtrl::SetColumnWidth( int aCol, int aWidth )
{
    if( aCol < 0 || aCol >= std::max( GetColumnCount(), 1 ) )
        return false;

    if( aWidth == wxLIST_AUTOSIZE )
    {
        resizeColumnToContents( aCol );
    }
    else if( aWidth == wxLIST_AUTOSIZE_USEHEADER )
    {
        resizeColumnToContents( aCol );
        setColumnWidth( aCol, std::max( columnWidth( aCol ), header()->sectionSizeHint( aCol ) ) );
    }
    else
    {
        setColumnWidth( aCol, aWidth );
    }

    return true;
}


int wxListCtrl::GetColumnWidth( int aCol ) const
{
    return columnWidth( aCol );
}


// The items --------------------------------------------------------------------------------

long wxListCtrl::InsertItem( long aIndex, const wxString& aLabel, int aImageIndex )
{
    long row = std::clamp<long>( aIndex, 0, long( m_model->m_rows.size() ) );

    m_model->beginInsertRows( QModelIndex(), int( row ), int( row ) );

    KIWX_LIST_MODEL::ROW entry;
    entry.cells.resize( std::max<size_t>( m_model->m_columns.size(), 1 ) );
    entry.cells[0].text = ToQString( aLabel );
    entry.cells[0].image = aImageIndex;
    m_model->m_rows.insert( m_model->m_rows.begin() + row, entry );

    m_model->endInsertRows();
    return row;
}


long wxListCtrl::InsertItem( const wxListItem& aInfo )
{
    long row = InsertItem( aInfo.GetId(), aInfo.GetText(), aInfo.GetImage() );

    if( aInfo.GetMask() & wxLIST_MASK_DATA )
        SetItemPtrData( row, aInfo.GetData() );

    return row;
}


bool wxListCtrl::SetItem( const wxListItem& aInfo )
{
    long row = aInfo.GetId();
    int  col = aInfo.GetColumn();

    if( row < 0 || row >= long( m_model->m_rows.size() ) || col < 0 )
        return false;

    KIWX_LIST_MODEL::ROW& entry = m_model->m_rows[row];

    if( col >= int( entry.cells.size() ) )
        entry.cells.resize( col + 1 );

    if( aInfo.GetMask() & wxLIST_MASK_TEXT )
        entry.cells[col].text = ToQString( aInfo.GetText() );

    if( aInfo.GetMask() & wxLIST_MASK_IMAGE )
        entry.cells[col].image = aInfo.GetImage();

    if( aInfo.GetMask() & wxLIST_MASK_DATA )
        entry.data = aInfo.GetData();

    if( aInfo.GetMask() & wxLIST_MASK_STATE )
        SetItemState( row, aInfo.GetState(), aInfo.GetState() );

    if( aInfo.GetFont().IsOk() )
        entry.font = aInfo.GetFont();

    if( aInfo.GetTextColour().IsOk() )
        entry.foreground = aInfo.GetTextColour();

    if( aInfo.GetBackgroundColour().IsOk() )
        entry.background = aInfo.GetBackgroundColour();

    m_model->cellChanged( row, -1 );
    return true;
}


long wxListCtrl::SetItem( long aIndex, int aCol, const wxString& aLabel, int aImageId )
{
    wxListItem info;
    info.SetId( aIndex );
    info.SetColumn( aCol );
    info.SetText( aLabel );

    if( aImageId != -1 )
        info.SetImage( aImageId );

    return SetItem( info ) ? aIndex : -1;
}


bool wxListCtrl::GetItem( wxListItem& aInfo ) const
{
    long row = aInfo.GetId();
    int  col = aInfo.GetColumn();

    if( row < 0 || row >= GetItemCount() )
        return false;

    aInfo.SetText( GetItemText( row, col ) );
    aInfo.SetData( reinterpret_cast<void*>( GetItemData( row ) ) );
    aInfo.SetState( GetItemState( row, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED ) );

    if( !IsVirtual() && col < int( m_model->m_rows[row].cells.size() ) )
        aInfo.SetImage( m_model->m_rows[row].cells[col].image );

    return true;
}


wxString wxListCtrl::GetItemText( long aItem, int aCol ) const
{
    if( aItem < 0 || aItem >= GetItemCount() )
        return wxString();

    if( IsVirtual() )
        return OnGetItemText( aItem, aCol );

    const KIWX_LIST_MODEL::ROW& row = m_model->m_rows[aItem];
    return aCol >= 0 && aCol < int( row.cells.size() ) ? ToWxString( row.cells[aCol].text )
                                                       : wxString();
}


bool wxListCtrl::SetItemData( long aItem, long aData )
{
    return SetItemPtrData( aItem, wxUIntPtr( aData ) );
}


bool wxListCtrl::SetItemPtrData( long aItem, wxUIntPtr aData )
{
    if( IsVirtual() || aItem < 0 || aItem >= GetItemCount() )
        return false;

    m_model->m_rows[aItem].data = aData;
    return true;
}


wxUIntPtr wxListCtrl::GetItemData( long aItem ) const
{
    if( IsVirtual() || aItem < 0 || aItem >= GetItemCount() )
        return 0;

    return m_model->m_rows[aItem].data;
}


bool wxListCtrl::SetItemImage( long aItem, int aImage, int aSelImage )
{
    return SetItemColumnImage( aItem, 0, aImage );
}


bool wxListCtrl::SetItemColumnImage( long aItem, long aColumn, int aImage )
{
    if( IsVirtual() || aItem < 0 || aItem >= GetItemCount() || aColumn < 0 )
        return false;

    KIWX_LIST_MODEL::ROW& row = m_model->m_rows[aItem];

    if( aColumn >= long( row.cells.size() ) )
        row.cells.resize( aColumn + 1 );

    row.cells[aColumn].image = aImage;
    m_model->cellChanged( aItem, int( aColumn ) );
    return true;
}


void wxListCtrl::SetItemFont( long aItem, const wxFont& aFont )
{
    if( IsVirtual() || aItem < 0 || aItem >= GetItemCount() )
        return;

    m_model->m_rows[aItem].font = aFont;
    m_model->cellChanged( aItem, -1 );
}


wxFont wxListCtrl::GetItemFont( long aItem ) const
{
    if( IsVirtual() || aItem < 0 || aItem >= GetItemCount() || !m_model->m_rows[aItem].font.IsOk() )
        return GetFont();

    return m_model->m_rows[aItem].font;
}


void wxListCtrl::SetItemTextColour( long aItem, const wxColour& aColour )
{
    if( IsVirtual() || aItem < 0 || aItem >= GetItemCount() )
        return;

    m_model->m_rows[aItem].foreground = aColour;
    m_model->cellChanged( aItem, -1 );
}


wxColour wxListCtrl::GetItemTextColour( long aItem ) const
{
    if( IsVirtual() || aItem < 0 || aItem >= GetItemCount() )
        return wxColour();

    return m_model->m_rows[aItem].foreground;
}


void wxListCtrl::SetItemBackgroundColour( long aItem, const wxColour& aColour )
{
    if( IsVirtual() || aItem < 0 || aItem >= GetItemCount() )
        return;

    m_model->m_rows[aItem].background = aColour;
    m_model->cellChanged( aItem, -1 );
}


bool wxListCtrl::DeleteItem( long aItem )
{
    if( IsVirtual() || aItem < 0 || aItem >= GetItemCount() )
        return false;

    m_model->beginRemoveRows( QModelIndex(), int( aItem ), int( aItem ) );
    m_model->m_rows.erase( m_model->m_rows.begin() + aItem );
    m_model->endRemoveRows();
    return true;
}


bool wxListCtrl::DeleteAllItems()
{
    m_model->beginResetModel();
    m_model->m_rows.clear();
    m_model->m_virtualCount = 0;
    m_model->endResetModel();
    return true;
}


void wxListCtrl::ClearAll()
{
    m_model->beginResetModel();
    m_model->m_rows.clear();
    m_model->m_columns.clear();
    m_model->m_virtualCount = 0;
    m_model->endResetModel();
}


int wxListCtrl::GetItemCount() const
{
    return m_model->rowCount();
}


void wxListCtrl::SetItemCount( long aCount )
{
    m_model->beginResetModel();
    m_model->m_virtualCount = std::max<long>( aCount, 0 );
    m_model->endResetModel();
}


void wxListCtrl::RefreshItem( long aItem )
{
    if( aItem >= 0 && aItem < GetItemCount() )
        m_model->cellChanged( aItem, -1 );
}


void wxListCtrl::RefreshItems( long aFrom, long aTo )
{
    if( GetItemCount() == 0 || aTo < aFrom )
        return;

    long from = std::clamp<long>( aFrom, 0, GetItemCount() - 1 );
    long to = std::clamp<long>( aTo, 0, GetItemCount() - 1 );

    emit m_model->dataChanged( m_model->index( int( from ), 0 ),
                               m_model->index( int( to ), m_model->columnCount() - 1 ) );
}


// The selection and the states -------------------------------------------------------------

long wxListCtrl::GetNextItem( long aItem, int aGeometry, int aState ) const
{
    int count = GetItemCount();

    for( long row = aItem + 1; row < count; ++row )
    {
        if( aState == wxLIST_STATE_DONTCARE || ( GetItemState( row, aState ) & aState ) )
            return row;
    }

    return -1;
}


int wxListCtrl::GetItemState( long aItem, long aStateMask ) const
{
    if( aItem < 0 || aItem >= GetItemCount() )
        return 0;

    int state = 0;

    if( ( aStateMask & wxLIST_STATE_SELECTED )
        && selectionModel()->isRowSelected( int( aItem ), QModelIndex() ) )
    {
        state |= wxLIST_STATE_SELECTED;
    }

    if( ( aStateMask & wxLIST_STATE_FOCUSED ) && currentIndex().isValid()
        && currentIndex().row() == aItem )
    {
        state |= wxLIST_STATE_FOCUSED;
    }

    return state;
}


bool wxListCtrl::SetItemState( long aItem, long aState, long aStateMask )
{
    // -1: all the items
    if( aItem == -1 )
    {
        for( long row = 0; row < GetItemCount(); ++row )
            SetItemState( row, aState, aStateMask );

        return true;
    }

    if( aItem < 0 || aItem >= GetItemCount() )
        return false;

    QModelIndex index = m_model->index( int( aItem ), 0 );

    if( aStateMask & wxLIST_STATE_SELECTED )
    {
        QItemSelectionModel::SelectionFlags flags = ( aState & wxLIST_STATE_SELECTED )
                                                            ? QItemSelectionModel::Select
                                                            : QItemSelectionModel::Deselect;

        if( ( aState & wxLIST_STATE_SELECTED ) && selectionMode() == QAbstractItemView::SingleSelection )
            flags = QItemSelectionModel::ClearAndSelect;

        selectionModel()->select( index, flags | QItemSelectionModel::Rows );
    }

    if( ( aStateMask & wxLIST_STATE_FOCUSED ) && ( aState & wxLIST_STATE_FOCUSED ) )
        selectionModel()->setCurrentIndex( index, QItemSelectionModel::NoUpdate );

    return true;
}


int wxListCtrl::GetSelectedItemCount() const
{
    return int( selectionModel()->selectedRows().size() );
}


bool wxListCtrl::EnsureVisible( long aItem )
{
    if( aItem < 0 || aItem >= GetItemCount() )
        return false;

    scrollTo( m_model->index( int( aItem ), 0 ) );
    return true;
}


long wxListCtrl::FindItem( long aStart, const wxString& aText, bool aPartial ) const
{
    for( long row = std::max<long>( aStart + 1, 0 ); row < GetItemCount(); ++row )
    {
        wxString text = GetItemText( row );

        if( aPartial ? text.Lower().StartsWith( aText.Lower() ) : text.IsSameAs( aText, false ) )
            return row;
    }

    return -1;
}


long wxListCtrl::FindItem( long aStart, wxUIntPtr aData ) const
{
    for( long row = std::max<long>( aStart + 1, 0 ); row < GetItemCount(); ++row )
    {
        if( GetItemData( row ) == aData )
            return row;
    }

    return -1;
}


long wxListCtrl::GetTopItem() const
{
    QModelIndex index = indexAt( QPoint( 1, 1 ) );
    return index.isValid() ? index.row() : 0;
}


int wxListCtrl::GetCountPerPage() const
{
    int rowHeight = sizeHintForRow( 0 );
    return rowHeight > 0 ? viewport()->height() / rowHeight : 0;
}


bool wxListCtrl::GetItemRect( long aItem, wxRect& aRect, int aCode ) const
{
    if( aItem < 0 || aItem >= GetItemCount() )
        return false;

    QRect rect = visualRect( m_model->index( int( aItem ), 0 ) );

    for( int col = 1; col < m_model->columnCount(); ++col )
        rect = rect.united( visualRect( m_model->index( int( aItem ), col ) ) );

    aRect = wxRect( rect.x(), rect.y(), rect.width(), rect.height() );
    return true;
}


long wxListCtrl::HitTest( const wxPoint& aPoint, int& aFlags, long* aSubItem ) const
{
    QModelIndex index = indexAt( QPoint( aPoint.x, aPoint.y ) );

    aFlags = index.isValid() ? 0x0080 /* wxLIST_HITTEST_ONITEMLABEL */ : 0x0004 /* NOWHERE */;

    if( aSubItem )
        *aSubItem = index.isValid() ? index.column() : -1;

    return index.isValid() ? index.row() : -1;
}


// The check boxes and the images -----------------------------------------------------------

bool wxListCtrl::EnableCheckBoxes( bool aEnable )
{
    m_checkBoxes = aEnable;
    m_model->reset();
    return true;
}


void wxListCtrl::CheckItem( long aItem, bool aCheck )
{
    if( IsVirtual() || aItem < 0 || aItem >= GetItemCount() )
        return;

    m_model->m_rows[aItem].checked = aCheck;
    m_model->cellChanged( aItem, 0 );
}


bool wxListCtrl::IsItemChecked( long aItem ) const
{
    return !IsVirtual() && aItem >= 0 && aItem < GetItemCount() && m_model->m_rows[aItem].checked;
}


void wxListCtrl::SetImageList( wxImageList* aImageList, int aWhich )
{
    if( m_ownImageList )
        delete m_imageList;

    m_imageList = aImageList;
    m_ownImageList = false;

    if( m_imageList && m_imageList->GetSize().y > 0 )
        setIconSize( QSize( m_imageList->GetSize().x, m_imageList->GetSize().y ) );

    m_model->reset();
}


void wxListCtrl::AssignImageList( wxImageList* aImageList, int aWhich )
{
    SetImageList( aImageList, aWhich );
    m_ownImageList = true;
}


wxImageList* wxListCtrl::GetImageList( int aWhich ) const
{
    return m_imageList;
}


void wxListCtrl::SetNormalImages( const std::vector<wxBitmapBundle>& aImages )
{
    m_images = aImages;
    m_model->reset();
}


void wxListCtrl::SetSmallImages( const std::vector<wxBitmapBundle>& aImages )
{
    m_images = aImages;

    if( !m_images.empty() )
    {
        wxSize size = m_images.front().GetDefaultSize();
        setIconSize( QSize( size.x, size.y ) );
    }

    m_model->reset();
}


wxBitmap wxListCtrl::ImageCompat( int aIndex ) const
{
    if( aIndex < 0 )
        return wxBitmap();

    if( m_imageList )
        return m_imageList->GetBitmap( aIndex );

    if( aIndex < int( m_images.size() ) )
    {
        QSize size = iconSize().isValid() ? iconSize() : QSize( 16, 16 );
        return wxBitmap( m_images[aIndex].pixmap( size ) );
    }

    return wxBitmap();
}


// wxListView -------------------------------------------------------------------------------

void wxListView::Select( long aItem, bool aSelect )
{
    SetItemState( aItem, aSelect ? wxLIST_STATE_SELECTED : 0, wxLIST_STATE_SELECTED );
}


void wxListView::Focus( long aItem )
{
    SetItemState( aItem, wxLIST_STATE_FOCUSED, wxLIST_STATE_FOCUSED );
    EnsureVisible( aItem );
}


long wxListView::GetFocusedItem() const
{
    return currentIndex().isValid() ? currentIndex().row() : -1;
}
