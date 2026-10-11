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
 * @file pcb_net_inspector_panel_qt.cpp
 * The view of the Net Inspector with Qt (pcbnew/widgets/pcb_net_inspector_panel_wx.cpp with
 * wxWidgets): the QTreeView of the nets on their DATA_MODEL, sorted by a SORT_PROXY, the columns,
 * the menus and the dialogs.  What the panel does is in pcbnew/widgets/pcb_net_inspector_panel.cpp.
 */

#include <widgets/pcb_net_inspector_panel.h>
#include <widgets/pcb_net_inspector_panel_data_model.h>

#include <QAction>
#include <QCursor>
#include <QFileDialog>
#include <QFontMetrics>
#include <QGridLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QItemSelectionModel>
#include <QLineEdit>
#include <QMenu>
#include <QSignalBlocker>
#include <QSortFilterProxyModel>
#include <QTreeView>

#include <bitmaps.h>
#include <class_draw_panel_gal.h>
#include <pcb_edit_frame.h>
#include <pcb_painter.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <view/view.h>
#include <widgets/bitmap_button.h>
#include <wildcards_and_files_ext.h>

#include <core/ki_assert.h>
#include <qt_strings.h>
#include <qt_ui.h>


/*****************************************************************************************
 *
 * NET_INSPECTOR_PANEL
 *
 * ***************************************************************************************/


NET_INSPECTOR_PANEL::NET_INSPECTOR_PANEL( QWidget* aParent, EDA_BASE_FRAME* aFrame ) :
        QWidget( aParent ),
        m_frame( aFrame )
{
    QGridLayout* sizer = new QGridLayout( this );
    sizer->setContentsMargins( 0, 0, 0, 0 );
    sizer->setSpacing( 5 );

    m_searchCtrl = new QLineEdit( this );
    m_searchCtrl->setPlaceholderText( ToQString( _( "Net name filter" ) ) );
    m_searchCtrl->setClearButtonEnabled( true );
    sizer->addWidget( m_searchCtrl, 0, 0 );

    m_configureBtn = new BITMAP_BUTTON( this );
    m_configureBtn->SetBitmap( KiBitmapBundle( BITMAPS::config ) );
    m_configureBtn->SetPadding( 2 );
    sizer->addWidget( m_configureBtn, 0, 1 );

    m_netsList = new QTreeView( this );
    m_netsList->setSelectionMode( QAbstractItemView::ExtendedSelection );
    m_netsList->setSelectionBehavior( QAbstractItemView::SelectRows );
    m_netsList->setUniformRowHeights( true );
    m_netsList->setAlternatingRowColors( true );
    m_netsList->setContextMenuPolicy( Qt::CustomContextMenu );
    m_netsList->header()->setContextMenuPolicy( Qt::CustomContextMenu );
    m_netsList->header()->setSectionsMovable( true );
    m_netsList->header()->setStretchLastSection( false );
    m_netsList->setFont( KIQT::InfoFont( this ) );
    sizer->addWidget( m_netsList, 1, 0, 1, 2 );

    sizer->setRowStretch( 1, 1 );
    sizer->setColumnStretch( 0, 1 );

    QObject::connect( m_searchCtrl, &QLineEdit::textChanged, this,
                      [this]() { OnSearchTextChanged(); } );

    QObject::connect( m_configureBtn, &QAbstractButton::clicked, this,
                      [this]() { OnConfigButton(); } );

    if( m_frame )
        m_frame->AddLanguageChangedListener( this, [this]() { OnLanguageChangedImpl(); } );
}


NET_INSPECTOR_PANEL::~NET_INSPECTOR_PANEL()
{
}


/*****************************************************************************************
 *
 * The sorting of the view
 *
 * ***************************************************************************************/


/**
 * The sorting of the nets (DATA_MODEL::Compare() of the wxWidgets build): the groups first, then
 * by the values of the column, then by the order of the entries.
 */
