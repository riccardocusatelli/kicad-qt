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
 * @file lib_tree_model_adapter_qt.cpp
 * LIB_TREE_MODEL_ADAPTER as the QAbstractItemModel of the QTreeView of a LIB_TREE (see
 * common/lib_tree_model_adapter_wx.cpp): the indexes of the nodes, their texts, the columns and
 * the expanded libraries.  What the adapter does is in common/lib_tree_model_adapter.cpp.
 */

#include <lib_tree_model_adapter.h>

#include <QFont>
#include <QFontMetrics>
#include <QHeaderView>
#include <QTreeView>

#include <eda_base_frame.h>
#include <settings/app_settings.h>
#include <core/ki_assert.h>
#include <qt_strings.h>


static const int kDataViewIndent = 20;


LIB_TREE_NODE* LIB_TREE_MODEL_ADAPTER::ToNode( const QModelIndex& aIndex )
{
    return aIndex.isValid() ? static_cast<LIB_TREE_NODE*>( aIndex.internalPointer() ) : nullptr;
}


const std::vector<LIB_TREE_NODE*>& LIB_TREE_MODEL_ADAPTER::VisibleChildren( const LIB_TREE_NODE* aNode ) const
{
    if( !aNode )
        aNode = &m_tree;

    auto it = m_visibleChildren.find( aNode );

    if( it != m_visibleChildren.end() )
        return it->second;

    // The children shown: the ones matching the search (GetChildren() of the wxWidgets build)
    std::vector<LIB_TREE_NODE*>& children = m_visibleChildren[aNode];

    if( aNode->m_Type == LIB_TREE_NODE::TYPE::ROOT
            || aNode->m_Type == LIB_TREE_NODE::TYPE::LIBRARY
            || ( m_show_units && aNode->m_Type == LIB_TREE_NODE::TYPE::ITEM ) )
    {
        for( const std::unique_ptr<LIB_TREE_NODE>& child : aNode->m_Children )
        {
            if( child->m_Score > 0 )
            {
                m_rows[child.get()] = static_cast<int>( children.size() );
                children.push_back( child.get() );
            }
        }
    }

    return children;
}


QModelIndex LIB_TREE_MODEL_ADAPTER::ToIndex( const LIB_TREE_NODE* aNode, int aColumn ) const
{
    if( !aNode || aNode == &m_tree || aNode->m_Type == LIB_TREE_NODE::TYPE::ROOT )
        return QModelIndex();

    const LIB_TREE_NODE* parent = aNode->m_Parent ? aNode->m_Parent : &m_tree;

    // The rows of the children of the parent
    VisibleChildren( parent );

    auto it = m_rows.find( aNode );

    if( it == m_rows.end() )
        return QModelIndex();

    return createIndex( it->second, aColumn, const_cast<LIB_TREE_NODE*>( aNode ) );
}


void LIB_TREE_MODEL_ADAPTER::BeforeReset()
{
    beginResetModel();
    m_visibleChildren.clear();
    m_rows.clear();
}


void LIB_TREE_MODEL_ADAPTER::AfterReset()
{
    m_visibleChildren.clear();
    m_rows.clear();
    endResetModel();
}


std::vector<wxString> LIB_TREE_MODEL_ADAPTER::GetOpenLibs() const
{
    std::vector<wxString> openLibs;

    if( !m_widget )
        return openLibs;

    for( LIB_TREE_NODE* child : VisibleChildren( &m_tree ) )
    {
        if( m_widget->isExpanded( ToIndex( child ) ) )
            openLibs.emplace_back( child->m_LibId.GetLibNickname().wx_str() );
    }

    return openLibs;
}


void LIB_TREE_MODEL_ADAPTER::OpenLibs( const std::vector<wxString>& aLibs )
{
    if( !m_widget )
        return;

    m_widget->setUpdatesEnabled( false );

    for( const wxString& lib : aLibs )
    {
        QModelIndex item = FindItem( LIB_ID( lib, wxEmptyString ) );

        if( item.isValid() )
            m_widget->expand( item );
    }

    m_widget->setUpdatesEnabled( true );
}


