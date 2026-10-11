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
 * @file pcb_net_inspector_panel_data_model.h
 * The data model of the Net Inspector list with wxWidgets (the Qt build has its own, in
 * qt/pcbnew/widgets): the entries are in pcb_net_inspector_list_item.h.
 */

#ifndef PCB_NET_INSPECTOR_PANEL_DATA_MODEL
#define PCB_NET_INSPECTOR_PANEL_DATA_MODEL

#include <widgets/pcb_net_inspector_list_item.h>


/**
 * Data model for display in the Net Inspector panel.
*/
class PCB_NET_INSPECTOR_PANEL::DATA_MODEL : public wxDataViewModel
{
public:
    DATA_MODEL( PCB_NET_INSPECTOR_PANEL& parent ) :
            m_parent( parent )
    {}

    unsigned int columnCount() const { return m_parent.m_columns.size(); }

    unsigned int itemCount() const { return m_items.size(); }

    wxVariant valueAt( unsigned int aCol, unsigned int aRow ) const
    {
        wxVariant r;
        GetValue( r, wxDataViewItem( const_cast<LIST_ITEM*>( &*( m_items[aRow] ) ) ), aCol );
        return r;
    }

    const LIST_ITEM& itemAt( unsigned int aRow ) const { return *m_items.at( aRow ); }

    wxVariant valueForItem( const LIST_ITEM* aItem, unsigned int aCol ) const
    {
        wxVariant r;
        GetValue( r, wxDataViewItem( const_cast<LIST_ITEM*>( aItem ) ), aCol );
        return r;
    }

    std::vector<std::pair<wxString, wxDataViewItem>> getGroupDataViewItems()
    {
        std::vector<std::pair<wxString, wxDataViewItem>> ret;

        for( std::unique_ptr<LIST_ITEM>& item : m_items )
        {
            if( item->GetIsGroup() )
                ret.push_back( std::make_pair( item->GetGroupName(), wxDataViewItem( item.get() ) ) );
        }

        return ret;
    }

    std::optional<LIST_ITEM_ITER> findItem( int aNetCode )
    {
        auto i = std::lower_bound( m_items.begin(), m_items.end(), aNetCode,
                                   LIST_ITEM_NETCODE_CMP_LESS() );

        if( i == m_items.end() || ( *i )->GetNetCode() != aNetCode )
            return std::nullopt;

        return { i };
    }

    std::optional<LIST_ITEM_ITER> findItem( NETINFO_ITEM* aNet )
    {
        if( aNet != nullptr )
            return findItem( aNet->GetNetCode() );
        else
            return std::nullopt;
    }

    std::optional<LIST_ITEM_ITER> findGroupItem( int aGroupNumber )
    {
        auto i = std::lower_bound( m_items.begin(), m_items.end(), aGroupNumber,
                                   LIST_ITEM_GROUP_NUMBER_CMP_LESS() );

        if( i == m_items.end() || ( *i )->GetGroupNumber() != aGroupNumber )
            return std::nullopt;

        return { i };
    }

    LIST_ITEM* addGroup( LIST_ITEM_ITER groupsBegin, LIST_ITEM_ITER groupsEnd,
                         wxString groupName, LIST_ITEM::GROUP_TYPE groupType )
    {
        LIST_ITEM_ITER group = std::find_if( groupsBegin, groupsEnd,
                                             [&]( const std::unique_ptr<LIST_ITEM>& x )
                                             {
                                                 return x->GetGroupName() == groupName
                                                        && x->GetGroupType() == groupType;
                                             } );

        if( group == groupsEnd )
        {
            int                        dist = std::distance( groupsBegin, groupsEnd );
            std::unique_ptr<LIST_ITEM> groupItem = std::make_unique<LIST_ITEM>( dist, groupName, groupType );

            group = m_items.insert( groupsEnd, std::move( groupItem ) );
            ItemAdded( wxDataViewItem( ( *group )->Parent() ), wxDataViewItem( ( *group ).get() ) );
        }

        return ( *group ).get();
    }