class PCB_NET_INSPECTOR_PANEL::SORT_PROXY : public QSortFilterProxyModel
{
public:
    SORT_PROXY( PCB_NET_INSPECTOR_PANEL& aParent ) :
            QSortFilterProxyModel( &aParent ),
            m_parent( aParent )
    {
        setDynamicSortFilter( true );
    }

protected:
    bool lessThan( const QModelIndex& aLeft, const QModelIndex& aRight ) const override
    {
        const LIST_ITEM* left = DATA_MODEL::itemOf( aLeft );
        const LIST_ITEM* right = DATA_MODEL::itemOf( aRight );

        if( !left || !right )
            return false;

        bool timeDomain = m_parent.m_dataModel->IsTimeDomain();
        int  col = aLeft.column();

        // Qt reverses the order for a descending sort, KiCad compares for the sort order (the
        // groups stay first)
        if( sortOrder() == Qt::AscendingOrder )
        {
            if( int res = m_parent.compareItems( *left, *right, col, true, timeDomain ) )
                return res < 0;

            return left < right;
        }

        if( int res = m_parent.compareItems( *right, *left, col, false, timeDomain ) )
            return res < 0;

        return right < left;
    }

private:
    PCB_NET_INSPECTOR_PANEL& m_parent;
};


/*****************************************************************************************
 *
 * PCB_NET_INSPECTOR_PANEL
 *
 * ***************************************************************************************/


PCB_NET_INSPECTOR_PANEL::PCB_NET_INSPECTOR_PANEL( QWidget* parent, PCB_EDIT_FRAME* aFrame ) :
        NET_INSPECTOR_PANEL( parent, aFrame ),
        m_frame( aFrame ),
        m_dataModel( new DATA_MODEL( *this ) ),
        m_proxy( new SORT_PROXY( *this ) )
{
    m_board = m_frame->GetBoard();

    m_proxy->setSourceModel( m_dataModel );
    m_netsList->setModel( m_proxy );
    m_netsList->setSortingEnabled( true );

    // Rebuild nets list
    buildNetsList( true );

    // Register the panel to receive board change notifications
    if( m_board != nullptr )
    {
        PCB_NET_INSPECTOR_PANEL::OnBoardHighlightNetChanged( *m_board );
        m_board->AddListener( this );
    }

    // Connect to board events
    m_unitsListener = m_frame->AddUnitsChangedListener(
            [this]( EDA_BASE_FRAME* )
            {
                onUnitsChanged();
            } );

    // Connect to the view events
    QObject::connect( m_netsList, &QTreeView::expanded, this, [this]() { OnExpandCollapseRow(); } );
    QObject::connect( m_netsList, &QTreeView::collapsed, this, [this]() { OnExpandCollapseRow(); } );

    QObject::connect( m_netsList->header(), &QWidget::customContextMenuRequested, this,
                      [this]( const QPoint& aPos ) { OnHeaderContextMenu( aPos ); } );

    QObject::connect( m_netsList, &QWidget::customContextMenuRequested, this,
                      [this]( const QPoint& aPos ) { OnNetsListContextMenu( aPos ); } );

    QObject::connect( m_netsList, &QAbstractItemView::activated, this,
                      [this]() { OnNetsListItemActivated(); } );

    QObject::connect( m_netsList->header(), &QHeaderView::sortIndicatorChanged, this,
                      [this]() { OnColumnSorted(); } );
}


PCB_NET_INSPECTOR_PANEL::~PCB_NET_INSPECTOR_PANEL()
{
    PCB_NET_INSPECTOR_PANEL::SaveSettings();

    if( m_unitsListener >= 0 )
        m_frame->RemoveUnitsChangedListener( m_unitsListener );

    if( m_board )
        m_board->RemoveListener( this );

    // The view must not use the model while it is destroyed
    m_netsList->setModel( nullptr );
}


