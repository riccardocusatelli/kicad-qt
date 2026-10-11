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
 * @file pcb_net_inspector_list_item.h
 * The entries of the Net Inspector list (the nets and their groups), shared by the data models of
 * the wxWidgets and Qt builds (pcb_net_inspector_panel_data_model.h).
 */

#ifndef PCB_NET_INSPECTOR_LIST_ITEM_H
#define PCB_NET_INSPECTOR_LIST_ITEM_H

#include <algorithm>
#include <map>
#include <vector>

#include <eda_pattern_match.h>
#include <netinfo.h>
#include <netclass.h>
#include <string_utils.h>
#include <core/ki_assert.h>


/**
 * Primary data item for entries in the Net Inspector list.
 *
 * This class tracks all data for a given net entry in the net inspector list.
 */
class PCB_NET_INSPECTOR_PANEL::LIST_ITEM
{
public:
    enum class GROUP_TYPE
    {
        NONE,
        USER_DEFINED,
        NETCLASS
    };

    LIST_ITEM( unsigned int aGroupNumber, const wxString& aGroupName, GROUP_TYPE aGroupType ) :
            m_group_type( aGroupType ),
            m_group_number( aGroupNumber ),
            m_net_name( aGroupName )
    {
        m_group_name = aGroupName;
        m_column_changed.resize( COLUMN_LAST_STATIC_COL + 1 + 2, 0 ); // 2 for default layer count
    }

    LIST_ITEM( NETINFO_ITEM* aNet ) :
            m_group_type( GROUP_TYPE::NONE ),
            m_net( aNet )
    {
        KI_ASSERT( aNet );
        m_net_name = UnescapeString( aNet->GetNetname() );
        m_net_class = UnescapeString( aNet->GetNetClass()->GetHumanReadableName() );
        m_column_changed.resize( COLUMN_LAST_STATIC_COL + 1 + 2, 0 );
    }

    LIST_ITEM()
    {
        m_column_changed.resize( COLUMN_LAST_STATIC_COL + 1 + 2, 0 );
    }

    LIST_ITEM& operator=( const LIST_ITEM& ) = delete;

    bool            GetIsGroup() const { return m_group_type != GROUP_TYPE::NONE; }
    const wxString& GetGroupName() const { return m_group_name; }
    GROUP_TYPE      GetGroupType() const { return m_group_type; }
    int             GetGroupNumber() const { return m_group_number; }

    auto         ChildrenBegin() const { return m_children.begin(); }
    auto         ChildrenEnd() const { return m_children.end(); }
    unsigned int ChildrenCount() const { return m_children.size(); }

    void SetLayerCount( unsigned int aValue )
    {
        m_column_changed.resize( COLUMN_LAST_STATIC_COL + 1 + aValue, 0 );
    }

    NETINFO_ITEM* GetNet() const { return m_net; }

    int GetNetCode() const
    {
        return GetIsGroup() ? ( 0 - int( m_group_number ) - 1 ) : m_net->GetNetCode();
    }

    const wxString& GetNetName() const { return m_net_name; }
    const wxString& GetNetclassName() const { return m_net_class; }

    void ResetColumnChangedBits()
    {
        std::fill( m_column_changed.begin(), m_column_changed.end(), 0 );
    }

    unsigned int GetPadCount() const { return m_pad_count; }

    bool PadCountChanged() const { return m_column_changed[COLUMN_PAD_COUNT]; }

    void SetPadCount( unsigned int aValue )
    {
        if( m_parent )
            m_parent->SetPadCount( m_parent->GetPadCount() - m_pad_count + aValue );

        m_column_changed[COLUMN_PAD_COUNT] |= ( m_pad_count != aValue );
        m_pad_count = aValue;
    }

    void AddPadCount( unsigned int aValue )
    {
        if( m_parent )
            m_parent->AddPadCount( aValue );

        m_column_changed[COLUMN_PAD_COUNT] |= ( aValue != 0 );
        m_pad_count += aValue;
    }

    void SubPadCount( unsigned int aValue )
    {
        if( m_parent )
            m_parent->SubPadCount( aValue );

        m_column_changed[COLUMN_PAD_COUNT] |= ( aValue != 0 );
        m_pad_count -= aValue;
    }

    unsigned GetViaCount() const { return m_via_count; }

    bool ViaCountChanged() const { return m_column_changed[COLUMN_VIA_COUNT]; }