void LIB_TREE_MODEL_ADAPTER::SaveSettings()
{
    if( m_widget )
    {
        m_cfg.columns = GetShownColumns();
        m_cfg.column_widths.clear();

        for( const std::pair<const wxString, COLUMN*>& pair : m_colNameMap )
        {
            if( !pair.second )
                continue;

            int width = m_widget->header()->sectionSize( pair.second->index );

            if( IsValidColumnWidth( width ) )
                m_cfg.column_widths[pair.first] = width;
        }

        m_cfg.open_libs = GetOpenLibs();
    }
}


void LIB_TREE_MODEL_ADAPTER::UpdateSearchString( const wxString& aSearch, bool aState )
{
    const LIB_TREE_NODE* firstMatch = nullptr;

    if( m_widget )
    {
        m_widget->setUpdatesEnabled( false );
        m_widget->clearSelection();
    }

    // DO NOT REMOVE THE FREEZE/THAW. This freeze/thaw is a flag for this model adapter
    // that tells it when it shouldn't trust any of the data in the model.
    Freeze();
    BeforeReset();

    scoreAndSortNodes( aSearch );

    AfterReset();
    Thaw();

    // The view collapsed its rows with the reset: the matches are expanded again
    if( m_widget )
    {
        firstMatch = showResults();
        m_widget->setUpdatesEnabled( true );
    }

    if( firstMatch && m_widget )
    {
        QModelIndex item = ToIndex( firstMatch );
        m_widget->setCurrentIndex( item );

        // Make sure the *parent* item is visible. The selected item is the first (shown) child
        // of the parent. So it's always right below the parent, and this way the user can also
        // see what library the selected part belongs to.
        if( firstMatch->m_Parent )
            m_widget->scrollTo( ToIndex( firstMatch->m_Parent ) );

        m_widget->scrollTo( item );
    }
}


void LIB_TREE_MODEL_ADAPTER::AttachTo( QTreeView* aTreeView )
{
    m_widget = aTreeView;
    aTreeView->setIndentation( kDataViewIndent );
    aTreeView->setModel( this );
    recreateColumns();
}


void LIB_TREE_MODEL_ADAPTER::recreateColumns()
{
    beginResetModel();

    m_columns.clear();
    m_colIdxMap.clear();

    // The columns not shown are kept as known names (addColumnIfNecessary())
    for( std::pair<const wxString, COLUMN*>& pair : m_colNameMap )
        pair.second = nullptr;

    m_visibleChildren.clear();
    m_rows.clear();

    // The Item column is always shown
    doAddColumn( wxT( "Item" ) );
    createMissingColumns();

    endResetModel();

    if( m_widget )
    {
        for( const std::unique_ptr<COLUMN>& col : m_columns )
            m_widget->header()->resizeSection( col->index, m_colWidths[m_colIdxMap[col->index]] );
    }
}


void LIB_TREE_MODEL_ADAPTER::createMissingColumns()
{
    for( const wxString& colName : m_shownColumns )
    {
        if( !m_colNameMap.count( colName ) || !m_colNameMap[colName] )
            doAddColumn( colName, colName == wxT( "Description" ) );
    }
}


void LIB_TREE_MODEL_ADAPTER::resortTree()
{
    Freeze();
    BeforeReset();

    m_tree.SortNodes( m_sort_mode == BEST_MATCH );

    AfterReset();
    Thaw();
}