/*****************************************************************************************
 *
 * Grid / model columns configuration
 *
 * ***************************************************************************************/


void PCB_NET_INSPECTOR_PANEL::viewBuildColumns( PANEL_NET_INSPECTOR_SETTINGS* cfg )
{
    // The model has the columns of m_columns: the header shows them in the order of the settings
    m_dataModel->resetColumns();

    QHeaderView* header = m_netsList->header();
    QSignalBlocker blocker( header );

    for( int visual = 0; visual < static_cast<int>( cfg->col_order.size() ); ++visual )
    {
        int logical = cfg->col_order[visual];

        if( logical < 0 || logical >= header->count() )
            continue;

        int from = header->visualIndex( logical );

        if( from != visual )
            header->moveSection( from, visual );
    }

    // The name column is the first one, with the expanders of the groups
    if( header->visualIndex( COLUMN_NAME ) != 0 )
        header->moveSection( header->visualIndex( COLUMN_NAME ), 0 );

    adjustListColumnSizes( cfg );
}


int PCB_NET_INSPECTOR_PANEL::getMinColumnWidth( int aModelColumn ) const
{
    constexpr int margins = 15;
    constexpr int extra_width = 30;

    QFontMetrics metrics( m_netsList->font() );
    int headerWidth = metrics.horizontalAdvance( ToQString( m_columns[aModelColumn].display_name ) );
    headerWidth += ( aModelColumn == COLUMN_NAME ) ? extra_width : margins;

    return headerWidth;
}


void PCB_NET_INSPECTOR_PANEL::autosizeColumn( int aModelColumn )
{
    if( aModelColumn < 0 || aModelColumn >= static_cast<int>( m_columns.size() )
        || m_netsList->header()->isSectionHidden( aModelColumn ) )
    {
        return;
    }

    constexpr int margins = 15;
    constexpr int extra_width = 30;
    const bool    isNameCol = ( aModelColumn == COLUMN_NAME );
    const int     padding = isNameCol ? extra_width : margins;
    const int     indent = m_netsList->indentation();
    QFontMetrics  metrics( m_netsList->font() );
    bool          timeDomain = m_dataModel->IsTimeDomain();

    int maxWidth = getMinColumnWidth( aModelColumn );

    for( unsigned int row = 0; row < m_dataModel->itemCount(); ++row )
    {
        const LIST_ITEM& item = m_dataModel->itemAt( row );
        int width = metrics.horizontalAdvance( ToQString( itemText( &item, aModelColumn, timeDomain ) ) ) + padding;

        if( isNameCol && item.Parent() )
            width += indent;

        maxWidth = std::max( maxWidth, width );
    }

    m_netsList->header()->resizeSection( aModelColumn, maxWidth );
}


void PCB_NET_INSPECTOR_PANEL::adjustListColumnSizes( PANEL_NET_INSPECTOR_SETTINGS* cfg ) const
{
    QHeaderView* header = m_netsList->header();
    QFontMetrics metrics( m_netsList->font() );

    auto textWidth =
            [&]( const wxString& aText )
            {
                return metrics.horizontalAdvance( ToQString( aText ) );
            };

    if( cfg->col_widths.size() != m_columns.size() )
    {
        int minValueWidth = textWidth( wxT( "00000,000 mm" ) );
        int minNumberWidth = textWidth( wxT( "000" ) );
        int minNameWidth = textWidth( wxT( "MMMMMMMMMMMM" ) );

        // Considering left and right margins, the sorting arrow in the column header and the
        // expander icons of the first column
        constexpr int margins = 15;
        constexpr int extra_width = 30;

        auto getTargetWidth =
                [&]( int columnID )
                {
                    switch( columnID )
                    {
                    case COLUMN_NAME:      return minNameWidth + extra_width;
                    case COLUMN_NETCLASS:  return minNameWidth + margins;
                    case COLUMN_VIA_COUNT: return minNumberWidth + margins;
                    case COLUMN_PAD_COUNT: return minNumberWidth + margins;
                    default:               return minValueWidth + margins;
                    }
                };

        for( size_t i = 0; i < m_columns.size(); ++i )
        {
            int titleSize = textWidth( m_columns[i].display_name );
            titleSize = i == COLUMN_NAME ? titleSize + extra_width : titleSize + margins;
            header->resizeSection( static_cast<int>( i ), std::max( titleSize, getTargetWidth( i ) ) );
            header->setSectionHidden( static_cast<int>( i ), false );
        }
    }
    else
    {
        KI_ASSERT( m_columns.size() == cfg->col_hidden.size() );

        // The widths and the hidden flags of the settings are in the order of the columns shown
        for( size_t ii = 0; ii < m_columns.size() && ii < cfg->col_order.size(); ++ii )
        {
            const int modelCol = cfg->col_order[ii];

            if( modelCol < 0 || modelCol >= static_cast<int>( m_columns.size() ) )
                continue;

            header->resizeSection( modelCol, std::max( cfg->col_widths[ii], getMinColumnWidth( modelCol ) ) );
            header->setSectionHidden( modelCol, cfg->col_hidden[ii] );
        }
    }
}


