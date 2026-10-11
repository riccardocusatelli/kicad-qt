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
 * @file pcb_net_inspector_panel.cpp
 * What the Net Inspector does, without the GUI toolkit: the nets and their statistics, the
 * filters and groups, the commands of the menus, the report and the settings.  The view is in
 * pcb_net_inspector_panel_wx.cpp (and in qt/pcbnew/widgets/pcb_net_inspector_panel_qt.cpp).
 */

#include <properties/property.h>
#include <widgets/pcb_net_inspector_panel.h>
#include <widgets/pcb_net_inspector_panel_data_model.h>

#include <advanced_config.h>
#include <board_design_settings.h>
#include <class_draw_panel_gal.h>
#include <confirm.h>
#include <connectivity/connectivity_algo.h>
#include <footprint.h>
#include <length_delay_calculation/length_delay_calculation.h>
#include <length_delay_calculation/length_delay_calculation_item.h>
#include <pad.h>
#include <pcb_edit_frame.h>
#include <pcb_painter.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <string_utils.h>
#include <eda_pattern_match.h>
#include <view/view.h>

#include <wx/textfile.h>

#include <algorithm>
#include <bitset>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <thread_pool.h>
#include <core/ki_assert.h>


/*****************************************************************************************
 *
 * Grid / model columns configuration
 *
 * ***************************************************************************************/


void PCB_NET_INSPECTOR_PANEL::buildColumns()
{
    m_columns.clear();

    // Set up the column display vector
    m_columns.emplace_back( 0u, UNDEFINED_LAYER, _( "Name" ), _( "Net Name" ), CSV_COLUMN_DESC::CSV_QUOTE, false );
    m_columns.emplace_back( 1u, UNDEFINED_LAYER, _( "Netclass" ), _( "Netclass" ), CSV_COLUMN_DESC::CSV_QUOTE, false );

    if( m_showTimeDomainDetails )
    {
        m_columns.emplace_back( 2u, UNDEFINED_LAYER, _( "Total Delay" ), _( "Net Delay" ), CSV_COLUMN_DESC::CSV_NONE,
                                true );
    }
    else
    {
        m_columns.emplace_back( 2u, UNDEFINED_LAYER, _( "Total Length" ), _( "Net Length" ), CSV_COLUMN_DESC::CSV_NONE,
                                true );
    }

    m_columns.emplace_back( 3u, UNDEFINED_LAYER, _( "Via Count" ), _( "Via Count" ), CSV_COLUMN_DESC::CSV_NONE, false );

    if( m_showTimeDomainDetails )
    {
        m_columns.emplace_back( 4u, UNDEFINED_LAYER, _( "Via Delay" ), _( "Via Delay" ), CSV_COLUMN_DESC::CSV_NONE,
                                true );
        m_columns.emplace_back( 5u, UNDEFINED_LAYER, _( "Track Delay" ), _( "Track Delay" ), CSV_COLUMN_DESC::CSV_NONE,
                                true );
        m_columns.emplace_back( 6u, UNDEFINED_LAYER, _( "Die Delay" ), _( "Die Delay" ), CSV_COLUMN_DESC::CSV_NONE,
                                true );
    }
    else
    {
        m_columns.emplace_back( 4u, UNDEFINED_LAYER, _( "Via Length" ), _( "Via Length" ), CSV_COLUMN_DESC::CSV_NONE,
                                true );
        m_columns.emplace_back( 5u, UNDEFINED_LAYER, _( "Track Length" ), _( "Track Length" ),
                                CSV_COLUMN_DESC::CSV_NONE, true );
        m_columns.emplace_back( 6u, UNDEFINED_LAYER, _( "Die Length" ), _( "Die Length" ), CSV_COLUMN_DESC::CSV_NONE,
                                true );
    }

    m_columns.emplace_back( 7u, UNDEFINED_LAYER, _( "Pad Count" ), _( "Pad Count" ), CSV_COLUMN_DESC::CSV_NONE, false );

    const int staticColumns = COLUMN_LAST_STATIC_COL + 1;

    // If we have not yet loaded the first board, use a dummy local settings object to ensure we
    // don't over-write existing board settings (note that PCB_EDIT_FRAME loads the local settings
    // object prior to loading the board; the two are not synced and we need to account for that)
    PANEL_NET_INSPECTOR_SETTINGS  tempCfg;
    PANEL_NET_INSPECTOR_SETTINGS* cfg = &tempCfg;

    if( m_boardLoaded )
    {
        PROJECT_LOCAL_SETTINGS& localSettings = Pgm().GetSettingsManager().Prj().GetLocalSettings();
        cfg = &localSettings.m_NetInspectorPanel;
    }

    // Reset the column display settings if column count doesn't match
    const int totalNumColumns = staticColumns + m_board->GetCopperLayerCount();

    if( (int) cfg->col_order.size() != totalNumColumns
        || (int) cfg->col_hidden.size() != totalNumColumns )
    {
        cfg->col_order.resize( totalNumColumns );
        cfg->col_hidden.resize( totalNumColumns );

        for( int i = 0; i < totalNumColumns; ++i )
        {
            cfg->col_order[i] = i;
            cfg->col_hidden[i] = false;
        }
    }

    // Check that all rows are unique to protect against corrupted settings data
    std::set<int> col_order_set( cfg->col_order.begin(), cfg->col_order.end() );

    if( col_order_set.size() != cfg->col_order.size() )
    {
        for( std::size_t i = 0; i < cfg->col_order.size(); ++i )
            cfg->col_order[i] = static_cast<int>( i );
    }

    // Add column records for copper layers
    for( PCB_LAYER_ID layer : m_board->GetEnabledLayers().Seq() )
    {
        if( !IsCopperLayer( layer ) )
            continue;

        m_columns.emplace_back( m_columns.size(), layer, m_board->GetLayerName( layer ), m_board->GetLayerName( layer ),
                                CSV_COLUMN_DESC::CSV_NONE, true );
    }

    // The columns of the view, in settings order
    viewBuildColumns( cfg );
}


