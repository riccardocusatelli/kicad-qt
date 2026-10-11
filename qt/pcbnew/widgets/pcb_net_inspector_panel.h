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
 * @file pcb_net_inspector_panel.h
 * PCB_NET_INSPECTOR_PANEL of the Qt build: its list of nets is a QTreeView on the DATA_MODEL of
 * qt/pcbnew/widgets/pcb_net_inspector_panel_data_model.h.  What the panel does is shared with the
 * wxWidgets build (pcbnew/widgets/pcb_net_inspector_panel.cpp); the view is in
 * pcb_net_inspector_panel_qt.cpp.
 */

#pragma once

#include <QModelIndex>
#include <QPoint>

#include <board.h>
#include <id.h>
#include <project/project_local_settings.h>
#include <widgets/net_inspector_panel.h>

#include <optional>
#include <vector>

class PCB_EDIT_FRAME;
class NETINFO_ITEM;
class BOARD;
class BOARD_ITEM;
class CN_ITEM;
class PCB_TRACK;
class EDA_COMBINED_MATCHER;
class QMenu;

/**
 * PCB net inspection panel
 *
 * Provides a read-only view of net information, such as routed lengths. Data is updated after every change of board
 * items.
 */
class PCB_NET_INSPECTOR_PANEL : public NET_INSPECTOR_PANEL, public BOARD_LISTENER
{
public:
    PCB_NET_INSPECTOR_PANEL( QWidget* parent, PCB_EDIT_FRAME* aFrame );
    virtual ~PCB_NET_INSPECTOR_PANEL();

    /**
     * Updates the netlist based on global board changes (e.g. stackup definition)
     *
     * Called by PCB_EDIT_FRAME after displaying the Board Setup dialog
     */
    void OnParentSetupChanged() override;

    /*
     * BOARD_LISTENER implementation
     */
    void OnBoardItemAdded( BOARD& aBoard, BOARD_ITEM* aBoardItem ) override;
    void OnBoardItemsAdded( BOARD& aBoard, std::vector<BOARD_ITEM*>& aBoardItems ) override;
    void OnBoardItemRemoved( BOARD& aBoard, BOARD_ITEM* aBoardItem ) override;
    void OnBoardItemsRemoved( BOARD& aBoard, std::vector<BOARD_ITEM*>& aBoardItems ) override;
    void OnBoardNetSettingsChanged( BOARD& aBoard ) override;
    void OnBoardItemChanged( BOARD& aBoard, BOARD_ITEM* aBoardItem ) override;
    void OnBoardItemsChanged( BOARD& aBoard, std::vector<BOARD_ITEM*>& aBoardItems ) override;
    void OnBoardHighlightNetChanged( BOARD& aBoard ) override;
    void OnBoardCompositeUpdate( BOARD& aBoard, std::vector<BOARD_ITEM*>& aAddedItems,
                                 std::vector<BOARD_ITEM*>& aRemovedItems,
                                 std::vector<BOARD_ITEM*>& aChangedItems ) override;

    /// Update panel when board is changed
    void OnBoardChanged() override;

    /// Prepare the panel when shown in the editor
    void OnShowPanel() override;

    /// Persist the net inspector configuration to project / global settings
    void SaveSettings() override;

protected:
    /// Reloads strings on an application language change
    void OnLanguageChangedImpl() override;

    /*
     * UI events
     */
    void OnSearchTextChanged() override;
    void OnConfigButton() override;
    void OnExpandCollapseRow();
    void OnHeaderContextMenu( const QPoint& aPos );
    void OnNetsListContextMenu( const QPoint& aPos );
    void OnNetsListItemActivated();
    void OnColumnSorted();

private:
    /// Updates displayed statistics for the given nets
    void updateNets( const std::vector<NETINFO_ITEM*>& aNets ) const;

    /// Unified handling of added / deleted / modified board items
    void updateBoardItems( const std::vector<BOARD_ITEM*>& aBoardItems );

    /*
     * Helper methods for returning formatted data
     */
    static wxString formatNetCode( const NETINFO_ITEM* aNet );
    static wxString formatNetName( const NETINFO_ITEM* aNet );
    static wxString formatCount( unsigned int aValue );
    wxString formatLength( int64_t aValue ) const;
    wxString formatDelay( int64_t aValue ) const;

    /// Generates a sub-menu for the show / hide columns submenu
    void generateShowHideColumnMenu( QMenu* target );

    /// Fetches an ordered (by NetCode) list of all board connectivity items
    std::vector<CN_ITEM*> relevantConnectivityItems() const;

    /// Filter to determine whether a board net should be included in the net inspector
    bool netFilterMatches( NETINFO_ITEM* aNet, PANEL_NET_INSPECTOR_SETTINGS* cfg = nullptr ) const;

    /// Rebuilds the net inspector list, removing all previous entries
    void buildNetsList( bool rebuildColumns = false );

    /// Build the required columns in the net inspector grid
    void buildColumns();

    /**
     * Adjust the sizing of list columns
     *
     * @param cfg the PANEL_NET_INSPECTOR_SETTINGS from which to read column widths
    */
    void adjustListColumnSizes( PANEL_NET_INSPECTOR_SETTINGS* cfg ) const;

