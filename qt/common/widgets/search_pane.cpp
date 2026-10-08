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
 * @file search_pane.cpp
 * The search pane with Qt: SEARCH_PANE, its tabs and their tables (see
 * common/widgets/search_pane.cpp and common/widgets/search_pane_tab.cpp).
 */

#include <widgets/search_pane.h>
#include <widgets/search_pane_tab.h>

#include <QAbstractTableModel>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QDockWidget>
#include <QGuiApplication>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMenu>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include <bitmaps.h>
#include <core/kicad_algo.h>
#include <core/ki_assert.h>
#include <eda_draw_frame.h>
#include <kiway.h>
#include <qt_strings.h>
#include <qt_ui.h>
#include <settings/app_settings.h>
#include <tool/action_menu.h>
#include <tool/tool_manager.h>
#include <ui_hooks.h>
#include <widgets/bitmap_button.h>


#define ID_TOGGLE_ZOOM_TO_SELECTION    14000
#define ID_TOGGLE_PAN_TO_SELECTION     14001
#define ID_TOGGLE_SEARCH_HIDDEN_FIELDS 14002
#define ID_TOGGLE_SEARCH_METADATA      14003


/**
 * The results of a search, as a table: the cells are given by the search handler.
 */
class SEARCH_RESULTS_MODEL : public QAbstractTableModel
{
public:
    SEARCH_RESULTS_MODEL( const std::shared_ptr<SEARCH_HANDLER>& aHandler, QObject* aParent ) :
            QAbstractTableModel( aParent ),
            m_handler( aHandler ),
            m_count( 0 )
    {
    }

    int rowCount( const QModelIndex& aParent = QModelIndex() ) const override
    {
        return aParent.isValid() ? 0 : m_count;
    }

    int columnCount( const QModelIndex& aParent = QModelIndex() ) const override
    {
        return aParent.isValid() ? 0 : (int) m_handler->GetColumns().size();
    }

    QVariant data( const QModelIndex& aIndex, int aRole ) const override
    {
        if( !aIndex.isValid() )
            return QVariant();

        if( aRole == Qt::DisplayRole )
            return ToQString( m_handler->GetResultCell( aIndex.row(), aIndex.column() ) );

        if( aRole == Qt::TextAlignmentRole )
            return QVariant::fromValue( alignment( aIndex.column() ) );

        return QVariant();
    }

    QVariant headerData( int aSection, Qt::Orientation aOrientation, int aRole ) const override
    {
        if( aOrientation != Qt::Horizontal )
            return QVariant();

        std::vector<std::tuple<wxString, int, SEARCH_COLUMN_ALIGN>> columns =
                m_handler->GetColumns();

        if( aSection < 0 || aSection >= (int) columns.size() )
            return QVariant();

        if( aRole == Qt::DisplayRole )
            return ToQString( wxGetTranslation( std::get<0>( columns[aSection] ) ) );

        if( aRole == Qt::TextAlignmentRole )
            return QVariant::fromValue( alignment( aSection ) );

        return QVariant();
    }

    /// Set the number of results (all the rows are new).
    void SetCount( int aCount )
    {
        beginResetModel();
        m_count = aCount;
        endResetModel();
    }

    /// The texts of the cells changed (after a sort, or with the units of the frame).
    void RefreshCells()
    {
        if( m_count > 0 && columnCount() > 0 )
            Q_EMIT dataChanged( index( 0, 0 ), index( m_count - 1, columnCount() - 1 ) );
    }

    /// The columns changed (the language of the GUI).
    void RefreshColumns()
    {
        beginResetModel();
        endResetModel();
    }

private:
    Qt::Alignment alignment( int aColumn ) const
    {
        std::vector<std::tuple<wxString, int, SEARCH_COLUMN_ALIGN>> columns =
                m_handler->GetColumns();

        if( aColumn < 0 || aColumn >= (int) columns.size() )
            return Qt::AlignLeft | Qt::AlignVCenter;

        switch( std::get<2>( columns[aColumn] ) )
        {
        case SEARCH_COLUMN_ALIGN::RIGHT:  return Qt::AlignRight | Qt::AlignVCenter;
        case SEARCH_COLUMN_ALIGN::CENTER: return Qt::AlignCenter;
        default:                          return Qt::AlignLeft | Qt::AlignVCenter;
        }
    }