bool PCB_NET_INSPECTOR_PANEL::restoreSortColumn( const int sortingColumnId, const bool sortOrderAsc ) const
{
    if( sortingColumnId >= 0 && sortingColumnId < static_cast<int>( m_columns.size() ) )
    {
        m_netsList->sortByColumn( sortingColumnId, sortOrderAsc ? Qt::AscendingOrder : Qt::DescendingOrder );
        return true;
    }

    return false;
}


/*****************************************************************************************
 *
 * Nets list generation
 *
 * ***************************************************************************************/


void PCB_NET_INSPECTOR_PANEL::buildNetsList( const bool rebuildColumns )
{
    // Only build the list of nets if there is a board present
    if( !m_board )
        return;

    m_inBuildNetsList = true;

    m_netsList->setUpdatesEnabled( false );

    m_dataModel->SetIsTimeDomain( m_showTimeDomainDetails );

    PROJECT_LOCAL_SETTINGS& localSettings = Pgm().GetSettingsManager().Prj().GetLocalSettings();
    PANEL_NET_INSPECTOR_SETTINGS* cfg = &localSettings.m_NetInspectorPanel;

    // Refresh all filtering / grouping settings
    readFilterSettings( cfg );

    // Attempt to keep any expanded groups open
    if( m_boardLoaded && !m_boardLoading )
    {
        cfg->expanded_rows.clear();

        for( const auto& [groupName, groupItem] : m_dataModel->getGroupItems() )
        {
            if( m_netsList->isExpanded( viewIndexOf( groupItem ) ) )
                cfg->expanded_rows.push_back( groupName );
        }
    }

    // When rebuilding the netlist, try to keep the row selection
    std::vector<int> prev_selected_netcodes;

    for( const LIST_ITEM* item : selectedItems() )
        prev_selected_netcodes.push_back( item->GetNetCode() );

    int  sorting_column_id = cfg->sorting_column;
    bool sort_order_asc = cfg->sort_order_asc;

    if( !m_boardLoading && m_netsList->header()->isSortIndicatorShown() && m_columns.size() )
    {
        sorting_column_id = m_netsList->header()->sortIndicatorSection();
        sort_order_asc = m_netsList->header()->sortIndicatorOrder() == Qt::AscendingOrder;
    }

    if( rebuildColumns )
        buildColumns();

    m_dataModel->SetIsTimeDomain( m_showTimeDomainDetails );

    fillDataModel( cfg );

    // Try to re-enable the sorting column
    if( !restoreSortColumn( sorting_column_id, sort_order_asc ) )
    {
        // By default, sort by Name column
        restoreSortColumn( COLUMN_NAME, true );
    }

    // Try to restore the expanded groups
    if( m_boardLoaded )
    {
        m_rowExpanding = true;

        std::vector<std::pair<wxString, LIST_ITEM*>> groupItems = m_dataModel->getGroupItems();

        for( wxString& groupName : cfg->expanded_rows )
        {
            auto tableItem = std::ranges::find_if( groupItems,
                                                   [&groupName]( const std::pair<wxString, LIST_ITEM*>& item )
                                                   {
                                                       return groupName == item.first;
                                                   } );

            if( tableItem != groupItems.end() )
                m_netsList->expand( viewIndexOf( tableItem->second ) );
        }

        m_rowExpanding = false;
    }

    m_netsList->setUpdatesEnabled( true );

    // Try to restore the selected rows
    std::vector<const LIST_ITEM*> sel;

    for( const int& nc : prev_selected_netcodes )
    {
        if( std::optional<LIST_ITEM_ITER> r = m_dataModel->findItem( nc ) )
            sel.push_back( r.value()->get() );
    }

    selectItems( sel );

    m_inBuildNetsList = false;
}