    /**
     * Computes the minimum display width for a column based on its header text
     *
     * @param aModelColumn The model column index
     * @returns The minimum width in pixels
    */
    int getMinColumnWidth( int aModelColumn ) const;

    /**
     * Auto-sizes a column to fit both its header text and content
     *
     * @param aModelColumn The column to auto-size
    */
    void autosizeColumn( int aModelColumn );

    /**
     * Sets the sort column in the grid to that showing the given model ID column
     *
     * @param sortingColumnId The model ID of the column to sort by
     * @param sortOrderAsc True for ascending sort, False for descending sort
     * @returns true if the column was found
    */
    bool restoreSortColumn( int sortingColumnId, bool sortOrderAsc ) const;


    /// Generates a CSV report from currently disaplyed data
    void generateReport();

    /// Highlight the currently selected net
    void highlightSelectedNets();

    /// Handle an application-level change of units
    void onUnitsChanged();

    /// Adds a new user-specified net to the board
    void onAddNet();

    /// Renames a selected net
    void onRenameSelectedNet();

    /// Deletes a selected net
    void onDeleteSelectedNet();

    /// Adds a custom display grouping of nets
    void onAddGroup();

    /// Removes a custom display grouping
    void onRemoveSelectedGroup();

    /// Clears highlighting from nets
    void onClearHighlighting();

    /// Forward declaration: container class for a set of net data
    class LIST_ITEM;

    /// The sorting of the view (with the groups first)
    class SORT_PROXY;

    /**
     * Calculates the length statistics for each given netcode
     *
     * @param aNetCodes is the list of netcodes to calculate statistics for. This must be sorted in ascending
     *                  netcode order
     * @param aIncludeZeroPadNets determines whether the results should include nets with no pads
     * @returns a map of net code to net length detail objects
    */
    std::vector<std::unique_ptr<LIST_ITEM>> calculateNets( const std::vector<NETINFO_ITEM*>& aNetCodes,
                                                           bool aIncludeZeroPadNets ) const;

    /// Ordered comparison of LIST_ITEMs by net code
    struct LIST_ITEM_NETCODE_CMP_LESS;

    /// Ordered comparison of LIST_ITEMs by group number
    struct LIST_ITEM_GROUP_NUMBER_CMP_LESS;

    using LIST_ITEM_ITER = std::vector<std::unique_ptr<LIST_ITEM>>::iterator;
    using LIST_ITEM_CONST_ITER = std::vector<std::unique_ptr<LIST_ITEM>>::const_iterator;

    /// Refreshes displayed data for the given rows
    void updateDisplayedRowValues( const std::optional<LIST_ITEM_ITER>& aRow ) const;

    // What the panel does, without the GUI toolkit (pcb_net_inspector_panel.cpp)

    /// The text of the column @a aCol of an entry (in the time domain or not).
    wxString itemText( const LIST_ITEM* aItem, unsigned int aCol, bool aTimeDomain ) const;

    /// The order of two entries by the column @a aCol: 0 if their values are the same.
    int compareItems( const LIST_ITEM& aItem1, const LIST_ITEM& aItem2, unsigned int aCol,
                      bool aAsc, bool aTimeDomain ) const;

    /// Read the filtering and grouping settings.
    void readFilterSettings( const PANEL_NET_INSPECTOR_SETTINGS* aCfg );

    /// Fill the data model with the nets of the board matching the filter.
    void fillDataModel( const PANEL_NET_INSPECTOR_SETTINGS* aCfg );

    /// The command @a aId of a context menu (ID_ADD_NET...).
    void doContextMenuCommand( int aId );

    // The view of the GUI toolkit (pcb_net_inspector_panel_qt.cpp)

    wxString searchText() const;
    void     setSearchText( const wxString& aText );

    /// The panel is shown on the screen (the board changes are ignored when it is hidden).
    bool isPanelShown() const;

    /// The view has been displayed (its settings can be saved).
    bool isViewDisplayed() const;

    void refreshList() const;
    void freezeList() const;
    void thawList() const;

    std::vector<const LIST_ITEM*> selectedItems() const;

    /// Select @a aItems (none: unselect all) and show the first one.
    void selectItems( const std::vector<const LIST_ITEM*>& aItems ) const;

    /// Create the columns of the view (for m_columns, in the order of the settings).
    void viewBuildColumns( PANEL_NET_INSPECTOR_SETTINGS* aCfg );

    /// Save the sorting, the columns and the expanded groups of the view.
    void saveViewSettings( PANEL_NET_INSPECTOR_SETTINGS& aCfg ) const;

    /// The commands of the context menus on the columns (ID_AUTOFIT_COLUMN, ID_HIDE_COLUMN...).
    void viewColumnCommand( int aId );

    /// Ask the user for a text (net or group name): false if cancelled.
    bool askText( const wxString& aTitle, const wxString& aPrompt, wxString& aValue,
                  const wxString& aHelp = wxEmptyString );

    /// The file of the report chosen by the user (empty if cancelled).
    wxString chooseReportFile();