    void SetViaCount( unsigned int aValue )
    {
        if( m_parent )
            m_parent->SetViaCount( m_parent->GetViaCount() - m_via_count + aValue );

        m_column_changed[COLUMN_VIA_COUNT] |= ( m_via_count != aValue );
        m_via_count = aValue;
    }

    void AddViaCount( unsigned int aValue )
    {
        if( m_parent )
            m_parent->AddViaCount( aValue );

        m_column_changed[COLUMN_VIA_COUNT] |= ( aValue != 0 );
        m_via_count += aValue;
    }

    void SubViaCount( unsigned int aValue )
    {
        if( m_parent )
            m_parent->SubViaCount( aValue );

        m_column_changed[COLUMN_VIA_COUNT] |= ( aValue != 0 );
        m_via_count -= aValue;
    }

    int64_t GetViaLength() const { return m_via_length; }

    bool ViaLengthChanged() const { return m_column_changed[COLUMN_VIA_LENGTH]; }

    void SetViaLength( unsigned int aValue )
    {
        if( m_parent )
            m_parent->SetViaLength( m_parent->GetViaLength() - m_via_length + aValue );

        m_column_changed[COLUMN_VIA_LENGTH] |= ( m_via_length != aValue );
        m_via_length = aValue;
    }

    void AddViaLength( unsigned int aValue )
    {
        if( m_parent )
            m_parent->AddViaLength( aValue );

        m_column_changed[COLUMN_VIA_LENGTH] |= ( aValue != 0 );
        m_via_length += aValue;
    }

    void SubViaLength( int64_t aValue )
    {
        if( m_parent )
            m_parent->SubViaLength( aValue );

        m_column_changed[COLUMN_VIA_LENGTH] |= ( aValue != 0 );
        m_via_length -= aValue;
    }

    int64_t GetViaDelay() const { return m_via_delay; }

    void SetViaDelay( unsigned int aValue )
    {
        if( m_parent )
            m_parent->SetViaDelay( m_parent->GetViaDelay() - m_via_delay + aValue );

        m_column_changed[COLUMN_VIA_LENGTH] |= ( m_via_delay != aValue );
        m_via_delay = aValue;
    }

    void AddViaDelay( unsigned int aValue )
    {
        if( m_parent )
            m_parent->AddViaDelay( aValue );

        m_column_changed[COLUMN_VIA_LENGTH] |= ( aValue != 0 );
        m_via_delay += aValue;
    }

    void SubViaDelay( int64_t aValue )
    {
        if( m_parent )
            m_parent->SubViaDelay( aValue );

        m_column_changed[COLUMN_VIA_LENGTH] |= ( aValue != 0 );
        m_via_delay -= aValue;
    }

    int64_t GetBoardWireLength() const
    {
        int64_t retval = 0;

        for( auto& [layer, length] : m_layer_wire_length )
            retval += length;

        return retval;
    }

    int64_t GetBoardWireDelay() const
    {
        int64_t retval = 0;

        for( auto& [layer, delay] : m_layer_wire_delay )
            retval += delay;

        return retval;
    }

    int64_t GetLayerWireLength( PCB_LAYER_ID aLayer ) const
    {
        auto it = m_layer_wire_length.find( aLayer );
        return it != m_layer_wire_length.end() ? it->second : 0;
    }

    int64_t GetLayerWireDelay( PCB_LAYER_ID aLayer ) const
    {
        auto it = m_layer_wire_delay.find( aLayer );
        return it != m_layer_wire_delay.end() ? it->second : 0;
    }

    bool BoardWireLengthChanged() const { return m_column_changed[COLUMN_BOARD_LENGTH]; }

    void SetLayerWireLength( const int64_t aValue, PCB_LAYER_ID aLayer )
    {
        auto it = m_layer_wire_length.find( aLayer );

        KI_CHECK_RET( it != m_layer_wire_length.end(), wxT( "Invalid layer specified" ) );

        auto& [_, length] = *it;

        if( m_parent )
        {
            m_parent->SetLayerWireLength( m_parent->GetBoardWireLength() - length + aValue,
                                           aLayer );
        }

        m_column_changed[COLUMN_BOARD_LENGTH] |= ( length != aValue );
        length = aValue;
    }

    std::map<PCB_LAYER_ID, int64_t> GetLayerWireLengths() const { return m_layer_wire_length; }

    std::map<PCB_LAYER_ID, int64_t> GetLayerWireDelays() const { return m_layer_wire_delay; }