void PCB_NET_INSPECTOR_PANEL::updateDisplayedRowValues( const std::optional<LIST_ITEM_ITER>& aRow ) const
{
    if( !aRow )
        return;

    // The selection of the view follows its entries
    m_dataModel->updateItem( aRow );
}


/*****************************************************************************************
 *
 * UI-generated event handling
 *
 * ***************************************************************************************/


void PCB_NET_INSPECTOR_PANEL::execMenu( QMenu& aMenu )
{
    aMenu.exec( QCursor::pos() );
}


void PCB_NET_INSPECTOR_PANEL::OnNetsListContextMenu( const QPoint& aPos )
{
    std::vector<const LIST_ITEM*> selection = selectedItems();
    bool                          multipleSelections = selection.size() > 1;
    const LIST_ITEM*              selItem = selection.size() == 1 ? selection.front() : nullptr;

    QMenu menu( this );

    auto addItem =
            [&]( int aId, const wxString& aText )
            {
                QAction* action = menu.addAction( ToQString( aText ) );
                QObject::connect( action, &QAction::triggered, this,
                                  [this, aId]() { doContextMenuCommand( aId ); } );
                return action;
            };

    // Net edit menu items
    QAction* highlightNet = addItem( ID_HIGHLIGHT_SELECTED_NETS, _( "Highlight Selected Net" ) );
    QAction* clearHighlighting = addItem( ID_CLEAR_HIGHLIGHTING, _( "Clear Net Highlighting" ) );

    RENDER_SETTINGS* renderSettings = m_frame->GetCanvas()->GetView()->GetPainter()->GetSettings();
    const std::set<int>& selected_codes = renderSettings->GetHighlightNetCodes();

    if( selected_codes.size() == 0 )
        clearHighlighting->setEnabled( false );

    menu.addSeparator();

    QAction* renameNet = addItem( ID_RENAME_NET, _( "Rename Selected Net..." ) );
    QAction* deleteNet = addItem( ID_DELETE_NET, _( "Delete Selected Net" ) );

    menu.addSeparator();

    addItem( ID_ADD_NET, _( "Add Net..." ) );

    if( !selItem && !multipleSelections )
    {
        highlightNet->setEnabled( false );
        deleteNet->setEnabled( false );
        renameNet->setEnabled( false );
    }
    else
    {
        if( multipleSelections || selItem->GetIsGroup() )
        {
            highlightNet->setText( ToQString( _( "Highlight Selected Nets" ) ) );
            renameNet->setEnabled( false );
            deleteNet->setText( ToQString( _( "Delete Selected Nets" ) ) );
        }
    }

    menu.addSeparator();

    QAction* removeSelectedGroup = addItem( ID_REMOVE_SELECTED_GROUP, _( "Remove Selected Custom Group" ) );

    if( !selItem || !selItem->GetIsGroup() )
        removeSelectedGroup->setEnabled( false );

    execMenu( menu );
}