    std::optional<LIST_ITEM_ITER> addItem( std::unique_ptr<LIST_ITEM> aItem )
    {
        if( aItem == nullptr )
            return {};

        bool groupMatched = false;

        // First see if item matches a group-by rule
        if( m_parent.m_custom_group_rules.size() > 0 )
        {
            wxString searchName = aItem->GetNetName();

            for( const std::unique_ptr<EDA_COMBINED_MATCHER>& rule : m_parent.m_custom_group_rules )
            {
                if( rule->Find( searchName ) )
                {
                    aItem->SetParent( m_custom_group_map[ rule->GetPattern() ] );
                    groupMatched = true;
                    break;
                }
            }
        }

        // Then add any netclass groups required by this item
        if( m_parent.m_groupByNetclass && !groupMatched )
        {
            LIST_ITEM_ITER groups_begin = m_items.begin();
            LIST_ITEM_ITER groups_end = std::find_if_not( m_items.begin(), m_items.end(),
                                                          []( const std::unique_ptr<LIST_ITEM>& x )
                                                          {
                                                              return x->GetIsGroup();
                                                          } );

            wxString match_str = aItem->GetNetclassName();
            LIST_ITEM* group = addGroup( groups_begin, groups_end, match_str, LIST_ITEM::GROUP_TYPE::NETCLASS );
            aItem->SetParent( group );
        }

        // Now add the item itself. Usually when new nets are added,
        // they always get a higher netcode number than the already existing ones.
        // however, if we've got filtering enabled, we might not have all the nets in
        // our list, so do a sorted insertion.
        auto new_iter = std::lower_bound( m_items.begin(), m_items.end(), aItem->GetNetCode(),
                                          LIST_ITEM_NETCODE_CMP_LESS() );

        new_iter = m_items.insert( new_iter, std::move( aItem ) );
        const std::unique_ptr<LIST_ITEM>& new_item = *new_iter;

        ItemAdded( wxDataViewItem( new_item->Parent() ), wxDataViewItem( new_item.get() ) );

        return { new_iter };
    }

    void addItems( std::vector<std::unique_ptr<LIST_ITEM>>& aItems )
    {
        m_items.reserve( m_items.size() + aItems.size() );

        for( std::unique_ptr<LIST_ITEM>& i : aItems )
            addItem( std::move( i ) );
    }

    std::unique_ptr<LIST_ITEM> deleteItem( const std::optional<LIST_ITEM_ITER>& aRow )
    {
        if( !aRow )
            return {};

        std::unique_ptr<LIST_ITEM> i = std::move( **aRow );

        LIST_ITEM* parent = i->Parent();
        i->SetParent( nullptr );

        m_items.erase( *aRow );
        ItemDeleted( wxDataViewItem( parent ), wxDataViewItem( &*i ) );

        if( parent )
        {
            ItemChanged( wxDataViewItem( parent ) );

            if( m_parent.m_groupByNetclass && parent != nullptr && parent->ChildrenCount() == 0 )
            {
                auto p = std::find_if( m_items.begin(), m_items.end(),
                                       [&]( std::unique_ptr<LIST_ITEM>& x )
                                       {
                                           return x.get() == parent;
                                       } );

                KI_ASSERT( p != m_items.end() );

                // Keep the parent alive through the ItemDeleted notification; erasing it from
                // m_items destroys the LIST_ITEM, after which parent would be dangling.
                wxDataViewItem             grandParent( parent->Parent() );
                std::unique_ptr<LIST_ITEM> removedParent = std::move( *p );
                m_items.erase( p );

                ItemDeleted( grandParent, wxDataViewItem( removedParent.get() ) );
            }
        }

        Resort();
        return i;
    }

    /**
     * Adds all custom group-by entries to the items table
     *
     * Note this assumes that m_items is empty prior to adding these groups
     */
    void addCustomGroups()
    {
        m_custom_group_map.clear();
        int groupId = 0;

        for( const std::unique_ptr<EDA_COMBINED_MATCHER>& rule : m_parent.m_custom_group_rules )
        {
            std::unique_ptr<LIST_ITEM>& group = m_items.emplace_back( std::make_unique<LIST_ITEM>(
                    groupId, rule->GetPattern(), LIST_ITEM::GROUP_TYPE::USER_DEFINED ) );
            m_custom_group_map[ rule->GetPattern() ] = group.get();
            group->SetLayerCount( m_parent.m_board->GetCopperLayerCount() );
            ItemAdded( wxDataViewItem( group->Parent() ), wxDataViewItem( group.get() ) );
            ++groupId;
        }
    }

    void deleteAllItems()
    {
        // BeforeReset() drives the canceller, dropping any deferred EnsureVisible before the
        // clear frees items it may point at.
        BeforeReset();
        m_items.clear();
        AfterReset();
    }

    void updateItem( const std::optional<LIST_ITEM_ITER>& aRow )
    {
        if( aRow )
        {
            const std::unique_ptr<LIST_ITEM>& listItem = *aRow.value();

            if( listItem->Parent() )
                ItemChanged( wxDataViewItem( listItem->Parent() ) );

            ItemChanged( wxDataViewItem( listItem.get() ) );
            resortIfChanged( listItem.get() );
        }
    }

    void updateAllItems()
    {
        for( std::unique_ptr<LIST_ITEM>& i : m_items )
            ItemChanged( wxDataViewItem( i.get() ) );
    }

    void resortIfChanged( LIST_ITEM* aItem )
    {
        if( wxDataViewColumn* column = m_parent.m_netsList->GetSortingColumn() )
        {
            bool changed = false;

            for( const LIST_ITEM* i = aItem; i != nullptr; i = i->Parent() )
                changed |= itemColumnChanged( i, column->GetModelColumn() );

            for( LIST_ITEM* i = aItem; i != nullptr; i = i->Parent() )
                i->ResetColumnChangedBits();

            if( changed )
                Resort();
        }
    }

