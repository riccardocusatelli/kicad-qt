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
 * @file pcb_net_inspector_panel_wx.cpp
 * The view of the Net Inspector with wxWidgets: its wxDataViewCtrl, the columns, the menus and
 * the dialogs (the Qt build has its own version).  What the panel does is in
 * pcb_net_inspector_panel.cpp.
 */

#include <widgets/pcb_net_inspector_panel.h>
#include <widgets/pcb_net_inspector_panel_data_model.h>

#include <class_draw_panel_gal.h>
#include <dialogs/dialog_text_entry.h>
#include <pcb_edit_frame.h>
#include <pcb_painter.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <validators.h>
#include <view/view.h>
#include <widgets/ui_common.h>
#include <wildcards_and_files_ext.h>

#include <wx/filedlg.h>
#include <wx/menu.h>
#include <wx/stattext.h>
#include <wx/wupdlock.h>
#include <kiplatform/ui.h>

#include <core/ki_assert.h>


PCB_NET_INSPECTOR_PANEL::PCB_NET_INSPECTOR_PANEL( wxWindow* parent, PCB_EDIT_FRAME* aFrame ) :
        NET_INSPECTOR_PANEL( parent, aFrame ),
        m_frame( aFrame ),
        m_dataModel( new DATA_MODEL( *this ) )
{
    m_board = m_frame->GetBoard();

    m_netsList->AssociateModel( &*m_dataModel );

    // Rebuild nets list
    buildNetsList( true );

    // Register the panel to receive board change notifications
    if( m_board != nullptr )
    {
        PCB_NET_INSPECTOR_PANEL::OnBoardHighlightNetChanged( *m_board );
        m_board->AddListener( this );
    }

    // Connect to board events
    m_frame->Bind( EDA_EVT_UNITS_CHANGED, &PCB_NET_INSPECTOR_PANEL::onUnitsChanged, this );

    // Connect to wxDataViewCtrl events
    m_netsList->Bind( wxEVT_DATAVIEW_ITEM_EXPANDED, &PCB_NET_INSPECTOR_PANEL::OnExpandCollapseRow, this );
    m_netsList->Bind( wxEVT_DATAVIEW_ITEM_COLLAPSED, &PCB_NET_INSPECTOR_PANEL::OnExpandCollapseRow, this );
    m_netsList->Bind( wxEVT_DATAVIEW_COLUMN_HEADER_RIGHT_CLICK, &PCB_NET_INSPECTOR_PANEL::OnHeaderContextMenu, this );
    m_netsList->Bind( wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, &PCB_NET_INSPECTOR_PANEL::OnNetsListContextMenu, this );
    m_netsList->Bind( wxEVT_DATAVIEW_ITEM_ACTIVATED, &PCB_NET_INSPECTOR_PANEL::OnNetsListItemActivated, this );
    m_netsList->Bind( wxEVT_DATAVIEW_COLUMN_SORTED, &PCB_NET_INSPECTOR_PANEL::OnColumnSorted, this );
}

PCB_NET_INSPECTOR_PANEL::~PCB_NET_INSPECTOR_PANEL()
{
    PCB_NET_INSPECTOR_PANEL::SaveSettings();

    m_netsList->AssociateModel( nullptr );

    // Disconnect from board events
    m_frame->Unbind( EDA_EVT_UNITS_CHANGED, &PCB_NET_INSPECTOR_PANEL::onUnitsChanged, this );

    // Connect to wxDataViewCtrl events
    m_netsList->Unbind( wxEVT_DATAVIEW_ITEM_EXPANDED, &PCB_NET_INSPECTOR_PANEL::OnExpandCollapseRow, this );
    m_netsList->Unbind( wxEVT_DATAVIEW_ITEM_COLLAPSED, &PCB_NET_INSPECTOR_PANEL::OnExpandCollapseRow, this );
    m_netsList->Unbind( wxEVT_DATAVIEW_COLUMN_HEADER_RIGHT_CLICK, &PCB_NET_INSPECTOR_PANEL::OnHeaderContextMenu, this );
    m_netsList->Unbind( wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, &PCB_NET_INSPECTOR_PANEL::OnNetsListContextMenu, this );
    m_netsList->Unbind( wxEVT_DATAVIEW_ITEM_ACTIVATED, &PCB_NET_INSPECTOR_PANEL::OnNetsListItemActivated, this );
    m_netsList->Unbind( wxEVT_DATAVIEW_COLUMN_SORTED, &PCB_NET_INSPECTOR_PANEL::OnColumnSorted, this );
}