LIB_TREE_MODEL_ADAPTER::COLUMN* LIB_TREE_MODEL_ADAPTER::doAddColumn( const wxString& aHeader, bool aTranslate )
{
    wxString translatedHeader = aTranslate ? wxGetTranslation( aHeader ) : aHeader;

    // The extent of the text doesn't take into account the space on either side
    // in the header, so artificially pad it
    int headerMinWidth = m_widget ? QFontMetrics( m_widget->font() ).horizontalAdvance(
                                            ToQString( translatedHeader + wxT( "MMM" ) ) )
                                  : 0;

    if( !m_colWidths.count( aHeader ) || m_colWidths[aHeader] < headerMinWidth )
        m_colWidths[aHeader] = headerMinWidth;

    int index = (int) m_columns.size();

    m_columns.emplace_back( std::make_unique<COLUMN>( COLUMN{ translatedHeader, index } ) );
    m_colNameMap[aHeader] = m_columns.back().get();
    m_colIdxMap[index] = aHeader;

    return m_columns.back().get();
}


LIB_ID LIB_TREE_MODEL_ADAPTER::GetAliasFor( const QModelIndex& aSelection ) const
{
    const LIB_TREE_NODE* node = ToNode( aSelection );
    return node ? node->m_LibId : LIB_ID();
}


int LIB_TREE_MODEL_ADAPTER::GetUnitFor( const QModelIndex& aSelection ) const
{
    const LIB_TREE_NODE* node = ToNode( aSelection );
    return node ? node->m_Unit : 0;
}


LIB_TREE_NODE::TYPE LIB_TREE_MODEL_ADAPTER::GetTypeFor( const QModelIndex& aSelection ) const
{
    const LIB_TREE_NODE* node = ToNode( aSelection );
    return node ? node->m_Type : LIB_TREE_NODE::TYPE::INVALID;
}


LIB_TREE_NODE* LIB_TREE_MODEL_ADAPTER::GetTreeNodeFor( const QModelIndex& aSelection ) const
{
    return ToNode( aSelection );
}


QModelIndex LIB_TREE_MODEL_ADAPTER::FindItem( const LIB_ID& aLibId )
{
    return ToIndex( findNode( aLibId ) );
}


QModelIndex LIB_TREE_MODEL_ADAPTER::GetCurrentDataViewItem()
{
    return FindItem( m_preselect_lib_id );
}


void LIB_TREE_MODEL_ADAPTER::FinishTreeInitialization()
{
    if( !m_widget || m_columns.empty() )
        return;

    QHeaderView* header = m_widget->header();
    int          totalWidth = 0;
    size_t       idx = 0;

    for( ; idx < m_columns.size() - 1; idx++ )
    {
        KI_ASSERT( m_colIdxMap.count( idx ) );

        const wxString& name = m_colIdxMap[idx];

        header->resizeSection( static_cast<int>( idx ), m_colWidths[name] );
        totalWidth += m_colWidths[name];
    }

    // The last column fills the width of the view
    int remainingWidth = m_widget->viewport()->width() - totalWidth;
    const wxString& name = m_colIdxMap[idx];

    header->resizeSection( static_cast<int>( idx ), std::max( m_colWidths[name], remainingWidth ) );
}


void LIB_TREE_MODEL_ADAPTER::RefreshTree()
{
    // The texts of the nodes changed (not the tree)
    if( m_widget )
        m_widget->viewport()->update();
}


bool LIB_TREE_MODEL_ADAPTER::IsContainer( const LIB_TREE_NODE* aNode ) const
{
    return aNode ? !aNode->m_Children.empty() : true;
}


wxString LIB_TREE_MODEL_ADAPTER::GetValue( const LIB_TREE_NODE* aNode, unsigned int aCol ) const
{
    return nodeText( aNode, aCol );
}


bool LIB_TREE_MODEL_ADAPTER::IsItalic( const LIB_TREE_NODE* aNode, unsigned int aCol ) const
{
    // Names of non-root aliases are italicized
    return aNode->m_Type == LIB_TREE_NODE::TYPE::ITEM && !aNode->m_IsRoot && aCol == 0;
}