    std::shared_ptr<SEARCH_HANDLER> m_handler;
    int                             m_count;
};


SEARCH_PANE_LISTVIEW::SEARCH_PANE_LISTVIEW( const std::shared_ptr<SEARCH_HANDLER>& aHandler,
                                            QWidget* aParent ) :
        QTableView( aParent ),
        m_handler( aHandler ),
        m_model( new SEARCH_RESULTS_MODEL( aHandler, this ) ),
        m_sortCol( -1 ),
        m_sortAscending( true ),
        m_selectionDirty( false )
{
    setModel( m_model );

    // A list of rows, as the report view of wxWidgets
    setSelectionBehavior( QAbstractItemView::SelectRows );
    setSelectionMode( QAbstractItemView::ExtendedSelection );
    setEditTriggers( QAbstractItemView::NoEditTriggers );
    setShowGrid( false );
    setWordWrap( false );
    setAlternatingRowColors( false );
    verticalHeader()->hide();
    verticalHeader()->setSectionResizeMode( QHeaderView::Fixed );
    verticalHeader()->setDefaultSectionSize( fontMetrics().height() + 4 );
    horizontalHeader()->setSectionsClickable( true );
    horizontalHeader()->setHighlightSections( false );
    horizontalHeader()->setStretchLastSection( false );
    horizontalHeader()->setSortIndicatorShown( false );

    RefreshColumnNames();

    QObject::connect( selectionModel(), &QItemSelectionModel::selectionChanged, this,
                      [this]()
                      {
                          m_selectionDirty = true;
                      } );

    QObject::connect( selectionModel(), &QItemSelectionModel::currentRowChanged, this,
                      [this]()
                      {
                          m_selectionDirty = true;
                      } );

    QObject::connect( this, &QAbstractItemView::activated, this,
                      [this]( const QModelIndex& aIndex )
                      {
                          long item = aIndex.row();

                          // After the event: activating an item can change the results
                          QMetaObject::invokeMethod( this,
                                                     [this, item]()
                                                     {
                                                         m_handler->ActivateItem( item );
                                                     },
                                                     Qt::QueuedConnection );

                          m_selectionDirty = true;
                      } );

    QObject::connect( horizontalHeader(), &QHeaderView::sectionClicked, this,
                      [this]( int aColumn )
                      {
                          onColClicked( aColumn );
                      } );

    // The items of the editor follow the selection of the list after the events
    KIQT::RegisterUpdateUI( this,
                            [this]()
                            {
                                if( m_selectionDirty )
                                {
                                    m_selectionDirty = false;

                                    std::vector<long> list;
                                    GetSelectRowsList( list );
                                    m_handler->SelectItems( list );
                                }
                            } );
}


SEARCH_PANE_LISTVIEW::~SEARCH_PANE_LISTVIEW()
{
}


void SEARCH_PANE_LISTVIEW::GetSelectRowsList( std::vector<long>& aSelectedList )
{
    QModelIndexList rows = selectionModel()->selectedRows();

    std::vector<long> selected;

    for( const QModelIndex& index : rows )
        selected.push_back( index.row() );

    // In the order of the rows, as the wxWidgets list
    std::sort( selected.begin(), selected.end() );

    aSelectedList.insert( aSelectedList.end(), selected.begin(), selected.end() );
}


void SEARCH_PANE_LISTVIEW::SetItemCount( int aCount )
{
    // The rows keep their selection (as the virtual list of wxWidgets): the selected rows that
    // remain are selected again, without a selection event
    std::vector<long> selection;
    GetSelectRowsList( selection );

    m_model->SetCount( aCount );

    QSignalBlocker blocker( selectionModel() );

    for( long row : selection )
    {
        if( row < aCount )
        {
            selectionModel()->select( m_model->index( (int) row, 0 ),
                                      QItemSelectionModel::Select
                                              | QItemSelectionModel::Rows );
        }
    }

    viewport()->update();
}