wxString PCB_NET_INSPECTOR_PANEL::itemText( const LIST_ITEM* i, unsigned int aCol, bool aTimeDomain ) const
{
    if( aCol == COLUMN_NAME )
    {
        if( i->GetIsGroup() )
        {
            switch( i->GetGroupType() )
            {
            case LIST_ITEM::GROUP_TYPE::NETCLASS:     return _( "Netclass" ) + ": " + i->GetGroupName();
            case LIST_ITEM::GROUP_TYPE::USER_DEFINED: return _( "Custom" ) + ": " + i->GetGroupName();
            default:                                  return i->GetGroupName();
            }
        }

        return i->GetNetName();
    }
    else if( aCol == COLUMN_NETCLASS )
    {
        return i->GetNetclassName();
    }
    else if( aCol == COLUMN_PAD_COUNT )
    {
        return formatCount( i->GetPadCount() );
    }
    else if( aCol == COLUMN_VIA_COUNT )
    {
        return formatCount( i->GetViaCount() );
    }
    else if( aCol == COLUMN_VIA_LENGTH )
    {
        return aTimeDomain ? formatDelay( i->GetViaDelay() ) : formatLength( i->GetViaLength() );
    }
    else if( aCol == COLUMN_BOARD_LENGTH )
    {
        return aTimeDomain ? formatDelay( i->GetBoardWireDelay() ) : formatLength( i->GetBoardWireLength() );
    }
    else if( aCol == COLUMN_PAD_DIE_LENGTH )
    {
        return aTimeDomain ? formatDelay( i->GetPadDieDelay() ) : formatLength( i->GetPadDieLength() );
    }
    else if( aCol == COLUMN_TOTAL_LENGTH )
    {
        return aTimeDomain ? formatDelay( i->GetTotalDelay() ) : formatLength( i->GetTotalLength() );
    }
    else if( aCol > COLUMN_LAST_STATIC_COL && aCol < m_columns.size() )
    {
        if( aTimeDomain )
            return formatDelay( i->GetLayerWireDelay( m_columns[aCol].layer ) );
        else
            return formatLength( i->GetLayerWireLength( m_columns[aCol].layer ) );
    }

    return wxEmptyString;
}


int PCB_NET_INSPECTOR_PANEL::compareItems( const LIST_ITEM& i1, const LIST_ITEM& i2, unsigned int aCol,
                                           bool aAsc, bool aTimeDomain ) const
{
    auto compareUInt =
            []( int64_t aValue1, int64_t aValue2, bool aAscending )
            {
                if( aAscending )
                    return aValue1 < aValue2 ? -1 : 1;
                else
                    return aValue2 < aValue1 ? -1 : 1;
            };

    if( i1.GetIsGroup() && !i2.GetIsGroup() )
        return -1;

    if( i2.GetIsGroup() && !i1.GetIsGroup() )
        return 1;

    if( aCol == COLUMN_NAME )
    {
        const wxString& s1 = i1.GetNetName();
        const wxString& s2 = i2.GetNetName();

        return aAsc ? ValueStringCompare( s1, s2 ) : ValueStringCompare( s2, s1 );
    }
    else if( aCol == COLUMN_NETCLASS )
    {
        const wxString& s1 = i1.GetNetclassName();
        const wxString& s2 = i2.GetNetclassName();

        return aAsc ? ValueStringCompare( s1, s2 ) : ValueStringCompare( s2, s1 );
    }
    else if( aCol == COLUMN_PAD_COUNT && i1.GetPadCount() != i2.GetPadCount() )
    {
        return compareUInt( i1.GetPadCount(), i2.GetPadCount(), aAsc );
    }
    else if( aCol == COLUMN_VIA_COUNT && i1.GetViaCount() != i2.GetViaCount() )
    {
        return compareUInt( i1.GetViaCount(), i2.GetViaCount(), aAsc );
    }
    else if( aCol == COLUMN_VIA_LENGTH )
    {
        if( aTimeDomain && i1.GetViaDelay() != i2.GetViaDelay() )
            return compareUInt( i1.GetViaDelay(), i2.GetViaDelay(), aAsc );

        if( !aTimeDomain && i1.GetViaLength() != i2.GetViaLength() )
            return compareUInt( i1.GetViaLength(), i2.GetViaLength(), aAsc );
    }
    else if( aCol == COLUMN_BOARD_LENGTH )
    {
        if( aTimeDomain && i1.GetBoardWireDelay() != i2.GetBoardWireDelay() )
            return compareUInt( i1.GetBoardWireDelay(), i2.GetBoardWireDelay(), aAsc );

        if( !aTimeDomain && i1.GetBoardWireLength() != i2.GetBoardWireLength() )
            return compareUInt( i1.GetBoardWireLength(), i2.GetBoardWireLength(), aAsc );
    }
    else if( aCol == COLUMN_PAD_DIE_LENGTH )
    {
        if( aTimeDomain && i1.GetPadDieDelay() != i2.GetPadDieDelay() )
            return compareUInt( i1.GetPadDieDelay(), i2.GetPadDieDelay(), aAsc );

        if( !aTimeDomain && i1.GetPadDieLength() != i2.GetPadDieLength() )
            return compareUInt( i1.GetPadDieLength(), i2.GetPadDieLength(), aAsc );
    }
    else if( aCol == COLUMN_TOTAL_LENGTH )
    {
        if( aTimeDomain && i1.GetTotalDelay() != i2.GetTotalDelay() )
            return compareUInt( i1.GetTotalDelay(), i2.GetTotalDelay(), aAsc );

        if( !aTimeDomain && i1.GetTotalLength() != i2.GetTotalLength() )
            return compareUInt( i1.GetTotalLength(), i2.GetTotalLength(), aAsc );
    }
    else if( aCol > COLUMN_LAST_STATIC_COL && aCol < m_columns.size() )
    {
        PCB_LAYER_ID layer = m_columns[aCol].layer;

        if( aTimeDomain && i1.GetLayerWireDelay( layer ) != i2.GetLayerWireDelay( layer ) )
            return compareUInt( i1.GetLayerWireDelay( layer ), i2.GetLayerWireDelay( layer ), aAsc );

        if( !aTimeDomain && i1.GetLayerWireLength( layer ) != i2.GetLayerWireLength( layer ) )
            return compareUInt( i1.GetLayerWireLength( layer ), i2.GetLayerWireLength( layer ), aAsc );
    }

    return 0;
}