    bool itemColumnChanged( const LIST_ITEM* aItem, unsigned int aCol ) const
    {
        if( aItem == nullptr || aCol >= m_parent.m_columns.size() )
            return false;

        if( aCol == COLUMN_PAD_COUNT )
            return aItem->PadCountChanged();

        else if( aCol == COLUMN_VIA_COUNT )
            return aItem->ViaCountChanged();

        else if( aCol == COLUMN_VIA_LENGTH )
            return aItem->ViaLengthChanged();

        else if( aCol == COLUMN_BOARD_LENGTH )
            return aItem->BoardWireLengthChanged();

        else if( aCol == COLUMN_PAD_DIE_LENGTH )
            return aItem->PadDieLengthChanged();

        else if( aCol == COLUMN_TOTAL_LENGTH )
            return aItem->TotalLengthChanged();

        else if( aCol > COLUMN_LAST_STATIC_COL )
            return aItem->BoardWireLengthChanged();


        return false;
    }

    void SetIsTimeDomain( const bool aIsTimeDomain ) { m_show_time_domain_details = aIsTimeDomain; }

    // implementation of wxDataViewModel interface
    // these are used to query the data model by the GUI view implementation.
    // these are not supposed to be used to modify the data model.  for that
    // use the public functions above.

protected:
    unsigned int GetColumnCount() const override { return columnCount(); }

    void GetValue( wxVariant& aOutValue, const wxDataViewItem& aItem,
                   unsigned int aCol ) const override
    {
        if( LIST_ITEM* i = static_cast<LIST_ITEM*>( aItem.GetID() ) )
            aOutValue = m_parent.itemText( i, aCol, m_show_time_domain_details );
    }

    int Compare( const wxDataViewItem& aItem1, const wxDataViewItem& aItem2, unsigned int aCol,
                 bool aAsc ) const override
    {
        const LIST_ITEM& i1 = *static_cast<const LIST_ITEM*>( aItem1.GetID() );
        const LIST_ITEM& i2 = *static_cast<const LIST_ITEM*>( aItem2.GetID() );

        if( int res = m_parent.compareItems( i1, i2, aCol, aAsc, m_show_time_domain_details ) )
            return res;

        // when the item values compare equal resort to pointer comparison.
        wxUIntPtr id1 = wxPtrToUInt( aItem1.GetID() );
        wxUIntPtr id2 = wxPtrToUInt( aItem2.GetID() );

        return aAsc ? id1 - id2 : id2 - id1;
    }

    bool SetValue( const wxVariant& aInValue, const wxDataViewItem& aItem,
                   unsigned int aCol ) override
    {
        return false;
    }

    wxDataViewItem GetParent( const wxDataViewItem& aItem ) const override
    {
        if( !aItem.IsOk() )
            return wxDataViewItem();

        return wxDataViewItem( static_cast<const LIST_ITEM*>( aItem.GetID() )->Parent() );
    }

    bool IsContainer( const wxDataViewItem& aItem ) const override
    {
        if( !aItem.IsOk() )
            return true;

        return static_cast<const LIST_ITEM*>( aItem.GetID() )->GetIsGroup();
    }

    bool HasContainerColumns( const wxDataViewItem& aItem ) const override
    {
        return IsContainer( aItem );
    }

    unsigned int GetChildren( const wxDataViewItem& aParent,
                              wxDataViewItemArray&  aChildren ) const override
    {
        const LIST_ITEM* p = static_cast<const LIST_ITEM*>( aParent.GetID() );

        if( !aParent.IsOk() )
        {
            aChildren.Alloc( m_items.size() );

            for( const std::unique_ptr<LIST_ITEM>& i : m_items )
            {
                if( i->Parent() == nullptr )
                    aChildren.Add( wxDataViewItem( &*i ) );
            }

            return aChildren.GetCount();
        }
        else if( p->GetIsGroup() )
        {
            const int count = p->ChildrenCount();

            if( count == 0 )
                return 0;

            aChildren.Alloc( count );

            for( auto i = p->ChildrenBegin(), end = p->ChildrenEnd(); i != end; ++i )
                aChildren.Add( wxDataViewItem( *i ) );

            return aChildren.GetCount();
        }

        return 0;
    }

    wxString GetColumnType( unsigned int /* aCol */ ) const override { return wxS( "string" ); }

private:
    PCB_NET_INSPECTOR_PANEL& m_parent;

    // primary container, sorted by netcode number.
    // groups have netcode < 0, so they always come first, in the order
    // of the filter strings as input by the user
    std::vector<std::unique_ptr<LIST_ITEM>> m_items;

    /// Map of custom group names to their representative list item
    std::map<wxString, LIST_ITEM*> m_custom_group_map;

    bool m_show_time_domain_details{ false };
};

#endif