void SEARCH_PANE_LISTVIEW::onColClicked( int aColumn )
{
    if( aColumn == m_sortCol )
    {
        m_sortAscending = !m_sortAscending;
    }
    else
    {
        m_sortAscending = true;
        m_sortCol = aColumn;
    }

    horizontalHeader()->setSortIndicatorShown( true );
    horizontalHeader()->setSortIndicator( m_sortCol, m_sortAscending ? Qt::AscendingOrder
                                                                     : Qt::DescendingOrder );

    std::vector<long> selection = Sort();

    QItemSelection newSelection;

    for( long row = 0; row < m_model->rowCount(); row++ )
    {
        if( alg::contains( selection, row ) )
        {
            newSelection.select( m_model->index( (int) row, 0 ),
                                 m_model->index( (int) row, m_model->columnCount() - 1 ) );
        }
    }

    selectionModel()->select( newSelection, QItemSelectionModel::ClearAndSelect );
    m_model->RefreshCells();
}


void SEARCH_PANE_LISTVIEW::keyPressEvent( QKeyEvent* aEvent )
{
    if( aEvent->matches( QKeySequence::Copy ) )
    {
        CopySelectionToClipboard();
        aEvent->accept();
        return;
    }

    // The other keys of the list (select all, the moves of the selection) are the keys of the
    // table of Qt
    QTableView::keyPressEvent( aEvent );
}


void SEARCH_PANE_LISTVIEW::contextMenuEvent( QContextMenuEvent* aEvent )
{
    QMenu menu( this );

    QObject::connect( menu.addAction( ToQString( _( "Copy" ) ) ), &QAction::triggered, this,
                      [this]()
                      {
                          CopySelectionToClipboard();
                      } );

    menu.exec( aEvent->globalPos() );
}


void SEARCH_PANE_LISTVIEW::CopySelectionToClipboard()
{
    wxString txt;

    for( int row = 0; row < m_model->rowCount(); row++ )
    {
        if( !selectionModel()->isRowSelected( row, QModelIndex() ) )
            continue;

        bool firstCol = true;

        for( int col = 0; col < m_model->columnCount(); col++ )
        {
            if( isColumnHidden( col ) || columnWidth( col ) <= 0 )
                continue;

            if( !firstCol )
                txt += wxT( "\t" );

            txt += m_handler->GetResultCell( row, col );
            firstCol = false;
        }

        txt += wxT( "\n" );
    }

    QGuiApplication::clipboard()->setText( ToQString( txt ) );
}


std::vector<long> SEARCH_PANE_LISTVIEW::Sort()
{
    std::vector<long> selection;
    GetSelectRowsList( selection );

    m_handler->Sort( m_sortCol, m_sortAscending, &selection );

    return selection;
}


void SEARCH_PANE_LISTVIEW::RefreshColumnNames()
{
    m_model->RefreshColumns();

    std::vector<std::tuple<wxString, int, SEARCH_COLUMN_ALIGN>> columns = m_handler->GetColumns();

    int widthUnit = viewport()->width() / 10;

    // Before the pane has its size: the width of the pane given to the table
    if( widthUnit <= 0 )
        widthUnit = std::max( 1, width() / 10 );

    for( int ii = 0; ii < (int) columns.size(); ++ii )
        setColumnWidth( ii, widthUnit * std::get<1>( columns[ii] ) );

    if( m_sortCol >= 0 )
    {
        horizontalHeader()->setSortIndicator( m_sortCol, m_sortAscending ? Qt::AscendingOrder
                                                                         : Qt::DescendingOrder );
    }
}


SEARCH_PANE_TAB::SEARCH_PANE_TAB( const std::shared_ptr<SEARCH_HANDLER>& aHandler,
                                  QWidget* aParent ) :
        QWidget( aParent ),
        m_handler( aHandler )
{
    QVBoxLayout* sizer = new QVBoxLayout( this );
    sizer->setContentsMargins( 0, 0, 1, 1 );

    m_listView = new SEARCH_PANE_LISTVIEW( aHandler, this );
    sizer->addWidget( m_listView, 5 );
}