/*****************************************************************************************
 *
 * Grid / model columns configuration
 *
 * ***************************************************************************************/


void PCB_NET_INSPECTOR_PANEL::viewBuildColumns( PANEL_NET_INSPECTOR_SETTINGS* cfg )
{
    auto appendColumn =
            [&]( int aModelColumn )
            {
                int flags = wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE;

                if( aModelColumn != COLUMN_NAME )
                    flags |= wxDATAVIEW_COL_REORDERABLE;

                wxAlignment align = aModelColumn == COLUMN_NAME || aModelColumn == COLUMN_NETCLASS
                                            ? wxALIGN_LEFT
                                            : wxALIGN_CENTER;

                m_netsList->AppendTextColumn( m_columns[aModelColumn].display_name, m_columns[aModelColumn],
                                              wxDATAVIEW_CELL_INERT, -1, align, flags );
            };

    // Add display columns in settings order
    for( const int i : cfg->col_order )
        appendColumn( i );

    // Set the name column as the expander row
    if( wxDataViewColumn* col = getDisplayedColumnForModelField( COLUMN_NAME ) )
        m_netsList->SetExpanderColumn( col );

    adjustListColumnSizes( cfg );
}


int PCB_NET_INSPECTOR_PANEL::getMinColumnWidth( int aModelColumn ) const
{
    constexpr int margins = 15;
    constexpr int extra_width = 30;

    int headerWidth = GetTextExtent( m_columns[aModelColumn].display_name ).x;
    headerWidth += ( aModelColumn == COLUMN_NAME ) ? extra_width : margins;

    return headerWidth;
}


void PCB_NET_INSPECTOR_PANEL::autosizeColumn( wxDataViewColumn* aCol )
{
    if( !aCol || aCol->IsHidden() )
        return;

    const unsigned int modelCol = aCol->GetModelColumn();

    if( modelCol >= m_columns.size() )
        return;

    constexpr int margins = 15;
    constexpr int extra_width = 30;
    const bool    isNameCol = ( modelCol == COLUMN_NAME );
    const int     padding = isNameCol ? extra_width : margins;
    const int     indent = m_netsList->GetIndent();

    int maxWidth = getMinColumnWidth( modelCol );

    for( unsigned int row = 0; row < m_dataModel->itemCount(); ++row )
    {
        wxVariant value = m_dataModel->valueAt( modelCol, row );
        int       textWidth = GetTextExtent( value.GetString() ).x + padding;
        maxWidth = std::max( maxWidth, textWidth );

        const LIST_ITEM& item = m_dataModel->itemAt( row );

        if( item.GetIsGroup() )
        {
            for( auto it = item.ChildrenBegin(); it != item.ChildrenEnd(); ++it )
            {
                wxVariant childValue = m_dataModel->valueForItem( *it, modelCol );
                int       childWidth = GetTextExtent( childValue.GetString() ).x + padding;

                if( isNameCol )
                    childWidth += indent;

                maxWidth = std::max( maxWidth, childWidth );
            }
        }
    }

    aCol->SetWidth( maxWidth );
}