    void SetLayerWireLengths( const std::map<PCB_LAYER_ID, int64_t>& aValue )
    {
        if( m_parent )
        {
            for( auto& [oldLayer, oldLength] : m_layer_wire_length )
                m_parent->SubLayerWireLength( oldLength, oldLayer );

            for( auto& [newLayer, newLength] : aValue )
                m_parent->AddLayerWireLength( newLength, newLayer );
        }

        m_layer_wire_length = aValue;
    }

    void AddLayerWireLength( const int64_t aValue, PCB_LAYER_ID aLayer )
    {
        if( m_parent )
            m_parent->AddLayerWireLength( aValue, aLayer );

        m_column_changed[COLUMN_BOARD_LENGTH] |= ( m_layer_wire_length[aLayer] != 0 );
        m_layer_wire_length[aLayer] += aValue;
    }

    void SubLayerWireLength( const int64_t aValue, PCB_LAYER_ID aLayer )
    {
        if( m_parent )
            m_parent->SubLayerWireLength( aValue, aLayer );

        m_column_changed[COLUMN_BOARD_LENGTH] |= ( m_layer_wire_length[aLayer] != 0 );
        m_layer_wire_length[aLayer] -= aValue;
    }

    void SetLayerWireDelays( const std::map<PCB_LAYER_ID, int64_t>& aValue )
    {
        if( m_parent )
        {
            for( auto& [oldLayer, oldLength] : m_layer_wire_delay )
                m_parent->SubLayerWireDelay( oldLength, oldLayer );

            for( auto& [newLayer, newLength] : aValue )
                m_parent->AddLayerWireDelay( newLength, newLayer );
        }

        m_layer_wire_delay = aValue;
    }

    void AddLayerWireDelay( const int64_t aValue, PCB_LAYER_ID aLayer )
    {
        if( m_parent )
            m_parent->AddLayerWireDelay( aValue, aLayer );

        m_column_changed[COLUMN_BOARD_LENGTH] |= ( m_layer_wire_delay[aLayer] != 0 );
        m_layer_wire_delay[aLayer] += aValue;
    }

    void SubLayerWireDelay( const int64_t aValue, PCB_LAYER_ID aLayer )
    {
        if( m_parent )
            m_parent->SubLayerWireDelay( aValue, aLayer );

        m_column_changed[COLUMN_BOARD_LENGTH] |= ( m_layer_wire_delay[aLayer] != 0 );
        m_layer_wire_delay[aLayer] -= aValue;
    }

    int64_t GetPadDieLength() const { return m_pad_die_length; }

    bool PadDieLengthChanged() const { return m_column_changed[COLUMN_PAD_DIE_LENGTH]; }

    void SetPadDieLength( int64_t aValue )
    {
        if( m_parent )
            m_parent->SetPadDieLength( m_parent->GetPadDieLength() - m_pad_die_length + aValue );

        m_column_changed[COLUMN_PAD_DIE_LENGTH] |= ( m_pad_die_length != aValue );
        m_pad_die_length = aValue;
    }

    void AddPadDieLength( int64_t aValue )
    {
        if( m_parent )
            m_parent->AddPadDieLength( aValue );

        m_column_changed[COLUMN_PAD_DIE_LENGTH] |= ( aValue != 0 );
        m_pad_die_length += aValue;
    }

    void SubPadDieLength( int64_t aValue )
    {
        if( m_parent )
            m_parent->SubPadDieLength( aValue );

        m_column_changed[COLUMN_PAD_DIE_LENGTH] |= ( aValue != 0 );
        m_pad_die_length -= aValue;
    }

    int64_t GetPadDieDelay() const { return m_pad_die_delay; }

    void SetPadDieDelay( int64_t aValue )
    {
        if( m_parent )
            m_parent->SetPadDieDelay( m_parent->GetPadDieDelay() - m_pad_die_delay + aValue );

        m_column_changed[COLUMN_PAD_DIE_LENGTH] |= ( m_pad_die_delay != aValue );
        m_pad_die_delay = aValue;
    }

    void AddPadDieDelay( int64_t aValue )
    {
        if( m_parent )
            m_parent->AddPadDieDelay( aValue );

        m_column_changed[COLUMN_PAD_DIE_LENGTH] |= ( aValue != 0 );
        m_pad_die_delay += aValue;
    }

    void SubPadDieDelay( int64_t aValue )
    {
        if( m_parent )
            m_parent->SubPadDieDelay( aValue );

        m_column_changed[COLUMN_PAD_DIE_LENGTH] |= ( aValue != 0 );
        m_pad_die_delay -= aValue;
    }