void PCB_NET_INSPECTOR_PANEL::OnSearchTextChanged()
{
    SaveSettings();
    buildNetsList();
}


void PCB_NET_INSPECTOR_PANEL::OnExpandCollapseRow()
{
    if( !m_rowExpanding )
        SaveSettings();
}


void PCB_NET_INSPECTOR_PANEL::OnHeaderContextMenu( const QPoint& aPos )
{
    m_contextMenuColumn = m_netsList->header()->logicalIndexAt( aPos );

    QMenu menu( this );

    for( auto [id, text] : { std::pair<int, wxString>( ID_AUTOFIT_COLUMN, _( "Auto-fit Column" ) ),
                             std::pair<int, wxString>( ID_AUTOFIT_ALL_COLUMNS, _( "Auto-fit All Columns" ) ) } )
    {
        QAction* action = menu.addAction( ToQString( text ) );
        QObject::connect( action, &QAction::triggered, this,
                          [this, id]() { doContextMenuCommand( id ); } );
    }

    menu.addSeparator();

    generateShowHideColumnMenu( &menu );
    execMenu( menu );
}


void PCB_NET_INSPECTOR_PANEL::OnConfigButton()
{
    PROJECT_LOCAL_SETTINGS& localSettings = Pgm().GetSettingsManager().Prj().GetLocalSettings();
    auto&                   cfg = localSettings.m_NetInspectorPanel;

    std::vector<const LIST_ITEM*> selection = selectedItems();
    const LIST_ITEM*              selItem = selection.size() == 1 ? selection.front() : nullptr;

    QMenu menu( this );

    auto addItem =
            [&]( QMenu* aMenu, int aId, const wxString& aText, std::optional<bool> aChecked = std::nullopt )
            {
                QAction* action = aMenu->addAction( ToQString( aText ) );

                if( aChecked )
                {
                    action->setCheckable( true );
                    action->setChecked( *aChecked );
                }

                QObject::connect( action, &QAction::triggered, this,
                                  [this, aId]() { doContextMenuCommand( aId ); } );
                return action;
            };

    // Filtering menu items
    addItem( &menu, ID_FILTER_BY_NET_NAME, _( "Filter by Net Name" ), cfg.filter_by_net_name );
    addItem( &menu, ID_FILTER_BY_NETCLASS, _( "Filter by Netclass" ), cfg.filter_by_netclass );

    menu.addSeparator();

    // Grouping menu items
    addItem( &menu, ID_GROUP_BY_NETCLASS, _( "Group by Netclass" ), m_groupByNetclass );

    menu.addSeparator();

    addItem( &menu, ID_ADD_GROUP, _( "Add Custom Group..." ) );

    QAction* removeSelectedGroup = addItem( &menu, ID_REMOVE_SELECTED_GROUP, _( "Remove Selected Custom Group" ) );

    if( !selItem || !selItem->GetIsGroup() )
        removeSelectedGroup->setEnabled( false );

    QAction* removeCustomGroups = addItem( &menu, ID_REMOVE_GROUPS, _( "Remove All Custom Groups" ) );
    removeCustomGroups->setEnabled( m_custom_group_rules.size() != 0 );

    menu.addSeparator();

    addItem( &menu, ID_SHOW_ZERO_NET_PADS, _( "Show Zero Pad Nets" ), m_showZeroPadNets );
    addItem( &menu, ID_SHOW_UNCONNECTED_NETS, _( "Show Unconnected Nets" ), m_showUnconnectedNets );

    menu.addSeparator();

    addItem( &menu, ID_SHOW_TIME_DOMAIN_DETAILS, _( "Show Time Domain Details" ), m_showTimeDomainDetails );

    menu.addSeparator();

    // Report generation
    addItem( &menu, ID_GENERATE_REPORT, _( "Save Net Inspector Report..." ) );

    menu.addSeparator();

    // Show / hide columns menu items
    QMenu* colsMenu = menu.addMenu( ToQString( _( "Show / Hide Columns" ) ) );
    generateShowHideColumnMenu( colsMenu );

    execMenu( menu );
}