/*****************************************************************************************
 *
 * Nets list generation
 *
 * ***************************************************************************************/


void PCB_NET_INSPECTOR_PANEL::readFilterSettings( const PANEL_NET_INSPECTOR_SETTINGS* aCfg )
{
    m_filterByNetName = aCfg->filter_by_net_name;
    m_filterByNetclass = aCfg->filter_by_netclass;
    m_showZeroPadNets = aCfg->show_zero_pad_nets;
    m_showTimeDomainDetails = aCfg->show_time_domain_details;
    m_groupByNetclass = aCfg->group_by_netclass;
    m_groupByConstraint = aCfg->group_by_constraint;
}


void PCB_NET_INSPECTOR_PANEL::fillDataModel( const PANEL_NET_INSPECTOR_SETTINGS* aCfg )
{
    m_dataModel->deleteAllItems();
    m_custom_group_rules.clear();

    for( const wxString& rule : aCfg->custom_group_rules )
        m_custom_group_rules.push_back( std::make_unique<EDA_COMBINED_MATCHER>( rule, CTX_NET ) );

    m_dataModel->addCustomGroups();

    std::vector<NETINFO_ITEM*> netCodes;

    for( NETINFO_ITEM* ni : m_board->GetNetInfo() )
    {
        if( netFilterMatches( ni, const_cast<PANEL_NET_INSPECTOR_SETTINGS*>( aCfg ) ) )
            netCodes.emplace_back( ni );
    }

    std::ranges::sort( netCodes,
                       []( const NETINFO_ITEM* a, const NETINFO_ITEM* b )
                       {
                           return a->GetNetCode() < b->GetNetCode();
                       } );

    std::vector<std::unique_ptr<LIST_ITEM>> lengths = calculateNets( netCodes, m_showZeroPadNets );

    m_dataModel->addItems( lengths );
}


bool PCB_NET_INSPECTOR_PANEL::netFilterMatches( NETINFO_ITEM*                 aNet,
                                                PANEL_NET_INSPECTOR_SETTINGS* cfg ) const
{
    if( cfg == nullptr )
    {
        PROJECT_LOCAL_SETTINGS& localSettings = Pgm().GetSettingsManager().Prj().GetLocalSettings();
        cfg = &localSettings.m_NetInspectorPanel;
    }

    // Never show an unconnected net
    if( aNet->GetNetCode() <= 0 )
        return false;

    const wxString  filterString = UnescapeString( searchText() ).Upper();
    const wxString  netName = UnescapeString( aNet->GetNetname() ).Upper();
    const NETCLASS* netClass = aNet->GetNetClass();
    const wxString  netClassName = UnescapeString( netClass->GetName() ).Upper();

    bool matched = false;

    // No filter - match all
    if( filterString.Length() == 0 )
        matched = true;

    // Search on net class
    if( !matched && cfg->filter_by_netclass && netClassName.Find( filterString ) != wxNOT_FOUND )
        matched = true;

    // Search on net name
    if( !matched && cfg->filter_by_net_name && netName.Find( filterString ) != wxNOT_FOUND )
        matched = true;

    // Remove unconnected nets if required
    if( matched )
    {
        if( !m_showUnconnectedNets )
            matched = !netName.StartsWith( wxT( "UNCONNECTED-(" ) );
    }

    return matched;
}


struct NETCODE_CMP_LESS
{
    bool operator()( const CN_ITEM* a, const CN_ITEM* b ) const { return a->Net() < b->Net(); }

    bool operator()( const CN_ITEM* a, int b ) const { return a->Net() < b; }

    bool operator()( int a, const CN_ITEM* b ) const { return a < b->Net(); }
};


std::vector<CN_ITEM*> PCB_NET_INSPECTOR_PANEL::relevantConnectivityItems() const
{
    // Pre-filter the connectivity items and sort them by netcode. This avoids quadratic runtime when building the whole
    // net list.
    const auto type_bits = std::bitset<MAX_STRUCT_TYPE_ID>().set( PCB_TRACE_T )
                                                            .set( PCB_ARC_T )
                                                            .set( PCB_VIA_T )
                                                            .set( PCB_PAD_T );

    std::vector<CN_ITEM*> cn_items;
    cn_items.reserve( 1024 );

    for( CN_ITEM* cn_item : m_board->GetConnectivity()->GetConnectivityAlgo()->ItemList() )
    {
        if( cn_item->Valid() && type_bits[cn_item->Parent()->Type()] )
            cn_items.push_back( cn_item );
    }

    std::ranges::sort( cn_items, NETCODE_CMP_LESS() );

    return cn_items;
}