void PCB_NET_INSPECTOR_PANEL::adjustListColumnSizes( PANEL_NET_INSPECTOR_SETTINGS* cfg ) const
{
    wxWindowUpdateLocker locker( m_netsList );

    if( cfg->col_widths.size() != m_columns.size() )
    {
        int minValueWidth = GetTextExtent( wxT( "00000,000 mm" ) ).x;
        int minNumberWidth = GetTextExtent( wxT( "000" ) ).x;
        int minNameWidth = GetTextExtent( wxT( "MMMMMMMMMMMM" ) ).x;

        // Considering left and right margins.
        // For wxRenderGeneric it is 5px.
        // Also account for the sorting arrow in the column header.
        // Column 0 also needs space for any potential expander icons.
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

        KI_ASSERT( m_columns.size() == cfg->col_order.size() );

        for( size_t i = 0; i < m_columns.size(); ++i )
        {
            const int modelColumn = cfg->col_order[i];
            int       titleSize = GetTextExtent( m_columns[modelColumn].display_name ).x;
            titleSize = modelColumn == COLUMN_NAME ? titleSize + extra_width : titleSize + margins;
            const int valSize = getTargetWidth( modelColumn );
            m_netsList->GetColumn( i )->SetWidth( std::max( titleSize, valSize ) );
        }
    }
    else
    {
        KI_ASSERT( m_columns.size() == cfg->col_hidden.size() );
        KI_ASSERT( m_columns.size() == cfg->col_widths.size() );

        for( size_t ii = 0; ii < m_columns.size(); ++ii )
        {
            const int modelCol = static_cast<int>( m_netsList->GetColumn( ii )->GetModelColumn() );
            const int minWidth = getMinColumnWidth( modelCol );
            const int newWidth = cfg->col_widths[ii];

            m_netsList->GetColumn( ii )->SetWidth( std::max( newWidth, minWidth ) );
            m_netsList->GetColumn( ii )->SetHidden( cfg->col_hidden[ii] );
        }
    }

    m_netsList->Refresh();
}


bool PCB_NET_INSPECTOR_PANEL::restoreSortColumn( const int sortingColumnId, const bool sortOrderAsc ) const
{
    if( sortingColumnId != -1 )
    {
        if( wxDataViewColumn* col = getDisplayedColumnForModelField( sortingColumnId ) )
        {
            col->SetSortOrder( sortOrderAsc );
            m_dataModel->Resort();
            return true;
        }
    }

    return false;
}