QModelIndex LIB_TREE_MODEL_ADAPTER::index( int aRow, int aColumn, const QModelIndex& aParent ) const
{
    if( aRow < 0 || aColumn < 0 || aColumn >= columnCount( aParent ) )
        return QModelIndex();

    const LIB_TREE_NODE*               parent = aParent.isValid() ? ToNode( aParent ) : &m_tree;
    const std::vector<LIB_TREE_NODE*>& children = VisibleChildren( parent );

    if( aRow >= static_cast<int>( children.size() ) )
        return QModelIndex();

    return createIndex( aRow, aColumn, children[aRow] );
}


QModelIndex LIB_TREE_MODEL_ADAPTER::parent( const QModelIndex& aIndex ) const
{
    const LIB_TREE_NODE* node = ToNode( aIndex );

    if( !node || m_freeze )
        return QModelIndex();

    // The top level nodes have no parent in the view (the root isn't shown)
    if( !node->m_Parent || node->m_Parent->m_Type == LIB_TREE_NODE::TYPE::ROOT )
        return QModelIndex();

    return ToIndex( node->m_Parent );
}


int LIB_TREE_MODEL_ADAPTER::rowCount( const QModelIndex& aParent ) const
{
    if( aParent.isValid() && aParent.column() != 0 )
        return 0;

    const LIB_TREE_NODE* parent = aParent.isValid() ? ToNode( aParent ) : &m_tree;

    return static_cast<int>( VisibleChildren( parent ).size() );
}


int LIB_TREE_MODEL_ADAPTER::columnCount( const QModelIndex& aParent ) const
{
    return static_cast<int>( m_columns.size() );
}


bool LIB_TREE_MODEL_ADAPTER::hasChildren( const QModelIndex& aParent ) const
{
    if( !aParent.isValid() )
        return true;

    return rowCount( aParent ) > 0;
}


QVariant LIB_TREE_MODEL_ADAPTER::data( const QModelIndex& aIndex, int aRole ) const
{
    const LIB_TREE_NODE* node = ToNode( aIndex );

    if( !node || IsFrozen() )
        return QVariant();

    switch( aRole )
    {
    case Qt::DisplayRole:
        return ToQString( GetValue( node, aIndex.column() ) );

    case Qt::ToolTipRole:
        if( aIndex.column() == NAME_COL && !node->m_Desc.IsEmpty() )
            return ToQString( node->m_Desc );

        return QVariant();

    case Qt::FontRole:
        if( IsItalic( node, aIndex.column() ) && m_widget )
        {
            QFont font = m_widget->font();
            font.setItalic( true );
            return font;
        }

        return QVariant();

    default:
        return QVariant();
    }
}


QVariant LIB_TREE_MODEL_ADAPTER::headerData( int aSection, Qt::Orientation aOrientation, int aRole ) const
{
    if( aOrientation != Qt::Horizontal || aRole != Qt::DisplayRole || aSection < 0
            || aSection >= static_cast<int>( m_columns.size() ) )
    {
        return QVariant();
    }

    return ToQString( m_columns[aSection]->title );
}


Qt::ItemFlags LIB_TREE_MODEL_ADAPTER::flags( const QModelIndex& aIndex ) const
{
    if( !aIndex.isValid() )
        return Qt::NoItemFlags;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}


void LIB_TREE_MODEL_ADAPTER::expandAncestors( const LIB_TREE_NODE* aNode )
{
    if( !m_widget )
        return;

    for( const LIB_TREE_NODE* parent = aNode->m_Parent;
         parent && parent->m_Type != LIB_TREE_NODE::TYPE::ROOT; parent = parent->m_Parent )
    {
        m_widget->expand( ToIndex( parent ) );
    }
}


void LIB_TREE_MODEL_ADAPTER::ensureVisible( const LIB_TREE_NODE* aNode )
{
    if( m_widget )
        m_widget->scrollTo( ToIndex( aNode ) );
}
