/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Henner Zeller <h.zeller@acm.org>
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
 * @file lib_tree.h
 * LIB_TREE of the Qt build: the tree of the libraries of the choosers and of the library editors,
 * with its search field, its sort menu and the details of the selected item.  The interface is
 * the one of include/widgets/lib_tree.h; the EVT_LIBITEM_SELECTED and EVT_LIBITEM_CHOSEN events
 * of the wxWidgets build are callbacks.
 */

#ifndef LIB_TREE_H
#define LIB_TREE_H

#include <kiqt.h>     // Qt, without its METHOD macro
#include <QModelIndex>
#include <QWidget>

#include <functional>
#include <map>
#include <vector>

#include <lib_tree_model_adapter.h>
#include <widgets/html_window.h>

class BITMAP_BUTTON;
class LIB_ID;
class QKeyEvent;
class QLineEdit;
class QPoint;
class QTimer;
class QTreeView;
class QVBoxLayout;


/**
 * Widget displaying a tree of symbols with optional search text control and description panel.
 */
class LIB_TREE : public QWidget
{
public:
    ///< Flags to select extra widgets and options
    enum FLAGS
    {
        FLAGS_NONE  = 0x00,
        SEARCH      = 0x01,
        FILTERS     = 0x02,
        DETAILS     = 0x04,
        ALL_WIDGETS = 0x0F,
        MULTISELECT = 0x10
    };

    /**
     * Construct a symbol tree.
     *
     * @param aParent parent window containing this tree widget
     * @param aRecentSearchesKey the key of the recent searches of this kind of tree
     * @param aAdapter a LIB_TREE_MODEL_ADAPTER instance to use
     * @param aFlags if set, include a search text control or a details control
     * @param aDetails if not null, a custom HTML_WINDOW to hold symbol details. If null this
     *                 will be created inside the LIB_TREE.
     */
    LIB_TREE( QWidget* aParent, const wxString& aRecentSearchesKey,
              wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER>& aAdapter, int aFlags = ALL_WIDGETS,
              HTML_WINDOW* aDetails = nullptr );

    ~LIB_TREE() override;

    /**
     * For multi-unit symbols, if the user selects the symbol itself rather than picking
     * an individual unit, 0 will be returned in aUnit.
     *
     * @param aUnit if not NULL, the selected unit is filled in here
     * @return the library id of the symbol that has been selected
     */
    LIB_ID GetSelectedLibId( int* aUnit = nullptr ) const;

    int GetSelectionCount() const;

    /**
     * Retrieve a list of selections for trees that allow multi-selection
     *
     * @param aSelection will be filled with a list of selected LIB_IDs
     * @param aUnit is an optional pointer to a list to fill with unit numbers
     * @return the number of selected items
     */
    int GetSelectedLibIds( std::vector<LIB_ID>& aSelection,
                           std::vector<int>* aUnit = nullptr ) const;

    LIB_TREE_NODE* GetCurrentTreeNode() const;

    /**
     * Retrieve a list of pointers to selected tree nodes for trees that allow multi-selection
     *
     * @param aSelection will be filled with a list of pointers to selected tree nodes
     * @return the number of selected items
     */
    int GetSelectedTreeNodes( std::vector<LIB_TREE_NODE*>& aSelection ) const;

    /**
     * Select an item in the tree widget.
     */
    void SelectLibId( const LIB_ID& aLibId );

    /**
     * Ensure that an item is visible (preferably centered).
     */
    void CenterLibId( const LIB_ID& aLibId );

    /**
     * Unselect currently selected item in wxDataViewCtrl
     */
    void Unselect();

    /**
     * Expand and item i the tree widget.
     */
    void ExpandLibId( const LIB_ID& aLibId );

    /**
     * Get the library IDs of all expanded libraries (top-level nodes only).
     */
    std::vector<LIB_ID> GetExpandedLibraries() const;

    void ExpandAll();
    void CollapseAll();