void PCB_NET_INSPECTOR_PANEL::generateShowHideColumnMenu( QMenu* target )
{
    auto addColumn =
            [&]( int aColumn )
            {
                QAction* action = target->addAction( ToQString( m_columns[aColumn].display_name ) );
                action->setCheckable( true );
                action->setChecked( !m_netsList->header()->isSectionHidden( aColumn ) );

                QObject::connect( action, &QAction::triggered, this,
                                  [this, aColumn]() { doContextMenuCommand( ID_HIDE_COLUMN + aColumn ); } );
            };

    for( int i = 1; i <= COLUMN_LAST_STATIC_COL; ++i )
        addColumn( i );

    target->addSeparator();

    for( std::size_t i = COLUMN_LAST_STATIC_COL + 1; i < m_columns.size(); ++i )
        addColumn( static_cast<int>( i ) );
}


void PCB_NET_INSPECTOR_PANEL::OnNetsListItemActivated()
{
    highlightSelectedNets();
}


void PCB_NET_INSPECTOR_PANEL::OnColumnSorted()
{
    if( !m_inBuildNetsList )
        SaveSettings();
}


void PCB_NET_INSPECTOR_PANEL::onUnitsChanged()
{
    m_dataModel->updateAllItems();
}


void PCB_NET_INSPECTOR_PANEL::viewColumnCommand( int aId )
{
    if( aId == ID_AUTOFIT_COLUMN )
    {
        if( m_contextMenuColumn >= 0 )
            autosizeColumn( m_contextMenuColumn );
    }
    else if( aId == ID_AUTOFIT_ALL_COLUMNS )
    {
        for( int i = 0; i < static_cast<int>( m_columns.size() ); ++i )
            autosizeColumn( i );
    }
    else if( aId >= ID_HIDE_COLUMN )
    {
        const int    columnId = aId - ID_HIDE_COLUMN;
        QHeaderView* header = m_netsList->header();

        if( columnId >= 0 && columnId < header->count() )
        {
            // Make sure we end up with something non-zero so we can resize it
            header->resizeSection( columnId, std::max( header->sectionSize( columnId ), 10 ) );
            header->setSectionHidden( columnId, !header->isSectionHidden( columnId ) );
        }
    }
}


/*****************************************************************************************
 *
 * The view of the GUI toolkit
 *
 * ***************************************************************************************/


QModelIndex PCB_NET_INSPECTOR_PANEL::viewIndexOf( const LIST_ITEM* aItem ) const
{
    return m_proxy->mapFromSource( m_dataModel->indexOf( aItem ) );
}


const PCB_NET_INSPECTOR_PANEL::LIST_ITEM*
PCB_NET_INSPECTOR_PANEL::itemOfViewIndex( const QModelIndex& aIndex ) const
{
    return DATA_MODEL::itemOf( m_proxy->mapToSource( aIndex ) );
}


wxString PCB_NET_INSPECTOR_PANEL::searchText() const
{
    return ToWxString( m_searchCtrl->text() );
}


void PCB_NET_INSPECTOR_PANEL::setSearchText( const wxString& aText )
{
    QSignalBlocker blocker( m_searchCtrl );
    m_searchCtrl->setText( ToQString( aText ) );
}


bool PCB_NET_INSPECTOR_PANEL::isPanelShown() const
{
    return isVisible();
}


bool PCB_NET_INSPECTOR_PANEL::isViewDisplayed() const
{
    QHeaderView* header = m_netsList->header();

    for( int ii = 0; ii < header->count(); ++ii )
    {
        if( header->sectionSize( ii ) > 0 )
            return true;
    }

    return false;
}