    // the total length column is always computed, never stored.
    unsigned long long int GetTotalLength() const
    {
        return GetBoardWireLength() + GetViaLength() + GetPadDieLength();
    }

    unsigned long long int GetTotalDelay() const { return GetBoardWireDelay() + GetViaDelay() + GetPadDieDelay(); }

    bool TotalLengthChanged() const
    {
        return BoardWireLengthChanged() || ViaLengthChanged() || PadDieLengthChanged();
    }

    LIST_ITEM* Parent() const { return m_parent; }

    void SetParent( LIST_ITEM* aParent )
    {
        if( m_parent == aParent )
            return;

        if( m_parent != nullptr )
        {
            m_parent->SubPadCount( GetPadCount() );
            m_parent->SubViaCount( GetViaCount() );
            m_parent->SubViaLength( GetViaLength() );
            m_parent->SubViaDelay( GetViaDelay() );

            for( auto& [layer, length] : m_layer_wire_length )
                m_parent->SubLayerWireLength( length, layer );

            for( auto& [layer, delay] : m_layer_wire_delay )
                m_parent->SubLayerWireDelay( delay, layer );

            m_parent->SubPadDieLength( GetPadDieLength() );
            m_parent->SubPadDieDelay( GetPadDieDelay() );

            m_parent->m_children.erase( std::find( m_parent->m_children.begin(),
                                                   m_parent->m_children.end(), this ) );
        }

        m_parent = aParent;

        if( m_parent != nullptr )
        {
            m_parent->AddPadCount( GetPadCount() );
            m_parent->AddViaCount( GetViaCount() );
            m_parent->AddViaLength( GetViaLength() );
            m_parent->AddViaDelay( GetViaDelay() );

            for( auto& [layer, length] : m_layer_wire_length )
                m_parent->AddLayerWireLength( length, layer );

            for( auto& [layer, delay] : m_layer_wire_delay )
                m_parent->AddLayerWireDelay( delay, layer );

            m_parent->AddPadDieLength( GetPadDieLength() );
            m_parent->AddPadDieDelay( GetPadDieDelay() );

            m_parent->m_children.push_back( this );
        }
    }

private:
    LIST_ITEM*              m_parent = nullptr;
    std::vector<LIST_ITEM*> m_children;

    GROUP_TYPE    m_group_type = GROUP_TYPE::NONE;
    unsigned int  m_group_number = 0;
    NETINFO_ITEM* m_net = nullptr;
    unsigned int  m_pad_count = 0;
    unsigned int  m_via_count = 0;
    int64_t       m_via_length = 0;
    int64_t       m_via_delay = 0;
    int64_t       m_pad_die_length = 0;
    int64_t       m_pad_die_delay = 0;

    std::map<PCB_LAYER_ID, int64_t> m_layer_wire_length{};
    std::map<PCB_LAYER_ID, int64_t> m_layer_wire_delay{};

    // Dirty bits to record when some attribute has changed, in order to avoid unnecessary sort
    // operations.
    // The values are semantically bools, but STL auto-promotes a std::vector<bool> to a bitset,
    // and then operator|= doesn't work.
    std::vector<int> m_column_changed;

    // cached formatted names for faster display sorting
    wxString m_net_name;
    wxString m_net_class;
    wxString m_group_name;
};


struct PCB_NET_INSPECTOR_PANEL::LIST_ITEM_NETCODE_CMP_LESS
{
    template <typename T>
    bool operator()( const T& a, const T& b ) const
    {
        return a->GetNetCode() < b->GetNetCode();
    }

    template <typename T>
    bool operator()( const T& a, int b ) const
    {
        return a->GetNetCode() < b;
    }

    template <typename T>
    bool operator()( int a, const T& b ) const
    {
        return a < b->GetNetCode();
    }
};


struct PCB_NET_INSPECTOR_PANEL::LIST_ITEM_GROUP_NUMBER_CMP_LESS
{
    template <typename T>
    bool operator()( const T& a, const T& b ) const
    {
        return a->GetGroupNumber() < b->GetGroupNumber();
    }

    template <typename T>
    bool operator()( const T& a, int b ) const
    {
        return a->GetGroupNumber() < b;
    }

    template <typename T>
    bool operator()( int a, const T& b ) const
    {
        return a < b->GetGroupNumber();
    }
};



#endif