wxDataViewColumn* PCB_NET_INSPECTOR_PANEL::getDisplayedColumnForModelField( const int columnId ) const
{
    for( unsigned int i = 0; i < m_netsList->GetColumnCount(); ++i )
    {
        wxDataViewColumn* col = m_netsList->GetColumn( i );

        if( static_cast<int>( col->GetModelColumn() ) == columnId )
            return col;
    }

    return nullptr;
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

    m_netsList->Freeze();

    m_dataModel->SetIsTimeDomain( m_showTimeDomainDetails );

    PROJECT_LOCAL_SETTINGS& localSettings = Pgm().GetSettingsManager().Prj().GetLocalSettings();
    PANEL_NET_INSPECTOR_SETTINGS* cfg = &localSettings.m_NetInspectorPanel;

    // Refresh all filtering / grouping settings
    readFilterSettings( cfg );

    // Attempt to keep any expanded groups open
    if( m_boardLoaded && !m_boardLoading )
    {
        cfg->expanded_rows.clear();
        DATA_MODEL* model = static_cast<DATA_MODEL*>( m_netsList->GetModel() );

        for( const auto& [groupName, groupItem] : model->getGroupDataViewItems() )
        {
            if( m_netsList->IsExpanded( groupItem ) )
                cfg->expanded_rows.push_back( groupName );
        }
    }

    // When rebuilding the netlist, try to keep the row selection
    wxDataViewItemArray sel;
    m_netsList->GetSelections( sel );

    std::vector<int> prev_selected_netcodes;
    prev_selected_netcodes.reserve( sel.GetCount() );

    for( unsigned int i = 0; i < sel.GetCount(); ++i )
    {
        const LIST_ITEM* item = static_cast<const LIST_ITEM*>( sel.Item( i ).GetID() );
        prev_selected_netcodes.push_back( item->GetNetCode() );
    }

    int  sorting_column_id = cfg->sorting_column;
    bool sort_order_asc = cfg->sort_order_asc;

    if( wxDataViewColumn* sorting_column = m_netsList->GetSortingColumn() )
    {
        if( !m_boardLoading )
        {
            sorting_column_id = static_cast<int>( sorting_column->GetModelColumn() );
            sort_order_asc = sorting_column->IsSortOrderAscending();
        }

        // On GTK, wxDVC will crash if we rebuild with a sorting column set.
        sorting_column->UnsetAsSortKey();
    }

    if( rebuildColumns )
    {
        m_netsList->ClearColumns();
        buildColumns();
    }

    fillDataModel( cfg );

    // Try to re-enable the sorting column
    if( !restoreSortColumn( sorting_column_id, sort_order_asc ))
    {
        // By default, sort by Name column
        restoreSortColumn( COLUMN_NAME, true );
    }

    // Try to restore the expanded groups
    if( m_boardLoaded )
    {
        m_rowExpanding = true;

        std::vector<std::pair<wxString, wxDataViewItem>> groupItems = m_dataModel->getGroupDataViewItems();

        for( wxString& groupName : cfg->expanded_rows )
        {
            auto pred =
                    [&groupName]( const std::pair<wxString, wxDataViewItem>& item )
                    {
                        return groupName == item.first;
                    };

            auto tableItem = std::ranges::find_if( groupItems, pred );

            if( tableItem != groupItems.end() )
                m_netsList->Expand( tableItem->second );
        }

        m_rowExpanding = false;
    }

    // Try to restore the selected rows
    sel.Clear();

    for( const int& nc : prev_selected_netcodes )
    {
        if( std::optional<LIST_ITEM_ITER> r = m_dataModel->findItem( nc ) )
        {
            const std::unique_ptr<LIST_ITEM>& list_item = *r.value();
            sel.Add( wxDataViewItem( list_item.get() ) );
        }
    }

    m_netsList->Thaw(); // Must thaw before reselecting to avoid windows selection bug

    if( !sel.IsEmpty() )
    {
        m_netsList->SetSelections( sel );
        m_netsList->EnsureVisible( sel.Item( 0 ) );
    }
    else
    {
        m_netsList->UnselectAll();
    }

    m_inBuildNetsList = false;
}


void PCB_NET_INSPECTOR_PANEL::updateDisplayedRowValues( const std::optional<LIST_ITEM_ITER>& aRow ) const
{
    if( !aRow )
        return;

    wxDataViewItemArray sel;
    m_netsList->GetSelections( sel );

    m_dataModel->updateItem( aRow );

    if( !sel.IsEmpty() )
    {
        m_netsList->SetSelections( sel );
        m_netsList->EnsureVisible( sel.Item( 0 ) );
    }
}