void SEARCH_PANE_TAB::Search( wxString& query )
{
    int results = m_handler->Search( query );
    m_listView->SetItemCount( results );
    m_listView->Sort();
    m_listView->viewport()->update();
}


void SEARCH_PANE_TAB::Clear()
{
    m_listView->SetItemCount( 0 );
}


void SEARCH_PANE_TAB::RefreshColumnNames()
{
    m_listView->RefreshColumnNames();
}


class SEARCH_PANE_MENU : public ACTION_MENU
{
public:
    SEARCH_PANE_MENU( SEARCH_PANE* aSearchPane, EDA_DRAW_FRAME& aFrame ) :
            ACTION_MENU( true, nullptr ),
            m_frame( aFrame ),
            m_searchPane( aSearchPane )
    {
        Add( _( "Zoom to Selection" ), _( "Toggle zooming to selections in the search pane" ),
             ID_TOGGLE_ZOOM_TO_SELECTION, BITMAPS::zoom_fit_to_objects, true );
        Add( _( "Pan to Selection" ), _( "Toggle panning to selections in the search pane" ),
             ID_TOGGLE_PAN_TO_SELECTION, BITMAPS::zoom_center_on_screen, true );

        AppendSeparator();
        Add( _( "Search Hidden Fields" ), wxEmptyString,
             ID_TOGGLE_SEARCH_HIDDEN_FIELDS, BITMAPS::invisible_text, true );
        Add( _( "Search Metadata" ), _( "Search library links, descriptions and keywords" ),
             ID_TOGGLE_SEARCH_METADATA, BITMAPS::library, true );

        updateZoomPanCheckboxes();
    }

    OPT_TOOL_EVENT eventHandler( int aId ) override
    {
        APP_SETTINGS_BASE::SEARCH_PANE& settings = m_frame.config()->m_SearchPane;
        const int                       id = aId;
        const bool                      checked = IsChecked( id );

        switch( id )
        {
        case ID_TOGGLE_ZOOM_TO_SELECTION:
            settings.selection_zoom = checked ? APP_SETTINGS_BASE::SEARCH_PANE::SELECTION_ZOOM::ZOOM
                                              : APP_SETTINGS_BASE::SEARCH_PANE::SELECTION_ZOOM::NONE;
            updateZoomPanCheckboxes();
            break;

        case ID_TOGGLE_PAN_TO_SELECTION:
            settings.selection_zoom = checked ? APP_SETTINGS_BASE::SEARCH_PANE::SELECTION_ZOOM::PAN
                                              : APP_SETTINGS_BASE::SEARCH_PANE::SELECTION_ZOOM::NONE;
            updateZoomPanCheckboxes();
            break;

        case ID_TOGGLE_SEARCH_HIDDEN_FIELDS:
            settings.search_hidden_fields = checked;
            updateZoomPanCheckboxes();
            m_searchPane->RefreshSearch();
            break;

        case ID_TOGGLE_SEARCH_METADATA:
            settings.search_metadata = checked;
            updateZoomPanCheckboxes();
            m_searchPane->RefreshSearch();
            break;
        }

        return OPT_TOOL_EVENT();
    }

private:
    void updateZoomPanCheckboxes()
    {
        APP_SETTINGS_BASE::SEARCH_PANE& settings = m_frame.config()->m_SearchPane;

        Check( ID_TOGGLE_ZOOM_TO_SELECTION,
               settings.selection_zoom == APP_SETTINGS_BASE::SEARCH_PANE::SELECTION_ZOOM::ZOOM );
        Check( ID_TOGGLE_PAN_TO_SELECTION,
               settings.selection_zoom == APP_SETTINGS_BASE::SEARCH_PANE::SELECTION_ZOOM::PAN );
        Check( ID_TOGGLE_SEARCH_HIDDEN_FIELDS, settings.search_hidden_fields );
        Check( ID_TOGGLE_SEARCH_METADATA, settings.search_metadata );
    }

private:
    EDA_DRAW_FRAME& m_frame;
    SEARCH_PANE*    m_searchPane;
};