std::vector<std::unique_ptr<PCB_NET_INSPECTOR_PANEL::LIST_ITEM>>
PCB_NET_INSPECTOR_PANEL::calculateNets( const std::vector<NETINFO_ITEM*>& aNetCodes, bool aIncludeZeroPadNets ) const
{
    std::vector<std::unique_ptr<LIST_ITEM>> results;

    LENGTH_DELAY_CALCULATION*   calc = m_board->GetLengthCalculation();
    const std::vector<CN_ITEM*> conItems = relevantConnectivityItems();

    // First assemble the LENGTH_CALCULATION_ITEMs for board items which match the nets we need to recompute
    // Precondition: conItems and aNetCodes are sorted in increasing netcode value
    // Functionality: This extracts any items from conItems which have a netcode which is present in aNetCodes
    std::unordered_map<int, std::vector<LENGTH_DELAY_CALCULATION_ITEM>> netItemsMap;
    std::vector<NETINFO_ITEM*>                                          foundNets;

    auto itemItr = conItems.begin();
    auto netCodeItr = aNetCodes.begin();

    while( itemItr != conItems.end() && netCodeItr != aNetCodes.end() )
    {
        const int curNetCode = ( *netCodeItr )->GetNetCode();
        const int curItemNetCode = ( *itemItr )->Net();

        if( curItemNetCode == curNetCode )
        {
            if( foundNets.empty() || foundNets.back() != *netCodeItr )
                foundNets.emplace_back( *netCodeItr );

            // Take the item
            LENGTH_DELAY_CALCULATION_ITEM lengthItem = calc->GetLengthCalculationItem( ( *itemItr )->Parent() );
            netItemsMap[curItemNetCode].emplace_back( std::move( lengthItem ) );
            ++itemItr;
        }
        else if( curItemNetCode < curNetCode )
        {
            // Fast-forward through items
            while( itemItr != conItems.end() && ( *itemItr )->Net() < curNetCode )
                ++itemItr;
        }
        else if( curItemNetCode > curNetCode )
        {
            // Fast-forward through required net codes
            while( netCodeItr != aNetCodes.end() && curItemNetCode > ( *netCodeItr )->GetNetCode() )
                ++netCodeItr;
        }
    }

    // Now calculate the length statistics for each net. This includes potentially expensive path optimisations, so
    // parallelize this work.
    std::mutex   resultsMutex;
    thread_pool& tp = GetKiCadThreadPool();

    auto resultsFuture = tp.submit_loop(
            0, foundNets.size(),
            [&, this, calc]( const int i )
            {
                int netCode = foundNets[i]->GetNetCode();

                constexpr PATH_OPTIMISATIONS opts = { .OptimiseVias = true,
                                                      .MergeTracks = true,
                                                      .OptimiseTracesInPads = true,
                                                      .InferViaInPad = false };

                LENGTH_DELAY_STATS lengthDetails = calc->CalculateLengthDetails(
                                        netItemsMap[netCode],
                                        opts,
                                        nullptr,
                                        nullptr,
                                        LENGTH_DELAY_LAYER_OPT::WITH_LAYER_DETAIL,
                                        m_showTimeDomainDetails ? LENGTH_DELAY_DOMAIN_OPT::WITH_DELAY_DETAIL
                                                                : LENGTH_DELAY_DOMAIN_OPT::NO_DELAY_DETAIL );

                if( aIncludeZeroPadNets || lengthDetails.NumPads > 0 )
                {
                    std::unique_ptr<LIST_ITEM> new_item = std::make_unique<LIST_ITEM>( foundNets[i] );

                    new_item->SetPadCount( lengthDetails.NumPads );
                    new_item->SetLayerCount( m_board->GetCopperLayerCount() );
                    new_item->SetPadDieLength( lengthDetails.PadToDieLength );
                    new_item->SetPadDieDelay( lengthDetails.PadToDieDelay );
                    new_item->SetViaCount( lengthDetails.NumVias );
                    new_item->SetViaLength( lengthDetails.ViaLength );
                    new_item->SetViaDelay( lengthDetails.ViaDelay );
                    new_item->SetLayerWireLengths( *lengthDetails.LayerLengths );

                    if( m_showTimeDomainDetails )
                        new_item->SetLayerWireDelays( *lengthDetails.LayerDelays );

                    std::scoped_lock lock( resultsMutex );
                    results.emplace_back( std::move( new_item ) );
                }
            } );

    resultsFuture.get();

    return results;
}


/*****************************************************************************************
 *
 * Formatting helpers
 *
 * ***************************************************************************************/


wxString PCB_NET_INSPECTOR_PANEL::formatNetCode( const NETINFO_ITEM* aNet )
{
    return wxString::Format( wxT( "%.3d" ), aNet->GetNetCode() );
}


wxString PCB_NET_INSPECTOR_PANEL::formatNetName( const NETINFO_ITEM* aNet )
{
    return UnescapeString( aNet->GetNetname() );
}


wxString PCB_NET_INSPECTOR_PANEL::formatCount( const unsigned int aValue )
{
    return wxString::Format( wxT( "%u" ), aValue );
}


wxString PCB_NET_INSPECTOR_PANEL::formatLength( const int64_t aValue ) const
{
    return m_frame->MessageTextFromValue( aValue,
                                          // don't include unit label in the string when reporting
                                          !m_inReporting );
}

wxString PCB_NET_INSPECTOR_PANEL::formatDelay( const int64_t aValue ) const
{
    return m_frame->MessageTextFromValue( aValue,
                                          // don't include unit label in the string when reporting
                                          !m_inReporting, EDA_DATA_TYPE::TIME );
}