void PCB_NET_INSPECTOR_PANEL::OnNetsListContextMenu( wxDataViewEvent& event )
{
    bool             multipleSelections = false;
    const LIST_ITEM* selItem = nullptr;

    if( m_netsList->GetSelectedItemsCount() == 1 )
    {
        selItem = static_cast<const LIST_ITEM*>( m_netsList->GetSelection().GetID() );
    }
    else
    {
        if( m_netsList->GetSelectedItemsCount() > 1 )
            multipleSelections = true;
    }

    wxMenu menu;

    // Net edit menu items
    wxMenuItem* highlightNet = new wxMenuItem( &menu, ID_HIGHLIGHT_SELECTED_NETS, _( "Highlight Selected Net" ),
                                               wxEmptyString, wxITEM_NORMAL );
    menu.Append( highlightNet );

    wxMenuItem* clearHighlighting = new wxMenuItem( &menu, ID_CLEAR_HIGHLIGHTING, _( "Clear Net Highlighting" ),
                                                    wxEmptyString, wxITEM_NORMAL );
    menu.Append( clearHighlighting );

    RENDER_SETTINGS* renderSettings = m_frame->GetCanvas()->GetView()->GetPainter()->GetSettings();
    const std::set<int>& selected_codes = renderSettings->GetHighlightNetCodes();

    if( selected_codes.size() == 0 )
        clearHighlighting->Enable( false );

    menu.AppendSeparator();

    wxMenuItem* renameNet = new wxMenuItem( &menu, ID_RENAME_NET, _( "Rename Selected Net..." ), wxEmptyString,
                                            wxITEM_NORMAL );
    menu.Append( renameNet );

    wxMenuItem* deleteNet = new wxMenuItem( &menu, ID_DELETE_NET, _( "Delete Selected Net" ), wxEmptyString,
                                            wxITEM_NORMAL );
    menu.Append( deleteNet );

    menu.AppendSeparator();

    wxMenuItem* addNet = new wxMenuItem( &menu, ID_ADD_NET, _( "Add Net..." ), wxEmptyString, wxITEM_NORMAL );
    menu.Append( addNet );

    if( !selItem && !multipleSelections )
    {
        highlightNet->Enable( false );
        deleteNet->Enable( false );
        renameNet->Enable( false );
    }
    else
    {
        if( multipleSelections || selItem->GetIsGroup() )
        {
            highlightNet->SetItemLabel( _( "Highlight Selected Nets" ) );
            renameNet->Enable( false );
            deleteNet->SetItemLabel( _( "Delete Selected Nets" ) );
        }
    }

    menu.AppendSeparator();

    wxMenuItem* removeSelectedGroup = new wxMenuItem( &menu, ID_REMOVE_SELECTED_GROUP,
                                                      _( "Remove Selected Custom Group" ),
                                                      wxEmptyString, wxITEM_NORMAL );
    menu.Append( removeSelectedGroup );

    if( !selItem || !selItem->GetIsGroup() )
        removeSelectedGroup->Enable( false );

    menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &PCB_NET_INSPECTOR_PANEL::onContextMenuSelection, this );

    PopupMenu( &menu );
}


void PCB_NET_INSPECTOR_PANEL::OnSearchTextChanged( wxCommandEvent& event )
{
    SaveSettings();
    buildNetsList();
}


void PCB_NET_INSPECTOR_PANEL::OnExpandCollapseRow( wxCommandEvent& event )
{
    if( !m_rowExpanding )
        SaveSettings();
}


void PCB_NET_INSPECTOR_PANEL::OnHeaderContextMenu( wxDataViewEvent& event )
{
    m_contextMenuColumn = event.GetDataViewColumn();

    wxMenu menu;

    menu.Append( ID_AUTOFIT_COLUMN, _( "Auto-fit Column" ) );
    menu.Append( ID_AUTOFIT_ALL_COLUMNS, _( "Auto-fit All Columns" ) );
    menu.AppendSeparator();

    generateShowHideColumnMenu( &menu );
    menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &PCB_NET_INSPECTOR_PANEL::onContextMenuSelection, this );
    PopupMenu( &menu );
}