SEARCH_PANE::SEARCH_PANE( EDA_DRAW_FRAME* aFrame ) :
        SEARCH_PANE_BASE( aFrame ),
        m_frame( aFrame ),
        m_closeWatched( false )
{
    m_frame->AddLanguageChangedListener( this,
                                         [this]()
                                         {
                                             onLanguageChange();
                                         } );

    m_menu = new SEARCH_PANE_MENU( this, *m_frame );

    m_searchCtrl1->setPlaceholderText( ToQString( _( "Search" ) ) );

    m_menuButton->SetBitmap( KiBitmapBundle( BITMAPS::config ) );

    QObject::connect( m_menuButton, &QAbstractButton::pressed, this,
                      [this]()
                      {
                          m_menu->exec( QCursor::pos() );
                      } );

    QFont infoFont = KIQT::DockedPaneFont( this );
    setFont( infoFont );
    m_notebook->setFont( infoFont );
}


SEARCH_PANE::~SEARCH_PANE()
{
    m_handlers.clear();

    delete m_menu;
}


void SEARCH_PANE::onLanguageChange()
{
    m_searchCtrl1->setPlaceholderText( ToQString( _( "Search" ) ) );

    for( int i = 0; i < m_notebook->count(); ++i )
    {
        SEARCH_PANE_TAB* tab = dynamic_cast<SEARCH_PANE_TAB*>( m_notebook->widget( i ) );

        KI_CHECK( tab, /* void */ );

        tab->RefreshColumnNames();
        m_notebook->setTabText( i, ToQString( wxGetTranslation( tab->GetSearchHandler()->GetName() ) ) );
    }
}


void SEARCH_PANE::AddSearcher( const std::shared_ptr<SEARCH_HANDLER>& aHandler )
{
    SEARCH_PANE_TAB* tab = new SEARCH_PANE_TAB( aHandler, m_notebook );

    m_notebook->addTab( tab, ToQString( wxGetTranslation( aHandler->GetName() ) ) );
    m_handlers.push_back( aHandler );
    m_tabs.push_back( tab );
}


void SEARCH_PANE::RefreshSearch()
{
    SEARCH_PANE_TAB* tab = GetCurrentTab();

    if( tab )
        tab->Search( m_lastQuery );
}


void SEARCH_PANE::ClearAllResults()
{
    for( SEARCH_PANE_TAB* tab : m_tabs )
        tab->Clear();
}


void SEARCH_PANE::OnSearchTextEntry( QWidget* aSource )
{
    m_lastQuery = ToWxString( m_searchCtrl1->text() );

    RefreshSearch();
}


void SEARCH_PANE::FocusSearch()
{
    m_searchCtrl1->setFocus();
}


void SEARCH_PANE::OnNotebookPageChanged( int aPage )
{
    SEARCH_PANE_TAB* tab = GetCurrentTab();

    if( tab )
        tab->Search( m_lastQuery );
}


bool SEARCH_PANE::IsShownOnScreen() const
{
    return isVisible() && !visibleRegion().isEmpty();
}


void SEARCH_PANE::showEvent( QShowEvent* aEvent )
{
    SEARCH_PANE_BASE::showEvent( aEvent );

    // The pane is docked by the frame after its creation: the dock is known when it is shown
    if( m_closeWatched )
        return;

    if( QDockWidget* dock = qobject_cast<QDockWidget*>( parentWidget() ) )
    {
        m_closeWatched = true;

        QObject::connect( dock, &QDockWidget::visibilityChanged, this,
                          [this, dock]( bool aVisible )
                          {
                              // Hidden behind another tab of the dock is not closed
                              if( !aVisible && dock->isHidden() )
                                  onClosed();
                          } );
    }
}


void SEARCH_PANE::onClosed()
{
    if( APP_SETTINGS_BASE* cfg = m_frame->config() )
        m_frame->SaveSettings( cfg );
}


SEARCH_PANE_TAB* SEARCH_PANE::GetCurrentTab() const
{
    return dynamic_cast<SEARCH_PANE_TAB*>( m_notebook->currentWidget() );
}
