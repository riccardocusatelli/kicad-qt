/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright (C) 2014 Henner Zeller <h.zeller@acm.org>
 * Copyright (C) 2023 CERN
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
 * @file lib_tree_model_adapter.cpp
 * What LIB_TREE_MODEL_ADAPTER does without the GUI toolkit: the libraries and their items, the
 * columns, the search and the pinned libraries.  The model of the view is in
 * lib_tree_model_adapter_wx.cpp (and in qt/common/lib_tree_model_adapter_qt.cpp).
 */

#include <eda_base_frame.h>
#include <eda_pattern_match.h>
#include <kiface_base.h>
#include <lib_tree_model_adapter.h>
#include <project/project_file.h>
#include <settings/app_settings.h>
#include <wx/tokenzr.h>
#include <wx/log.h>
#include <string_utils.h>
#include <core/ki_assert.h>


bool LIB_TREE_MODEL_ADAPTER::IsValidColumnWidth( int aWidth )
{
    // An out-of-range persisted width (seen after mixed-DPI monitor changes) can push the
    // tree content out of view and leave the chooser unusable, so anything outside a width
    // that could legitimately fit on a display is treated as corrupt rather than a resize.
    return aWidth > 0 && aWidth <= MAX_COL_WIDTH;
}


LIB_TREE_MODEL_ADAPTER::LIB_TREE_MODEL_ADAPTER( EDA_BASE_FRAME* aParent, const wxString& aPinnedKey,
                                                APP_SETTINGS_BASE::LIB_TREE& aSettingsStruct ) :
        m_parent( aParent ),
        m_cfg( aSettingsStruct ),
        m_widget( nullptr ),
        m_lazyLoadHandler( nullptr ),
        m_sort_mode( BEST_MATCH ),
        m_show_units( true ),
        m_preselect_unit( 0 ),
        m_freeze( 0 ),
        m_filter( nullptr )
{
    // Default column widths.  Do not translate these names.
    m_colWidths[ _HKI( "Item" ) ] = 300;
    m_colWidths[ _HKI( "Description" ) ] = 600;

    m_availableColumns = { _HKI( "Item" ), _HKI( "Description" ) };

    loadColumnConfig();
}


LIB_TREE_MODEL_ADAPTER::~LIB_TREE_MODEL_ADAPTER()
{}


TOOL_DISPATCHER* LIB_TREE_MODEL_ADAPTER::GetToolDispatcher() const
{
    return m_parent->GetToolDispatcher();
}


void LIB_TREE_MODEL_ADAPTER::loadColumnConfig()
{
    for( const std::pair<const wxString, int>& pair : m_cfg.column_widths )
    {
        if( IsValidColumnWidth( pair.second ) )
            m_colWidths[pair.first] = pair.second;
    }

    m_shownColumns = m_cfg.columns;

    if( m_shownColumns.empty() )
        m_shownColumns = {  _HKI( "Item" ), _HKI( "Description" ) };

    if( m_shownColumns[0] != _HKI( "Item" ) )
        m_shownColumns.insert( m_shownColumns.begin(), _HKI( "Item" ) );
}


void LIB_TREE_MODEL_ADAPTER::ShowUnits( bool aShow )
{
    m_show_units = aShow;
}


void LIB_TREE_MODEL_ADAPTER::SetPreselectNode( const LIB_ID& aLibId, int aUnit )
{
    m_preselect_lib_id = aLibId;
    m_preselect_unit = aUnit;
}


LIB_TREE_NODE_LIBRARY& LIB_TREE_MODEL_ADAPTER::DoAddLibraryNode( const wxString& aNodeName, const wxString& aDesc,
                                                                 bool pinned )
{
    LIB_TREE_NODE_LIBRARY& lib_node = m_tree.AddLib( aNodeName, aDesc );

    lib_node.m_Pinned = pinned;

    return lib_node;
}


LIB_TREE_NODE_LIBRARY& LIB_TREE_MODEL_ADAPTER::DoAddLibrary( const wxString& aNodeName, const wxString& aDesc,
                                                             const std::vector<LIB_TREE_ITEM*>& aItemList,
                                                             bool pinned, bool presorted )
{
    LIB_TREE_NODE_LIBRARY& lib_node = DoAddLibraryNode( aNodeName, aDesc, pinned );

    for( LIB_TREE_ITEM* item: aItemList )
    {
        if( item )
            lib_node.AddItem( item );
    }

    lib_node.AssignIntrinsicRanks( m_shownColumns, presorted );

    return lib_node;
}