    /// The index in the view of an entry (invalid for nullptr).
    QModelIndex viewIndexOf( const LIST_ITEM* aItem ) const;

    /// The entry of an index of the view.
    const LIST_ITEM* itemOfViewIndex( const QModelIndex& aIndex ) const;

    /// Show a menu of commands (doContextMenuCommand()) at the cursor.
    void execMenu( QMenu& aMenu );

    /// Parent BOARD
    BOARD*          m_board = nullptr;

    /// Owning edit frame
    PCB_EDIT_FRAME* m_frame = nullptr;

    /// Data model which holds LIST_ITEMs
    class DATA_MODEL;

    /// The bound data model to display
    DATA_MODEL* m_dataModel;
    friend DATA_MODEL;

    SORT_PROXY* m_proxy;
    friend SORT_PROXY;

    /// The listener of the changes of units of the frame
    int m_unitsListener = -1;

    /*
     * Status flags set during reporting and net rebuild operations
     */
    bool m_inReporting = false;
    bool m_inBuildNetsList = false;

    /*
     * Status flags to indicate whether a board has been loaded in this control's
     * lifetime. Required as on PCB_EDIT_FRAME construction, there are multiple events
     * triggered which would usually result in saving settings and re-loading the board.
     * However, before the board loads the frame is in an inconsistent state: The project
     * settings are available, but the board is not yet loaded. This results in overwriting
     * settings calculated from an empty board. We do not save settings until the first
     * board load operation has occured.
     */
    bool m_boardLoaded = false;
    bool m_boardLoading = false;

    /*
     * Flags to indicate whether certain events should be disabled during programmatic updates
     */
    bool m_rowExpanding = false;
    bool m_highlightingNets = false;

    /*
     * Configuration flags - these are all persisted to the project storage
     */
    bool m_filterByNetName = true;
    bool m_filterByNetclass = true;
    bool m_showZeroPadNets = false;
    bool m_showUnconnectedNets = false;
    bool m_groupByNetclass = false;
    bool m_groupByConstraint = false;
    bool m_showTimeDomainDetails = false;

    /// Custom net grouping rules
    std::vector<std::unique_ptr<EDA_COMBINED_MATCHER>> m_custom_group_rules;

    /// CSV output control
    enum class CSV_COLUMN_DESC : int
    {
        CSV_NONE = 0,
        CSV_QUOTE = 1 << 0
    };

    /// Column metadata
    struct COLUMN_DESC
    {
        COLUMN_DESC( unsigned aNum, PCB_LAYER_ID aLayer, const wxString& aDisp,
                     const wxString& aCsv, CSV_COLUMN_DESC aFlags, bool aHasUnits ) :
                num( aNum ),
                layer( aLayer ), display_name( aDisp ), csv_name( aCsv ), csv_flags( aFlags ),
                has_units( aHasUnits )
        {
        }

        unsigned int    num;
        PCB_LAYER_ID    layer;
        wxString        display_name;
        wxString        csv_name;
        CSV_COLUMN_DESC csv_flags;
        bool            has_units;

        operator unsigned int() const { return num; }
    };

    /**
     * All displayed (or hidden) columns
     */
    std::vector<COLUMN_DESC> m_columns;

    /// Column static IDs. Used to refer to columns as user re-ordering can occur
    enum
    {
        COLUMN_NAME = 0,
        COLUMN_NETCLASS,
        COLUMN_TOTAL_LENGTH,
        COLUMN_VIA_COUNT,
        COLUMN_VIA_LENGTH,
        COLUMN_BOARD_LENGTH,
        COLUMN_PAD_DIE_LENGTH,
        COLUMN_PAD_COUNT,
        COLUMN_LAST_STATIC_COL = COLUMN_PAD_COUNT
    };

    /// Popup menu item IDs
    enum POPUP_MENU_OPTIONS
    {
        ID_ADD_NET = ID_POPUP_MENU_START,
        ID_RENAME_NET,
        ID_DELETE_NET,
        ID_ADD_GROUP,
        ID_GROUP_BY_CONSTRAINT,
        ID_GROUP_BY_NETCLASS,
        ID_FILTER_BY_NET_NAME,
        ID_FILTER_BY_NETCLASS,
        ID_REMOVE_SELECTED_GROUP,
        ID_REMOVE_GROUPS,
        ID_SHOW_ZERO_NET_PADS,
        ID_SHOW_UNCONNECTED_NETS,
        ID_GENERATE_REPORT,
        ID_HIGHLIGHT_SELECTED_NETS,
        ID_CLEAR_HIGHLIGHTING,
        ID_SHOW_TIME_DOMAIN_DETAILS,
        ID_LAST_STATIC_MENU = ID_CLEAR_HIGHLIGHTING,
        ID_AUTOFIT_COLUMN = ID_SHOW_TIME_DOMAIN_DETAILS + 1,
        ID_AUTOFIT_ALL_COLUMNS,
        ID_HIDE_COLUMN,
    };

    /// Tracks which column the header context menu was opened for
    int m_contextMenuColumn = -1;
};