void PCB_NET_INSPECTOR_PANEL::OnConfigButton( wxCommandEvent& event )
{
    PROJECT_LOCAL_SETTINGS& localSettings = Pgm().GetSettingsManager().Prj().GetLocalSettings();
    auto&                   cfg = localSettings.m_NetInspectorPanel;

    const LIST_ITEM* selItem = nullptr;

    if( m_netsList->GetSelectedItemsCount() == 1 )
    {
        selItem = static_cast<const LIST_ITEM*>( m_netsList->GetSelection().GetID() );
    }

    wxMenu menu;

    // Filtering menu items
    wxMenuItem* filterByNetName = new wxMenuItem( &menu, ID_FILTER_BY_NET_NAME, _( "Filter by Net Name" ),
                                                  wxEmptyString, wxITEM_CHECK );
    menu.Append( filterByNetName );
    filterByNetName->Check( cfg.filter_by_net_name );

    wxMenuItem* filterByNetclass = new wxMenuItem( &menu, ID_FILTER_BY_NETCLASS, _( "Filter by Netclass" ),
                                                   wxEmptyString, wxITEM_CHECK );
    menu.Append( filterByNetclass );
    filterByNetclass->Check( cfg.filter_by_netclass );

    menu.AppendSeparator();

    // Grouping menu items
    //wxMenuItem* groupConstraint =
    //        new wxMenuItem( &menu, ID_GROUP_BY_CONSTRAINT, _( "Group by DRC Constraint" ),
    //                        wxEmptyString, wxITEM_CHECK );
    //groupConstraint->Check( m_group_by_constraint );
    //menu.Append( groupConstraint );

    wxMenuItem* groupNetclass = new wxMenuItem( &menu, ID_GROUP_BY_NETCLASS, _( "Group by Netclass" ),
                                                wxEmptyString, wxITEM_CHECK );
    menu.Append( groupNetclass );
    groupNetclass->Check( m_groupByNetclass );

    menu.AppendSeparator();

    wxMenuItem* addGroup = new wxMenuItem( &menu, ID_ADD_GROUP, _( "Add Custom Group..." ),
                                           wxEmptyString, wxITEM_NORMAL );
    menu.Append( addGroup );

    wxMenuItem* removeSelectedGroup = new wxMenuItem( &menu, ID_REMOVE_SELECTED_GROUP,
                                                      _( "Remove Selected Custom Group" ),
                                                      wxEmptyString, wxITEM_NORMAL );
    menu.Append( removeSelectedGroup );

    if( !selItem || !selItem->GetIsGroup() )
        removeSelectedGroup->Enable( false );

    wxMenuItem* removeCustomGroups = new wxMenuItem( &menu, ID_REMOVE_GROUPS, _( "Remove All Custom Groups" ),
                                                     wxEmptyString, wxITEM_NORMAL );
    menu.Append( removeCustomGroups );
    removeCustomGroups->Enable( m_custom_group_rules.size() != 0 );

    menu.AppendSeparator();

    wxMenuItem* showZeroNetPads = new wxMenuItem( &menu, ID_SHOW_ZERO_NET_PADS, _( "Show Zero Pad Nets" ),
                                                  wxEmptyString, wxITEM_CHECK );
    menu.Append( showZeroNetPads );
    showZeroNetPads->Check( m_showZeroPadNets );

    wxMenuItem* showUnconnectedNets = new wxMenuItem( &menu, ID_SHOW_UNCONNECTED_NETS, _( "Show Unconnected Nets" ),
                                                      wxEmptyString, wxITEM_CHECK );
    menu.Append( showUnconnectedNets );
    showUnconnectedNets->Check( m_showUnconnectedNets );

    menu.AppendSeparator();

    wxMenuItem* showTimeDomainDetails = new wxMenuItem( &menu, ID_SHOW_TIME_DOMAIN_DETAILS,
                                                        _( "Show Time Domain Details" ),
                                                        wxEmptyString, wxITEM_CHECK );
    menu.Append( showTimeDomainDetails );
    showTimeDomainDetails->Check( m_showTimeDomainDetails );

    menu.AppendSeparator();

    // Report generation
    wxMenuItem* generateReport = new wxMenuItem( &menu, ID_GENERATE_REPORT, _( "Save Net Inspector Report..." ),
                                                 wxEmptyString, wxITEM_NORMAL );
    menu.Append( generateReport );

    menu.AppendSeparator();

    // Show / hide columns menu items
    wxMenu* colsMenu = new wxMenu();
    generateShowHideColumnMenu( colsMenu );
    menu.AppendSubMenu( colsMenu, _( "Show / Hide Columns" ) );

    menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &PCB_NET_INSPECTOR_PANEL::onContextMenuSelection, this );

    PopupMenu( &menu );
}