void PCB_NET_INSPECTOR_PANEL::OnBoardChanged()
{
    m_board = m_frame->GetBoard();

    if( m_board )
        m_board->AddListener( this );

    m_boardLoaded = true;
    m_boardLoading = true;

    const PROJECT_LOCAL_SETTINGS& localSettings = Pgm().GetSettingsManager().Prj().GetLocalSettings();
    auto&                   cfg = localSettings.m_NetInspectorPanel;
    setSearchText( cfg.filter_text );

    buildNetsList( true );

    m_boardLoading = false;
}


void PCB_NET_INSPECTOR_PANEL::OnBoardItemAdded( BOARD& aBoard, BOARD_ITEM* aBoardItem )
{
    const std::vector<BOARD_ITEM*> item{ aBoardItem };
    updateBoardItems( item );
}


void PCB_NET_INSPECTOR_PANEL::OnBoardItemsAdded( BOARD& aBoard, std::vector<BOARD_ITEM*>& aBoardItems )
{
    updateBoardItems( aBoardItems );
}


void PCB_NET_INSPECTOR_PANEL::updateBoardItems( const std::vector<BOARD_ITEM*>& aBoardItems )
{
    if( !isPanelShown() )
        return;

    // Rebuild full list for large changes
    if( aBoardItems.size()
        > static_cast<size_t>( ADVANCED_CFG::GetCfg().m_NetInspectorBulkUpdateOptimisationThreshold ) )
    {
        buildNetsList();
    }
    else
    {
        std::vector<NETINFO_ITEM*> changedNets;

        for( BOARD_ITEM* boardItem : aBoardItems )
        {
            if( NETINFO_ITEM* net = dynamic_cast<NETINFO_ITEM*>( boardItem ) )
            {
                // A new net has been added to the board. Add it to our list if it passes the netname filter test.
                if( netFilterMatches( net ) )
                    changedNets.emplace_back( net );
            }
            else if( BOARD_CONNECTED_ITEM* i = dynamic_cast<BOARD_CONNECTED_ITEM*>( boardItem ) )
            {
                changedNets.emplace_back( i->GetNet() );
            }
            else if( FOOTPRINT* footprint = dynamic_cast<FOOTPRINT*>( boardItem ) )
            {
                for( const PAD* pad : footprint->Pads() )
                {
                    if( netFilterMatches( pad->GetNet() ) )
                        changedNets.emplace_back( pad->GetNet() );
                }
            }
        }

        std::ranges::sort( changedNets,
                           []( const NETINFO_ITEM* a, const NETINFO_ITEM* b )
                           {
                               return a->GetNetCode() < b->GetNetCode();
                           } );

        updateNets( changedNets );
    }

    refreshList();
}


void PCB_NET_INSPECTOR_PANEL::updateNets( const std::vector<NETINFO_ITEM*>& aNets ) const
{
    std::vector<NETINFO_ITEM*>        netsToUpdate;
    std::unordered_set<NETINFO_ITEM*> netsToDelete;

    for( NETINFO_ITEM* net : aNets )
    {
        // Add all nets to the deletion list - we will prune this later to only contain unhandled nets
        netsToDelete.insert( net );

        // Only calculate nets that match the current filter
        if( netFilterMatches( net ) )
            netsToUpdate.emplace_back( net );
    }

    freezeList();

    std::vector<std::unique_ptr<LIST_ITEM>> newListItems = calculateNets( aNets, true );

    for( std::unique_ptr<LIST_ITEM>& newListItem : newListItems )
    {
        // Remove the handled net from the deletion list
        netsToDelete.erase( newListItem->GetNet() );

        std::optional<LIST_ITEM_ITER> curNetRow = m_dataModel->findItem( newListItem->GetNetCode() );

        if( !m_showZeroPadNets && newListItem->GetPadCount() == 0 )
        {
            m_dataModel->deleteItem( curNetRow );
            continue;
        }

        if( !curNetRow )
        {
            m_dataModel->addItem( std::move( newListItem ) );
            continue;
        }

        const std::unique_ptr<LIST_ITEM>& curListItem = *curNetRow.value();

        if( curListItem->GetNetName() != newListItem->GetNetName() )
        {
            // If the name has changed, it might require re-grouping. It's easier to remove and re-insert it.
            m_dataModel->deleteItem( curNetRow );
            m_dataModel->addItem( std::move( newListItem ) );
        }
        else
        {
            curListItem->SetPadCount( newListItem->GetPadCount() );
            curListItem->SetPadDieLength( newListItem->GetPadDieLength() );
            curListItem->SetPadDieDelay( newListItem->GetPadDieDelay() );
            curListItem->SetViaCount( newListItem->GetViaCount() );
            curListItem->SetViaLength( newListItem->GetViaLength() );
            curListItem->SetViaDelay( newListItem->GetViaDelay() );
            curListItem->SetLayerWireLengths( newListItem->GetLayerWireLengths() );

            if( m_showTimeDomainDetails )
                curListItem->SetLayerWireDelays( newListItem->GetLayerWireDelays() );

            updateDisplayedRowValues( curNetRow );
        }
    }

    // Delete any nets we have not yet handled
    for( const NETINFO_ITEM* netToDelete : netsToDelete )
        m_dataModel->deleteItem( m_dataModel->findItem( netToDelete->GetNetCode() ) );

    thawList();
}


void PCB_NET_INSPECTOR_PANEL::OnBoardItemRemoved( BOARD& aBoard, BOARD_ITEM* aBoardItem )
{
    const std::vector<BOARD_ITEM*> item{ aBoardItem };
    updateBoardItems( item );
}


void PCB_NET_INSPECTOR_PANEL::OnBoardItemsRemoved( BOARD& aBoard, std::vector<BOARD_ITEM*>& aBoardItems )
{
    updateBoardItems( aBoardItems );
}


