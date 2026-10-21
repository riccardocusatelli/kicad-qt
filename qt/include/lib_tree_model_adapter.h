/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright (C) 2014 Henner Zeller <h.zeller@acm.org>
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
 * @file lib_tree_model_adapter.h
 * LIB_TREE_MODEL_ADAPTER of the Qt build: the adapter between the tree of the libraries
 * (LIB_TREE_NODE) and the QTreeView of a LIB_TREE, as a QAbstractItemModel.  It has the interface
 * of include/lib_tree_model_adapter.h (the wxDataViewModel of the wxWidgets build), with the
 * QModelIndex of the view in place of the wxDataViewItem.  What the adapter does is shared
 * (common/lib_tree_model_adapter.cpp); the model is in lib_tree_model_adapter_qt.cpp.
 *
 * As in the wxWidgets build, the adapter is reference counted (wxObjectDataPtr): it is created by
 * the static factory methods of its derived classes and must not be deleted.
 */

#ifndef LIB_TREE_MODEL_ADAPTER_H
#define LIB_TREE_MODEL_ADAPTER_H

#include <kiqt.h>     // Qt, without its METHOD macro
#include <QAbstractItemModel>

#include <lib_tree_pinning.h>
#include <lib_id.h>
#include <lib_tree_model.h>
#include <settings/app_settings.h>
#include <project.h>

#include <wx/object.h>
#include <wx/dataview.h>

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

class TOOL_DISPATCHER;
class TOOL_INTERACTIVE;
class EDA_BASE_FRAME;
class wxWindow;


/**
 * The view of the tree of a library: a QTreeView with the call of wxDataViewCtrl that the
 * adapters of the editors make on it.
 */
class LIB_TREE_VIEW : public QTreeView
{
public:
    explicit LIB_TREE_VIEW( QWidget* aParent = nullptr ) : QTreeView( aParent ) {}

    /// The item is expanded in the view (wxDataViewCtrl::IsExpanded()).
    bool IsExpanded( const wxDataViewItem& aItem ) const;
};



class LIB_TREE_MODEL_ADAPTER : public QAbstractItemModel, public wxRefCounter
{
public:
    /**
     * @return a unicode string to mark a node name like a pinned library name.
     * This is not an ASCII7 char, but a unicode char.
     */
    static const wxString GetPinningSymbol()
    {
        return LibTreePinningSymbol();
    }

    /// Upper bound for a persisted column width; larger values are treated as corrupt settings.
    static constexpr int MAX_COL_WIDTH = 100000;

    /**
     * @return true if @p aWidth is a plausible persisted column width, false if it is corrupt.
     */
    static bool IsValidColumnWidth( int aWidth );

public:
    ~LIB_TREE_MODEL_ADAPTER() override;

    /**
     * This enum defines the order of the default columns in the tree view
     */
    enum TREE_COLS
    {
        NAME_COL = 0,   ///< Library or library item name column
        DESC_COL,       ///< Library or library description column
        NUM_COLS        ///< The number of default tree columns
    };

    enum SORT_MODE
    {
        BEST_MATCH = 0,
        ALPHABETIC
    };

    /**
     * Save the column widths to the config file. This requires the tree view to still be
     * alive.
     */
    void SaveSettings();

    /**
     * Set the filter.
     *
     * If set (non-null), the filter will be applied to all symbols.
     */
    void SetFilter( std::function<bool( LIB_TREE_NODE& aNode )>* aFilter ) { m_filter = aFilter; }

    /**
     * Return the active filter.
     */
    std::function<bool( LIB_TREE_NODE& aNode )>* GetFilter() const { return m_filter; }

    void SetSortMode( SORT_MODE aMode ) { m_sort_mode = aMode; }
    SORT_MODE GetSortMode() const { return m_sort_mode; }

    /**
     * Whether or not to show units. May be set at any time; updates at the next
     * UpdateSearchString()
     *
     * @param aShow if true, units are displayed
     */
    void ShowUnits( bool aShow );

    /**
     * Set the symbol name to be selected if there are no search results.
     * May be set at any time; updates at the next UpdateSearchString().
     *
     * @param aLibId    symbol #LIB_ID to be selected
     * @param aUnit     unit to be selected, if > 0 (0 selects the alias itself)
     */
    void SetPreselectNode( const LIB_ID& aLibId, int aUnit );

    /**
     * Add the given list of symbols by alias. To be called in the setup
     * phase.
     *
     * @param aNodeName    the parent node the symbols will appear under
     * @param aDesc        the description field of the parent node
     * @param aItemList    list of symbols
     */
    LIB_TREE_NODE_LIBRARY& DoAddLibrary( const wxString& aNodeName, const wxString& aDesc,
                                         const std::vector<LIB_TREE_ITEM*>& aItemList,
                                         bool pinned, bool presorted );

    /**
     * Remove one of the system groups from the library.
     */
    void RemoveGroup( bool aRecentlyUsedGroup, bool aAlreadyPlacedGroup );

    std::vector<wxString> GetAvailableColumns() const { return m_availableColumns; }