void PCB_NET_INSPECTOR_PANEL::generateShowHideColumnMenu( wxMenu* target )
{
    for( int i = 1; i <= COLUMN_LAST_STATIC_COL; ++i )
    {
        wxMenuItem* opt = new wxMenuItem( target, ID_HIDE_COLUMN + i, m_columns[i].display_name,
                                          wxEmptyString, wxITEM_CHECK );
        wxDataViewColumn* col = getDisplayedColumnForModelField( i );
        target->Append( opt );
        opt->Check( !col->IsHidden() );
    }

    target->AppendSeparator();

    for( std::size_t i = COLUMN_LAST_STATIC_COL + 1; i < m_columns.size(); ++i )
    {
        wxMenuItem* opt = new wxMenuItem( target, ID_HIDE_COLUMN + i, m_columns[i].display_name,
                                          wxEmptyString, wxITEM_CHECK );
        wxDataViewColumn* col = getDisplayedColumnForModelField( i );
        target->Append( opt );
        opt->Check( !col->IsHidden() );
    }
}


void PCB_NET_INSPECTOR_PANEL::OnNetsListItemActivated( wxDataViewEvent& event )
{
    highlightSelectedNets();
}


void PCB_NET_INSPECTOR_PANEL::OnColumnSorted( wxDataViewEvent& event )
{
    if( !m_inBuildNetsList )
        SaveSettings();
}


void PCB_NET_INSPECTOR_PANEL::onUnitsChanged( wxCommandEvent& event )
{
    m_dataModel->updateAllItems();
    event.Skip();
}


void PCB_NET_INSPECTOR_PANEL::onContextMenuSelection( wxCommandEvent& event )
{
    doContextMenuCommand( event.GetId() );
}


void PCB_NET_INSPECTOR_PANEL::viewColumnCommand( int aId )
{
    if( aId == ID_AUTOFIT_COLUMN )
    {
        if( m_contextMenuColumn )
            autosizeColumn( m_contextMenuColumn );
    }
    else if( aId == ID_AUTOFIT_ALL_COLUMNS )
    {
        for( unsigned int i = 0; i < m_netsList->GetColumnCount(); ++i )
            autosizeColumn( m_netsList->GetColumn( i ) );
    }
    else if( aId >= ID_HIDE_COLUMN )
    {
        const int columnId = aId - ID_HIDE_COLUMN;

        if( wxDataViewColumn* col = getDisplayedColumnForModelField( columnId ) )
        {
            // Make sure we end up with something non-zero so we can resize it
            col->SetWidth( std::max( col->GetWidth(), 10 ) );
            col->SetHidden( !col->IsHidden() );
        }
    }
}


/*****************************************************************************************
 *
 * The view of the GUI toolkit
 *
 * ***************************************************************************************/


wxString PCB_NET_INSPECTOR_PANEL::searchText() const
{
    return m_searchCtrl->GetValue();
}


void PCB_NET_INSPECTOR_PANEL::setSearchText( const wxString& aText )
{
    m_searchCtrl->SetValue( aText );
}


bool PCB_NET_INSPECTOR_PANEL::isPanelShown() const
{
    return IsShownOnScreen();
}


bool PCB_NET_INSPECTOR_PANEL::isViewDisplayed() const
{
    for( unsigned int ii = 0; ii < m_dataModel->columnCount(); ++ii )
    {
        if( m_netsList->GetColumn( ii )->GetWidth() > 0 )
            return true;
    }

    return false;
}


void PCB_NET_INSPECTOR_PANEL::refreshList() const
{
    m_netsList->Refresh();
}


void PCB_NET_INSPECTOR_PANEL::freezeList() const
{
    m_netsList->Freeze();
}


void PCB_NET_INSPECTOR_PANEL::thawList() const
{
    m_netsList->Thaw();
}