void PCB_NET_INSPECTOR_PANEL::OnBoardNetSettingsChanged( BOARD& aBoard )
{
    if( !isPanelShown() )
        return;

    buildNetsList();
}


void PCB_NET_INSPECTOR_PANEL::OnBoardItemChanged( BOARD& aBoard, BOARD_ITEM* aBoardItem )
{
    const std::vector<BOARD_ITEM*> item{ aBoardItem };
    updateBoardItems( item );
}


void PCB_NET_INSPECTOR_PANEL::OnBoardItemsChanged( BOARD& aBoard, std::vector<BOARD_ITEM*>& aBoardItems )
{
    updateBoardItems( aBoardItems );
}


void PCB_NET_INSPECTOR_PANEL::OnBoardCompositeUpdate( BOARD&                    aBoard,
                                                      std::vector<BOARD_ITEM*>& aAddedItems,
                                                      std::vector<BOARD_ITEM*>& aRemovedItems,
                                                      std::vector<BOARD_ITEM*>& aChangedItems )
{
    if( !isPanelShown() )
        return;

    std::vector<BOARD_ITEM*> allItems{ aAddedItems.begin(), aAddedItems.end() };
    allItems.insert( allItems.end(), aRemovedItems.begin(), aRemovedItems.end() );
    allItems.insert( allItems.end(), aChangedItems.begin(), aChangedItems.end() );
    updateBoardItems( allItems );
}


void PCB_NET_INSPECTOR_PANEL::OnShowPanel()
{
    buildNetsList();
    OnBoardHighlightNetChanged( *m_board );
}


void PCB_NET_INSPECTOR_PANEL::onClearHighlighting()
{
    m_highlightingNets = true;

    m_frame->GetCanvas()->GetView()->GetPainter()->GetSettings()->SetHighlight( false );
    m_frame->GetCanvas()->GetView()->UpdateAllLayersColor();
    m_frame->GetCanvas()->Refresh();

    m_highlightingNets = false;
}


void PCB_NET_INSPECTOR_PANEL::OnParentSetupChanged()
{
    // Rebuilt the nets list, and force rebuild of columns in case the stackup has changed
    buildNetsList( true );
}


void PCB_NET_INSPECTOR_PANEL::onRenameSelectedNet()
{
    std::vector<const LIST_ITEM*> selection = selectedItems();

    if( selection.size() == 1 )
    {
        const LIST_ITEM* sel = selection.front();

        if( sel->GetIsGroup() )
            return;

        NETINFO_ITEM* net = sel->GetNet();
        wxString      fullNetName = net->GetNetname();
        wxString      netPath;
        wxString      shortNetName;

        if( fullNetName.Contains( wxT( "/" ) ) )
        {
            netPath = fullNetName.BeforeLast( '/' ) + '/';
            shortNetName = fullNetName.AfterLast( '/' );
        }
        else
        {
            shortNetName = fullNetName;
        }

        wxString unescapedShortName = UnescapeString( shortNetName );

        wxString value = unescapedShortName;

        while( true )
        {
            if( !askText( _( "Rename Net" ), _( "Net name:" ), value ) || value == unescapedShortName )
                return;

            unescapedShortName = value;

            if( unescapedShortName.IsEmpty() )
            {
                DisplayError( this, _( "Net name cannot be empty." ) );
                continue;
            }

            shortNetName = EscapeString( unescapedShortName, CTX_NETNAME );
            fullNetName = netPath + shortNetName;

            if( m_board->FindNet( shortNetName ) || m_board->FindNet( fullNetName ) )
            {
                DisplayError( this, wxString::Format( _( "Net name '%s' is already in use." ), unescapedShortName ) );
                unescapedShortName = wxEmptyString;
            }
            else
            {
                break;
            }
        }

        for( BOARD_CONNECTED_ITEM* boardItem : m_frame->GetBoard()->AllConnectedItems() )
        {
            if( boardItem->GetNet() == net )
                boardItem->SetFlags( CANDIDATE );
            else
                boardItem->ClearFlags( CANDIDATE );
        }

        // the changed name might require re-grouping.  remove/re-insert is easier.
        auto removed_item = m_dataModel->deleteItem( m_dataModel->findItem( net ) );

        m_board->Remove( net );
        net->SetNetname( fullNetName );
        m_board->Add( net );

        for( BOARD_CONNECTED_ITEM* boardItem : m_frame->GetBoard()->AllConnectedItems() )
        {
            if( boardItem->GetFlags() & CANDIDATE )
                boardItem->SetNet( net );
        }

        buildNetsList();

        if( std::optional<LIST_ITEM_ITER> r = m_dataModel->findItem( net ) )
            selectItems( { r.value()->get() } );

        m_frame->OnModify();

        // Currently only tracks and pads have netname annotations and need to be redrawn,
        // but zones are likely to follow.  Since we don't have a way to ask what is current,
        // just refresh all items.
        m_frame->GetCanvas()->GetView()->UpdateAllItems( KIGFX::REPAINT );
        m_frame->GetCanvas()->Refresh();
    }
}