    /**
     * Save/restore search string.
     */
    void SetSearchString( const wxString& aSearchString );
    wxString GetSearchString() const;

    void SetSortMode( LIB_TREE_MODEL_ADAPTER::SORT_MODE aMode ) { m_adapter->SetSortMode( aMode ); }
    LIB_TREE_MODEL_ADAPTER::SORT_MODE GetSortMode() const { return m_adapter->GetSortMode(); }

    /**
     * Regenerate the tree.
     */
    void Regenerate( bool aKeepState );

    /**
     * Refresh the tree (mainly to update highlighting and asterisking)
     */
    void RefreshLibTree();

    QWidget* GetFocusTarget();

    /// The layout of the filters (FILTERS flag), or nullptr.
    QVBoxLayout* GetFiltersSizer() { return m_filtersSizer; }

    void FocusSearchFieldIfExists();

    void ShowChangedLanguage();

    void BlockPreview( bool aBlock )
    {
        m_previewDisabled = aBlock;
    }

    void ShutdownPreviews();

    /// The user selected an item (EVT_LIBITEM_SELECTED of the wxWidgets build).
    void SetSelectedHandler( std::function<void()> aHandler ) { m_selectedHandler = std::move( aHandler ); }

    /// The user chose an item: double click or Enter (EVT_LIBITEM_CHOSEN of the wxWidgets build).
    void SetChosenHandler( std::function<void()> aHandler ) { m_chosenHandler = std::move( aHandler ); }

    /// The tree view of the libraries.
    QTreeView* GetTreeView() const { return m_tree_ctrl; }

protected:
    /**
     * Expand or collapse a node, switching it to the opposite state.
     */
    void toggleExpand( const QModelIndex& aTreeId );

    /**
     * If a wxDataViewitem is valid, select it and post a selection event.
     */
    void selectIfValid( const QModelIndex& aTreeId );

    void centerIfValid( const QModelIndex& aTreeId );

    void expandIfValid( const QModelIndex& aTreeId );

    /**
     * Post a wxEVT_DATAVIEW_SELECTION_CHANGED to notify the selection handler that a new
     * part has been preselected.
     */
    void postPreselectEvent();

    /**
     * Post #SYMBOL_SELECTED event to notify the selection handler that a part has been selected.
     */
    void postSelectEvent();

    /**
     * Structure storing state of the symbol tree widget.
     */
    struct STATE
    {
        /// List of expanded nodes.
        std::vector<const LIB_TREE_NODE*> expanded;

        /// Current selection, might be not valid if nothing was selected.
        LIB_ID selection;
    };

    /**
     * Return the symbol tree widget state.
     */
    STATE getState() const;

    /**
     * Restore the symbol tree widget state from an object.
     */
    void setState( const STATE& aState );

    void updateRecentSearchMenu();

    void onQueryText();

    /// The keys of the search field and of the tree: true if used.
    bool onQueryKey( QKeyEvent* aEvent );
    bool onTreeKey( QKeyEvent* aEvent );

    void onTreeSelect();
    void onTreeActivate();

    void onDetailsLink( const wxString& aHref );
    void onPreselect();
    void onItemContextMenu( const QPoint& aPos );
    void onHeaderContextMenu( const QPoint& aPos );
    void onDebounceTimer();

    /// The selected index of the view (invalid if none).
    QModelIndex currentSelection() const;

protected:
    wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER> m_adapter;

    QLineEdit*       m_query_ctrl;
    BITMAP_BUTTON*   m_sort_ctrl;
    QTreeView*       m_tree_ctrl;
    HTML_WINDOW*     m_details_ctrl;
    QTimer*          m_debounceTimer;
    bool             m_inTimerEvent;

    wxString         m_recentSearchesKey;

    QVBoxLayout*     m_filtersSizer;

    bool             m_previewDisabled;

    std::function<void()> m_selectedHandler;
    std::function<void()> m_chosenHandler;
};

#endif /* LIB_TREE_H */