std::vector<const PCB_NET_INSPECTOR_PANEL::LIST_ITEM*> PCB_NET_INSPECTOR_PANEL::selectedItems() const
{
    std::vector<const LIST_ITEM*> items;
    wxDataViewItemArray           sel;

    m_netsList->GetSelections( sel );
    items.reserve( sel.GetCount() );

    for( unsigned int i = 0; i < sel.GetCount(); ++i )
        items.push_back( static_cast<const LIST_ITEM*>( sel.Item( i ).GetID() ) );

    return items;
}


void PCB_NET_INSPECTOR_PANEL::selectItems( const std::vector<const LIST_ITEM*>& aItems ) const
{
    if( aItems.empty() )
    {
        m_netsList->UnselectAll();
        return;
    }

    wxDataViewItemArray sel;
    sel.Alloc( aItems.size() );

    for( const LIST_ITEM* item : aItems )
        sel.Add( wxDataViewItem( const_cast<LIST_ITEM*>( item ) ) );

    m_netsList->SetSelections( sel );
    m_netsList->EnsureVisible( sel.Item( 0 ) );
}


void PCB_NET_INSPECTOR_PANEL::saveViewSettings( PANEL_NET_INSPECTOR_SETTINGS& cfg ) const
{
    // Grid sorting
    wxDataViewColumn* sortingCol = m_netsList->GetSortingColumn();
    cfg.sorting_column = sortingCol ? static_cast<int>( sortingCol->GetModelColumn() ) : -1;
    cfg.sort_order_asc = sortingCol ? sortingCol->IsSortOrderAscending() : true;

    // Column arrangement / sizes
    cfg.col_order.resize( m_dataModel->columnCount() );
    cfg.col_widths.resize( m_dataModel->columnCount() );
    cfg.col_hidden.resize( m_dataModel->columnCount() );

    for( unsigned int ii = 0; ii < m_dataModel->columnCount(); ++ii )
    {
        cfg.col_order[ii] = (int) m_netsList->GetColumn( ii )->GetModelColumn();
        cfg.col_widths[ii] = m_netsList->GetColumn( ii )->GetWidth();
        cfg.col_hidden[ii] = m_netsList->GetColumn( ii )->IsHidden();
    }

    // Expanded rows
    cfg.expanded_rows.clear();
    std::vector<std::pair<wxString, wxDataViewItem>> groupItems = m_dataModel->getGroupDataViewItems();

    for( std::pair<wxString, wxDataViewItem>& item : groupItems )
    {
        if( m_netsList->IsExpanded( item.second ) )
            cfg.expanded_rows.push_back( item.first );
    }
}


bool PCB_NET_INSPECTOR_PANEL::askText( const wxString& aTitle, const wxString& aPrompt, wxString& aValue,
                                       const wxString& aHelp )
{
    wxString          value = aValue;
    NETNAME_VALIDATOR validator( &value );

    WX_TEXT_ENTRY_DIALOG dlg( this, aPrompt, aTitle, aValue );

    if( !aHelp.IsEmpty() )
    {
        wxStaticText* help = new wxStaticText( &dlg, wxID_ANY, aHelp );
        help->SetFont( KIUI::GetInfoFont( this ).Italic() );
        dlg.m_ContentSizer->Add( help, 0, wxALL|wxEXPAND, 5 );
    }

    dlg.SetTextValidator( validator );
    dlg.GetSizer()->SetSizeHints( &dlg );

    if( dlg.ShowModal() != wxID_OK )
        return false;

    aValue = dlg.GetValue();
    return true;
}


wxString PCB_NET_INSPECTOR_PANEL::chooseReportFile()
{
    wxFileDialog dlg( this, _( "Save Net Inspector Report File" ), "", "",
                      _( "Report file" ) + AddFileExtListToFilter( { "csv" } ),
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    KIPLATFORM::UI::AllowNetworkFileSystems( &dlg );

    if( dlg.ShowModal() == wxID_CANCEL )
        return wxEmptyString;

    return dlg.GetPath();
}