void PCB_NET_INSPECTOR_PANEL::onDeleteSelectedNet()
{
    std::vector<const LIST_ITEM*> sel = selectedItems();

    if( sel.empty() )
        return;

    auto delete_one = [this]( const LIST_ITEM* i )
    {
        if( i->GetPadCount() == 0
            || IsOK( this, wxString::Format( _( "Net '%s' is in use.  Delete anyway?" ), i->GetNetName() ) ) )
        {
            // This is a bit hacky, but it will do for now, since this is the only path
            // outside the netlist updater where you can remove a net from a BOARD.
            int removedCode = i->GetNetCode();

            m_frame->GetCanvas()->GetView()->UpdateAllItemsConditionally(
                    [removedCode]( KIGFX::VIEW_ITEM* aItem ) -> int
                    {
                        auto boardItem = dynamic_cast<BOARD_CONNECTED_ITEM*>( aItem );

                        if( boardItem && boardItem->GetNetCode() == removedCode )
                            return KIGFX::REPAINT;

                        EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( aItem );

                        if( text && text->HasTextVars() )
                        {
                            text->ClearRenderCache();
                            text->ClearBoundingBoxCache();
                            return KIGFX::GEOMETRY | KIGFX::REPAINT;
                        }

                        return 0;
                    } );

            m_board->Remove( i->GetNet() );
            m_frame->OnModify();

            // We'll get an OnBoardItemRemoved callback from this to update our listbox
        }
    };

    for( const LIST_ITEM* ii : sel )
    {

        if( ii->GetIsGroup() )
        {
            if( ii->ChildrenCount() != 0
                && IsOK( this, wxString::Format( _( "Delete all nets in group '%s'?" ), ii->GetGroupName() ) ) )
            {
                // we can't be iterating the children container and deleting items from
                // it at the same time.  thus take a copy of it first.
                std::vector<const LIST_ITEM*> children;
                children.reserve( ii->ChildrenCount() );
                std::copy( ii->ChildrenBegin(), ii->ChildrenEnd(), std::back_inserter( children ) );

                for( const LIST_ITEM* c : children )
                    delete_one( c );
            }
        }
        else
        {
            delete_one( ii );
        }
    }
}


void PCB_NET_INSPECTOR_PANEL::OnLanguageChangedImpl()
{
    SaveSettings();
    buildNetsList( true );
    m_dataModel->updateAllItems();
}

void PCB_NET_INSPECTOR_PANEL::OnBoardHighlightNetChanged( BOARD& aBoard )
{
    if( m_highlightingNets || !isPanelShown() )
        return;

    std::vector<const LIST_ITEM*> new_selection;

    if( m_board->IsHighLightNetON() )
    {
        const std::set<int>& selected_codes = m_board->GetHighLightNetCodes();

        new_selection.reserve( selected_codes.size() );

        for( const int code : selected_codes )
        {
            if( std::optional<LIST_ITEM_ITER> r = m_dataModel->findItem( code ) )
                new_selection.push_back( &***r );
        }
    }

    selectItems( new_selection );
}


/*****************************************************************************************
 *
 * UI-generated event handling
 *
 * ***************************************************************************************/


void PCB_NET_INSPECTOR_PANEL::onAddGroup()
{
    wxString newGroupName;

    if( !askText( _( "New Group" ), _( "Group name / pattern:" ), newGroupName,
                  _( "(Use /.../ to indicate a regular expression.)" ) )
        || newGroupName.IsEmpty() )
    {
        return; //Aborted by user
    }

    newGroupName = UnescapeString( newGroupName );

    if( newGroupName == "" )
        return;

    if( std::ranges::find_if( m_custom_group_rules,
                              [&]( std::unique_ptr<EDA_COMBINED_MATCHER>& rule )
                              {
                                  return rule->GetPattern() == newGroupName;
                              } )
        == m_custom_group_rules.end() )
    {
        m_custom_group_rules.push_back( std::make_unique<EDA_COMBINED_MATCHER>( newGroupName, CTX_NET ) );
        SaveSettings();
    }

    buildNetsList();
}


void PCB_NET_INSPECTOR_PANEL::doContextMenuCommand( int aId )
{
    bool saveAndRebuild = true;

    switch( aId )
    {
    case ID_ADD_NET:
        onAddNet();
        break;

    case ID_RENAME_NET:
        onRenameSelectedNet();
        break;

    case ID_DELETE_NET:
        onDeleteSelectedNet();
        break;

    case ID_ADD_GROUP:
        onAddGroup();
        break;

    case ID_GROUP_BY_CONSTRAINT:
        m_groupByConstraint = !m_groupByConstraint;
        break;

    case ID_GROUP_BY_NETCLASS:
        m_groupByNetclass = !m_groupByNetclass;
        break;

    case ID_FILTER_BY_NET_NAME:
        m_filterByNetName = !m_filterByNetName;
        break;

    case ID_FILTER_BY_NETCLASS:
        m_filterByNetclass = !m_filterByNetclass;
        break;

    case ID_REMOVE_SELECTED_GROUP:
        onRemoveSelectedGroup();
        break;

    case ID_REMOVE_GROUPS:
        m_custom_group_rules.clear();
        break;

    case ID_SHOW_ZERO_NET_PADS:
        m_showZeroPadNets = !m_showZeroPadNets;
        break;

    case ID_SHOW_UNCONNECTED_NETS:
        m_showUnconnectedNets = !m_showUnconnectedNets;
        break;

    case ID_SHOW_TIME_DOMAIN_DETAILS:
        m_showTimeDomainDetails = !m_showTimeDomainDetails;
        break;

    case ID_GENERATE_REPORT:
        generateReport();
        saveAndRebuild = false;
        break;

    case ID_HIGHLIGHT_SELECTED_NETS:
        highlightSelectedNets();
        saveAndRebuild = false;
        break;

    case ID_CLEAR_HIGHLIGHTING:
        onClearHighlighting();
        saveAndRebuild = false;
        break;

    case ID_AUTOFIT_COLUMN:
    case ID_AUTOFIT_ALL_COLUMNS:
        viewColumnCommand( aId );
        saveAndRebuild = false;
        break;

    default:
        if( aId >= ID_HIDE_COLUMN )
            viewColumnCommand( aId );

        break;
    }

    if( saveAndRebuild )
    {
        SaveSettings();
        buildNetsList();
    }
}