    std::vector<wxString> GetShownColumns() const { return m_shownColumns; }

    std::vector<wxString> GetOpenLibs() const;
    void OpenLibs( const std::vector<wxString>& aLibs );

    /// Registers a function to be called whenever new lazy-loaded library content is available
    void RegisterLazyLoadHandler( std::function<void()>&& aHandler )
    {
        m_lazyLoadHandler = aHandler;
    }

    /**
     * Sets which columns are shown in the widget.  Invalid column names are discarded.
     * @param aColumnNames is an ordered list of column names to show
     */
    void SetShownColumns( const std::vector<wxString>& aColumnNames );

    /**
     * Sort the tree and assign ranks after adding libraries.
     */
    void AssignIntrinsicRanks()
    {
        m_tree.AssignIntrinsicRanks( m_shownColumns );

        for( const std::unique_ptr<LIB_TREE_NODE>& child : m_tree.m_Children )
            child->AssignIntrinsicRanks( m_shownColumns );
    }

    /**
     * Set the search string provided by the user.
     *
     * @param aSearch full, unprocessed search text
     * @param aState if true, we are keeping the state and so we shouldn't collapse the tree
     */
    void UpdateSearchString( const wxString& aSearch, bool aState );

    /**
     * Attach to a tree view.
     *
     * @param aTreeView the view to attach to
     */
    void AttachTo( LIB_TREE_VIEW* aTreeView );

    /**
     * A final-stage initialization to be called after the window hierarchy has been realized
     * and the window sizes set.
     */
    void FinishTreeInitialization();

    /**
     * Return the alias for the given item.
     */
    LIB_ID GetAliasFor( const QModelIndex& aSelection ) const;

    /**
     * Return the unit for the given item.
     */
    int GetUnitFor( const QModelIndex& aSelection ) const;

    /**
     * Return node type for the given item.
     */
    LIB_TREE_NODE::TYPE GetTypeFor( const QModelIndex& aSelection ) const;

    LIB_TREE_NODE* GetTreeNodeFor( const QModelIndex& aSelection ) const;

    virtual wxString GenerateInfo( const LIB_ID& aLibId, int aUnit ) { return wxEmptyString; }

    virtual bool HasPreview( const wxDataViewItem& aItem ) { return false; }
    virtual void ShowPreview( wxWindow* aParent, const wxDataViewItem& aItem ) {}
    virtual void ShutdownPreview( wxWindow* aParent ) {}

    TOOL_DISPATCHER* GetToolDispatcher() const;

    /**
     * Return the number of symbols loaded in the tree.
     */
    int GetItemCount() const;

    /**
     * Return the number of libraries loaded in the tree.
     */
    virtual int GetLibrariesCount() const
    {
        return (int) m_tree.m_Children.size();
    }

    /**
     * Returns the item of the alias (invalid if not found).
     */
    wxDataViewItem FindItem( const LIB_ID& aLibId );

    virtual wxDataViewItem GetCurrentDataViewItem();

    /// The node of an index of the view (nullptr for the root).
    static LIB_TREE_NODE* ToNode( const QModelIndex& aIndex );

    /// The node of an item (the wxDataViewModel interface of the adapters of the editors).
    static LIB_TREE_NODE* ToNode( const wxDataViewItem& aItem )
    {
        return static_cast<LIB_TREE_NODE*>( aItem.GetID() );
    }

    /// The item of a node.
    static wxDataViewItem ToItem( const LIB_TREE_NODE* aNode )
    {
        return wxDataViewItem( const_cast<LIB_TREE_NODE*>( aNode ) );
    }

    /// The index in the view of an item (invalid for the root or an item not shown).
    QModelIndex ToIndex( const wxDataViewItem& aItem ) const { return ToIndex( ToNode( aItem ) ); }

    /// The index in the view of a node (invalid for the root or a node not shown).
    QModelIndex ToIndex( const LIB_TREE_NODE* aNode, int aColumn = 0 ) const;

    /// The children of a node shown in the view (the ones matching the search).
    const std::vector<LIB_TREE_NODE*>& VisibleChildren( const LIB_TREE_NODE* aNode ) const;

    // Freezing/Thawing.  Used when updating the table model so that we don't try and fetch
    // values during updating.
    void Freeze() { m_freeze++; }
    void Thaw() { m_freeze--; }
    bool IsFrozen() const { return m_freeze; }

    /// The tree of the nodes is about to change (wxDataViewModel::BeforeReset()).
    void BeforeReset();

    /// The tree of the nodes changed: the view shows it again (wxDataViewModel::AfterReset()).
    void AfterReset();

    class ResetTreeView
    {
    public:
        explicit ResetTreeView( LIB_TREE_MODEL_ADAPTER& aAdapter ) :
                m_adapter( aAdapter )
        {
            m_adapter.Freeze();
            m_adapter.BeforeReset();
        }

        ~ResetTreeView()
        {
            m_adapter.AfterReset();
            m_adapter.Thaw();
        }

        ResetTreeView( const ResetTreeView& ) = delete;
        ResetTreeView& operator=( const ResetTreeView& ) = delete;