void LIB_TREE_MODEL_ADAPTER::RemoveGroup( bool aRecentGroup, bool aPlacedGroup )
{
    m_tree.RemoveGroup( aRecentGroup, aPlacedGroup );
}


LIB_TREE_NODE* LIB_TREE_MODEL_ADAPTER::findNode( const LIB_ID& aLibId )
{
    for( std::unique_ptr<LIB_TREE_NODE>& lib: m_tree.m_Children )
    {
        if( lib->m_Name != aLibId.GetLibNickname().wx_str() )
            continue;

        // if part name is not specified, return the library node
        if( aLibId.GetLibItemName() == "" )
            return lib.get();

        for( std::unique_ptr<LIB_TREE_NODE>& alias: lib->m_Children )
        {
            if( alias->m_Name == aLibId.GetLibItemName().wx_str() )
                return alias.get();
        }

        break;  // could not find the part in the requested library
    }

    return nullptr;
}


wxString LIB_TREE_MODEL_ADAPTER::nodeText( const LIB_TREE_NODE* node, unsigned int aCol ) const
{
    wxString valueStr;

    switch( aCol )
    {
    case NAME_COL:
        if( node->m_Pinned )
            valueStr = GetPinningSymbol() + UnescapeString( node->m_Name );
        else
            valueStr = UnescapeString( node->m_Name );

        break;

    default:
        if( m_colIdxMap.count( aCol ) )
        {
            const wxString& key = m_colIdxMap.at( aCol );

            if( key == wxT( "Description" ) )
                valueStr = UnescapeString( node->m_Desc );
            else if( node->m_Fields.count( key ) )
                valueStr = UnescapeString( node->m_Fields.at( key ) );
            else
                valueStr = wxEmptyString;
        }

        break;
    }

    valueStr.Replace( wxS( "\n" ), wxS( " " ) ); // Clear line breaks

    return valueStr;
}


void LIB_TREE_MODEL_ADAPTER::scoreAndSortNodes( const wxString& aSearch )
{
    // Don't cause KiCad to hang if someone accidentally pastes the PCB or schematic into
    // the search box.
    constexpr int MAX_TERMS = 100;

    wxStringTokenizer                                  tokenizer( aSearch, " \t\r\n", wxTOKEN_STRTOK );
    std::vector<std::unique_ptr<EDA_COMBINED_MATCHER>> termMatchers;

    while( tokenizer.HasMoreTokens() && termMatchers.size() < MAX_TERMS )
    {
        wxString term = tokenizer.GetNextToken().Lower();
        termMatchers.emplace_back( std::make_unique<EDA_COMBINED_MATCHER>( term, CTX_LIBITEM ) );
    }

    m_tree.UpdateScore( termMatchers, m_filter );

    m_tree.SortNodes( m_sort_mode == BEST_MATCH );
}


void LIB_TREE_MODEL_ADAPTER::PinLibrary( LIB_TREE_NODE* aTreeNode )
{
    m_parent->Prj().PinLibrary( aTreeNode->m_LibId.GetLibNickname(), getLibType() );
    aTreeNode->m_Pinned = true;

    resortTree();
    ensureVisible( aTreeNode );
}


void LIB_TREE_MODEL_ADAPTER::UnpinLibrary( LIB_TREE_NODE* aTreeNode )
{
    m_parent->Prj().UnpinLibrary( aTreeNode->m_LibId.GetLibNickname(), getLibType() );
    aTreeNode->m_Pinned = false;

    resortTree();
    // Keep focus at top when unpinning
}


void LIB_TREE_MODEL_ADAPTER::ShowChangedLanguage()
{
    recreateColumns();

    for( const std::unique_ptr<LIB_TREE_NODE>& lib: m_tree.m_Children )
    {
        if( lib->m_IsRecentlyUsedGroup )
            lib->m_Name = wxT( "-- " ) + _( "Recently Used" ) + wxT( " --" );
        else if( lib->m_IsAlreadyPlacedGroup )
            lib->m_Name = wxT( "-- " ) + _( "Already Placed" ) + wxT( " --" );
    }
}