void PCB_NET_INSPECTOR_PANEL::onRemoveSelectedGroup()
{
    std::vector<const LIST_ITEM*> selection = selectedItems();

    if( selection.size() != 1 )
        return;

    const LIST_ITEM* selItem = selection.front();

    if( selItem->GetIsGroup() )
    {
        const wxString groupName = selItem->GetGroupName();
        const auto     groupIter = std::ranges::find_if( m_custom_group_rules,
                                                         [&]( std::unique_ptr<EDA_COMBINED_MATCHER>& rule )
                                                         {
                                                             return rule->GetPattern() == groupName;
                                                         } );

        if( groupIter != m_custom_group_rules.end() )
        {
            m_custom_group_rules.erase( groupIter );
            SaveSettings();
            buildNetsList();
        }
    }
}


void PCB_NET_INSPECTOR_PANEL::generateReport()
{
    wxString path = chooseReportFile();

    if( path.IsEmpty() )
        return;

    wxTextFile f( path );

    f.Create();

    wxString txt;

    m_inReporting = true;

    // Print Header:
    for( auto&& col : m_columns )
    {
        txt += '"';

        if( col.has_units )
        {
            txt += wxString::Format( _( "%s (%s)" ),
                                     col.csv_name,
                                     EDA_UNIT_UTILS::GetLabel( m_frame->GetUserUnits() ) );
        }
        else
        {
            txt += col.csv_name;
        }

        txt += wxT( "\";" );
    }

    f.AddLine( txt );

    // Print list of nets:
    const unsigned int num_rows = m_dataModel->itemCount();

    for( unsigned int row = 0; row < num_rows; row++ )
    {
        auto& i = m_dataModel->itemAt( row );

        if( i.GetIsGroup() || i.GetNetCode() == 0 )
            continue;

        txt = "";

        for( auto&& col : m_columns )
        {
            wxString value = itemText( &i, col.num, m_showTimeDomainDetails );

            if( static_cast<int>( col.csv_flags ) & static_cast<int>( CSV_COLUMN_DESC::CSV_QUOTE ) )
                txt += '"' + value + wxT( "\";" );
            else
                txt += value + ';';
        }

        f.AddLine( txt );
    }

    m_inReporting = false;

    f.Write();
    f.Close();
}


void PCB_NET_INSPECTOR_PANEL::highlightSelectedNets()
{
    // ignore selection changes while the whole list is being rebuilt.
    if( m_inBuildNetsList )
        return;

    m_highlightingNets = true;

    RENDER_SETTINGS*              renderSettings = m_frame->GetCanvas()->GetView()->GetPainter()->GetSettings();
    std::vector<const LIST_ITEM*> sel = selectedItems();

    renderSettings->SetHighlight( false );

    for( const LIST_ITEM* ii : sel )
    {
        if( ii->GetIsGroup() )
        {
            for( auto c = ii->ChildrenBegin(), end = ii->ChildrenEnd(); c != end; ++c )
                renderSettings->SetHighlight( true, ( *c )->GetNetCode(), true );
        }
        else
        {
            renderSettings->SetHighlight( true, ii->GetNetCode(), true );
        }
    }

    m_frame->GetCanvas()->GetView()->UpdateAllLayersColor();
    m_frame->GetCanvas()->Refresh();

    m_highlightingNets = false;
}


void PCB_NET_INSPECTOR_PANEL::onAddNet()
{
    wxString newNetName;

    while( true )
    {
        if( !askText( _( "New Net" ), _( "Net name:" ), newNetName ) || newNetName.IsEmpty() )
            return; //Aborted by user

        if( m_board->FindNet( newNetName ) )
        {
            DisplayError( this, wxString::Format( _( "Net name '%s' is already in use." ), newNetName ) );
        }
        else
        {
            break;
        }
    }

    NETINFO_ITEM* newnet = new NETINFO_ITEM( m_board, newNetName, 0 );

    m_board->Add( newnet );

    // We'll get an OnBoardItemAdded callback from this to update our listbox
    m_frame->OnModify();
}


/*****************************************************************************************
 *
 * Settings persistence
 *
 * ***************************************************************************************/


void PCB_NET_INSPECTOR_PANEL::SaveSettings()
{
    // Don't save settings if a board has not yet been loaded or the panel hasn't been displayed.
    // Events fire while we set up the panel which overwrite the settings we haven't yet loaded.
    if( !isViewDisplayed() || !m_boardLoaded || m_boardLoading )
        return;

    PROJECT_LOCAL_SETTINGS& localSettings = Pgm().GetSettingsManager().Prj().GetLocalSettings();
    auto&                   cfg = localSettings.m_NetInspectorPanel;

    // User-defined filters / grouping
    cfg.filter_text = searchText();
    cfg.filter_by_net_name = m_filterByNetName;
    cfg.filter_by_netclass = m_filterByNetclass;
    cfg.group_by_netclass = m_groupByNetclass;
    cfg.group_by_constraint = m_groupByConstraint;
    cfg.show_zero_pad_nets = m_showZeroPadNets;
    cfg.show_unconnected_nets = m_showUnconnectedNets;
    cfg.show_time_domain_details = m_showTimeDomainDetails;

    // Grid sorting, column arrangement / sizes, expanded rows
    saveViewSettings( cfg );

    // Customer group rules
    cfg.custom_group_rules.clear();

    for( const std::unique_ptr<EDA_COMBINED_MATCHER>& rule : m_custom_group_rules )
        cfg.custom_group_rules.push_back( rule->GetPattern() );
}