void PCB_NET_INSPECTOR_PANEL::refreshList() const
{
    m_netsList->viewport()->update();
}


void PCB_NET_INSPECTOR_PANEL::freezeList() const
{
    m_netsList->setUpdatesEnabled( false );
}


void PCB_NET_INSPECTOR_PANEL::thawList() const
{
    m_netsList->setUpdatesEnabled( true );
}


std::vector<const PCB_NET_INSPECTOR_PANEL::LIST_ITEM*> PCB_NET_INSPECTOR_PANEL::selectedItems() const
{
    std::vector<const LIST_ITEM*> items;

    if( !m_netsList->selectionModel() )
        return items;

    for( const QModelIndex& index : m_netsList->selectionModel()->selectedRows() )
    {
        if( const LIST_ITEM* item = itemOfViewIndex( index ) )
            items.push_back( item );
    }

    return items;
}


void PCB_NET_INSPECTOR_PANEL::selectItems( const std::vector<const LIST_ITEM*>& aItems ) const
{
    QItemSelectionModel* selectionModel = m_netsList->selectionModel();

    if( !selectionModel )
        return;

    QItemSelection selection;

    for( const LIST_ITEM* item : aItems )
    {
        QModelIndex index = viewIndexOf( item );

        if( index.isValid() )
            selection.select( index, index );
    }

    selectionModel->select( selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows );

    if( !selection.isEmpty() )
        m_netsList->scrollTo( selection.indexes().first() );
}


void PCB_NET_INSPECTOR_PANEL::saveViewSettings( PANEL_NET_INSPECTOR_SETTINGS& cfg ) const
{
    QHeaderView* header = m_netsList->header();

    // Grid sorting
    cfg.sorting_column = header->isSortIndicatorShown() ? header->sortIndicatorSection() : -1;
    cfg.sort_order_asc = header->sortIndicatorOrder() == Qt::AscendingOrder;

    // Column arrangement / sizes, in the order of the columns shown
    const int count = header->count();

    cfg.col_order.resize( count );
    cfg.col_widths.resize( count );
    cfg.col_hidden.resize( count );

    for( int visual = 0; visual < count; ++visual )
    {
        int logical = header->logicalIndex( visual );

        cfg.col_order[visual] = logical;
        cfg.col_widths[visual] = header->sectionSize( logical );
        cfg.col_hidden[visual] = header->isSectionHidden( logical );
    }

    // Expanded rows
    cfg.expanded_rows.clear();

    for( const auto& [groupName, groupItem] : m_dataModel->getGroupItems() )
    {
        if( m_netsList->isExpanded( viewIndexOf( groupItem ) ) )
            cfg.expanded_rows.push_back( groupName );
    }
}


bool PCB_NET_INSPECTOR_PANEL::askText( const wxString& aTitle, const wxString& aPrompt, wxString& aValue,
                                       const wxString& aHelp )
{
    wxString prompt = aPrompt;

    if( !aHelp.IsEmpty() )
        prompt << wxT( "\n" ) << aHelp;

    bool    ok = false;
    QString text = QInputDialog::getText( this, ToQString( aTitle ), ToQString( prompt ),
                                          QLineEdit::Normal, ToQString( aValue ), &ok );

    if( !ok )
        return false;

    // The names of the nets can't have line breaks (NETNAME_VALIDATOR of the wxWidgets build)
    text.remove( QLatin1Char( '\r' ) );
    text.remove( QLatin1Char( '\n' ) );

    aValue = ToWxString( text );
    return true;
}


wxString PCB_NET_INSPECTOR_PANEL::chooseReportFile()
{
    QString path = QFileDialog::getSaveFileName( this, ToQString( _( "Save Net Inspector Report File" ) ),
                                                 QString(),
                                                 KIQT::FileDialogFilter( _( "Report file" )
                                                                         + AddFileExtListToFilter( { "csv" } ) ) );

    return ToWxString( path );
}