void LIB_TREE_MODEL_ADAPTER::addColumnIfNecessary( const wxString& aHeader )
{
    if( m_colNameMap.count( aHeader ) )
        return;

    // Columns will be created later
    m_colNameMap[aHeader] = nullptr;
    m_availableColumns.emplace_back( aHeader );
}


void LIB_TREE_MODEL_ADAPTER::SetShownColumns( const std::vector<wxString>& aColumnNames )
{
    bool recreate = m_shownColumns != aColumnNames;

    m_shownColumns = aColumnNames;

    if( recreate && m_widget )
        recreateColumns();

    for( std::unique_ptr<LIB_TREE_NODE>& lib: m_tree.m_Children )
        lib->AssignIntrinsicRanks( m_shownColumns );
}


int LIB_TREE_MODEL_ADAPTER::GetItemCount() const
{
    int n = 0;

    for( const std::unique_ptr<LIB_TREE_NODE>& lib: m_tree.m_Children )
        n += lib->m_Children.size();

    return n;
}


void recursiveDescent( LIB_TREE_NODE& aNode, const std::function<int( const LIB_TREE_NODE* )>& f )
{
    for( std::unique_ptr<LIB_TREE_NODE>& node: aNode.m_Children )
    {
        int r = f( node.get() );

        if( r == 0 )
            break;
        else if( r == -1 )
            continue;

        recursiveDescent( *node, f );
    }
}


const LIB_TREE_NODE* LIB_TREE_MODEL_ADAPTER::showResults()
{
    const LIB_TREE_NODE* firstMatch = nullptr;

    // Expand parents of leaf nodes with some level of matching
    recursiveDescent( m_tree,
            [&]( const LIB_TREE_NODE* n )
            {
                if( n->m_Type == LIB_TREE_NODE::TYPE::ITEM && n->m_Score > 1 )
                {
                    // Mirror the sort order (see LIB_TREE_NODE::Compare): an exact match
                    // outranks any score, otherwise the higher score wins.  Otherwise the
                    // view would scroll to a high-scoring item that isn't at the top.
                    if( !firstMatch )
                        firstMatch = n;
                    else if( n->m_ExactMatch && !firstMatch->m_ExactMatch )
                        firstMatch = n;
                    else if( n->m_ExactMatch == firstMatch->m_ExactMatch && n->m_Score > firstMatch->m_Score )
                        firstMatch = n;

                    expandAncestors( n );
                }

                return 1; // keep going to expand ancestors of all found items
            } );

    // If no matches, find and show the preselect node
    if( !firstMatch && m_preselect_lib_id.IsValid() )
    {
        recursiveDescent( m_tree,
                [&]( const LIB_TREE_NODE* n )
                {
                    // Don't match the recent and already placed libraries
                    if( n->m_Name.StartsWith( "-- " ) )
                        return -1; // Skip this node and its children

                    if( n->m_Type == LIB_TREE_NODE::TYPE::ITEM
                              && ( n->m_Children.empty() || !m_preselect_unit )
                              && m_preselect_lib_id == n->m_LibId )
                    {
                        firstMatch = n;
                        expandAncestors( n );
                        return 0;
                    }
                    else if( n->m_Type == LIB_TREE_NODE::TYPE::UNIT
                              && ( m_preselect_unit && m_preselect_unit == n->m_Unit )
                              && m_preselect_lib_id == n->m_Parent->m_LibId )
                    {
                        firstMatch = n;
                        expandAncestors( n );
                        return 0;
                    }

                    return 1;
                } );
    }

    // If still no matches expand a single library if there is only one
    if( !firstMatch )
    {
        int libraries = 0;

        for( const std::unique_ptr<LIB_TREE_NODE>& child : m_tree.m_Children )
        {
            if( !child->m_Name.StartsWith( "-- " ) )
                 libraries++;
        }

        if( libraries != 1 )
            return nullptr;

        recursiveDescent( m_tree,
                [&]( const LIB_TREE_NODE* n )
                {
                    if( n->m_Type == LIB_TREE_NODE::TYPE::ITEM )
                    {
                        firstMatch = n;
                        expandAncestors( n );
                        return 0;
                    }

                    return 1;
                } );
    }

    return firstMatch;
}