    private:
        LIB_TREE_MODEL_ADAPTER& m_adapter;
    };

    void RefreshTree();

    // Allows subclasses to nominate a context menu handler.
    virtual TOOL_INTERACTIVE* GetContextMenuTool() { return nullptr; }

    void PinLibrary( LIB_TREE_NODE* aTreeNode );
    void UnpinLibrary( LIB_TREE_NODE* aTreeNode );

    void ShowChangedLanguage();

    // The interface of QAbstractItemModel

    QModelIndex index( int aRow, int aColumn, const QModelIndex& aParent = QModelIndex() ) const override;
    QModelIndex parent( const QModelIndex& aIndex ) const override;
    int         rowCount( const QModelIndex& aParent = QModelIndex() ) const override;
    int         columnCount( const QModelIndex& aParent = QModelIndex() ) const override;
    bool        hasChildren( const QModelIndex& aParent = QModelIndex() ) const override;
    QVariant    data( const QModelIndex& aIndex, int aRole = Qt::DisplayRole ) const override;
    QVariant    headerData( int aSection, Qt::Orientation aOrientation,
                            int aRole = Qt::DisplayRole ) const override;
    Qt::ItemFlags flags( const QModelIndex& aIndex ) const override;

protected:
    /**
     * Constructor
     */
    LIB_TREE_MODEL_ADAPTER( EDA_BASE_FRAME* aParent, const wxString& aPinnedKey,
                            APP_SETTINGS_BASE::LIB_TREE& aSettingsStruct );

    LIB_TREE_NODE_LIBRARY& DoAddLibraryNode( const wxString& aNodeName, const wxString& aDesc,
                                             bool pinned );

    virtual void loadColumnConfig();

    // The interface of wxDataViewModel that the adapters of the editors override: the model of
    // Qt shows what they give.

    /// The item can have children.
    virtual bool IsContainer( const wxDataViewItem& aItem ) const;

    /// The text of an item for the column @a aCol.
    virtual void GetValue( wxVariant& aVariant, const wxDataViewItem& aItem,
                           unsigned int aCol ) const;

    /// The attributes of the text of an item (bold, italics, ...): false if none.
    virtual bool GetAttr( const wxDataViewItem& aItem, unsigned int aCol,
                          wxDataViewItemAttr& aAttr ) const;

    virtual PROJECT::LIB_TYPE_T getLibType() = 0;

    void resortTree();

    // What the adapter does, without the GUI toolkit (common/lib_tree_model_adapter.cpp)

    /// The node of a library (empty item name) or of an item, or nullptr.
    LIB_TREE_NODE* findNode( const LIB_ID& aLibId );

    /// The text of the column @a aCol of a node.
    wxString nodeText( const LIB_TREE_NODE* aNode, unsigned int aCol ) const;

    /// Score and sort the nodes for a search string.
    void scoreAndSortNodes( const wxString& aSearch );

private:
    /**
     * Find and expand successful search results.  Return the best match (if any).
     */
    const LIB_TREE_NODE* showResults();

    // The view of the GUI toolkit (lib_tree_model_adapter_qt.cpp)

    /// Expand the parents of a node in the view.
    void expandAncestors( const LIB_TREE_NODE* aNode );

    /// Scroll the view to show a node.
    void ensureVisible( const LIB_TREE_NODE* aNode );

    /// A column of the view.
    struct COLUMN
    {
        wxString title;
        int      index;
    };

    COLUMN* doAddColumn( const wxString& aHeader, bool aTranslate = true );

protected:
    void addColumnIfNecessary( const wxString& aHeader );

    void recreateColumns();
    void createMissingColumns();

protected:
    EDA_BASE_FRAME*              m_parent;
    APP_SETTINGS_BASE::LIB_TREE& m_cfg;

    LIB_TREE_NODE_ROOT           m_tree;
    std::map<unsigned, wxString> m_colIdxMap;
    std::vector<wxString>        m_availableColumns;
    std::map<wxString, int>      m_colWidths;
    std::vector<wxString>        m_shownColumns;   // Stored in display order

    LIB_TREE_VIEW*               m_widget;

    std::function<void()>        m_lazyLoadHandler;

private:
    SORT_MODE                    m_sort_mode;
    bool                         m_show_units;
    LIB_ID                       m_preselect_lib_id;
    int                          m_preselect_unit;
    int                          m_freeze;

    std::function<bool( LIB_TREE_NODE& aNode )>* m_filter;

    std::vector<std::unique_ptr<COLUMN>>         m_columns;
    std::map<wxString, COLUMN*>                  m_colNameMap;

    /// The children shown in the view of the nodes, and the rows of the nodes (built when the
    /// view asks for them, cleared when the tree changes)
    mutable std::unordered_map<const LIB_TREE_NODE*, std::vector<LIB_TREE_NODE*>> m_visibleChildren;
    mutable std::unordered_map<const LIB_TREE_NODE*, int>                         m_rows;
};

#endif // LIB_TREE_MODEL_ADAPTER_H
