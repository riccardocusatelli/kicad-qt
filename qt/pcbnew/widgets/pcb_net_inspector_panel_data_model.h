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
 * The data model of the Net Inspector list with Qt: a tree of the groups and of the nets as a
 * QAbstractItemModel, with the same interface as the wxDataViewModel of the wxWidgets build
 * (pcbnew/widgets/pcb_net_inspector_panel_data_model.h).  The entries are in
 * pcb_net_inspector_list_item.h; the view sorts them with a QSortFilterProxyModel.
 */

#ifndef PCB_NET_INSPECTOR_PANEL_DATA_MODEL
#define PCB_NET_INSPECTOR_PANEL_DATA_MODEL

#include <QAbstractItemModel>

#include <qt_strings.h>
#include <widgets/pcb_net_inspector_list_item.h>


class PCB_NET_INSPECTOR_PANEL::DATA_MODEL : public QAbstractItemModel
{
public:
    DATA_MODEL( PCB_NET_INSPECTOR_PANEL& parent ) :
            QAbstractItemModel( &parent ),
            m_parent( parent )
    {}

    unsigned int itemCount() const { return m_items.size(); }

    const LIST_ITEM& itemAt( unsigned int aRow ) const { return *m_items.at( aRow ); }

    /// The groups, with their names.
    std::vector<std::pair<wxString, LIST_ITEM*>> getGroupItems()
    {
        std::vector<std::pair<wxString, LIST_ITEM*>> ret;

        for( std::unique_ptr<LIST_ITEM>& item : m_items )
        {
            if( item->GetIsGroup() )
                ret.push_back( std::make_pair( item->GetGroupName(), item.get() ) );
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
            insertTopLevel( group->get() );
        }

        return ( *group ).get();
    }

    std::optional<LIST_ITEM_ITER> addItem( std::unique_ptr<LIST_ITEM> aItem )
    {
        if( aItem == nullptr )
            return {};

        LIST_ITEM* parent = nullptr;

        // First see if item matches a group-by rule
        if( m_parent.m_custom_group_rules.size() > 0 )
        {
            wxString searchName = aItem->GetNetName();

            for( const std::unique_ptr<EDA_COMBINED_MATCHER>& rule : m_parent.m_custom_group_rules )
            {
                if( rule->Find( searchName ) )
                {
                    parent = m_custom_group_map[ rule->GetPattern() ];
                    break;
                }
            }
        }

        // Then add any netclass groups required by this item
        if( m_parent.m_groupByNetclass && !parent )
        {
            LIST_ITEM_ITER groups_begin = m_items.begin();
            LIST_ITEM_ITER groups_end = std::find_if_not( m_items.begin(), m_items.end(),
                                                          []( const std::unique_ptr<LIST_ITEM>& x )
                                                          {
                                                              return x->GetIsGroup();
                                                          } );

            parent = addGroup( groups_begin, groups_end, aItem->GetNetclassName(),
                               LIST_ITEM::GROUP_TYPE::NETCLASS );
        }

        // Now add the item itself. Usually when new nets are added,
        // they always get a higher netcode number than the already existing ones.
        // however, if we've got filtering enabled, we might not have all the nets in
        // our list, so do a sorted insertion.
        auto new_iter = std::lower_bound( m_items.begin(), m_items.end(), aItem->GetNetCode(),
                                          LIST_ITEM_NETCODE_CMP_LESS() );

        new_iter = m_items.insert( new_iter, std::move( aItem ) );
        LIST_ITEM* new_item = new_iter->get();

        if( parent )
        {
            int row = static_cast<int>( parent->ChildrenCount() );

            beginInsertRows( indexOf( parent ), row, row );
            new_item->SetParent( parent );
            endInsertRows();

            itemChanged( parent );
        }
        else
        {
            insertTopLevel( new_item );
        }

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
        LIST_ITEM*                 parent = i->Parent();

        m_items.erase( *aRow );

        if( parent )
        {
            int row = childRow( parent, i.get() );

            beginRemoveRows( indexOf( parent ), row, row );
            i->SetParent( nullptr );
            endRemoveRows();

            itemChanged( parent );

            if( m_parent.m_groupByNetclass && parent->ChildrenCount() == 0 )
            {
                auto p = std::find_if( m_items.begin(), m_items.end(),
                                       [&]( std::unique_ptr<LIST_ITEM>& x )
                                       {
                                           return x.get() == parent;
                                       } );

                KI_ASSERT( p != m_items.end() );

                // Out of the view before being destroyed
                removeTopLevel( parent );
                m_items.erase( p );
            }
        }
        else
        {
            removeTopLevel( i.get() );
        }

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
            insertTopLevel( group.get() );
            ++groupId;
        }
    }

    void deleteAllItems()
    {
        beginResetModel();
        m_topLevel.clear();
        m_items.clear();
        endResetModel();
    }

    void updateItem( const std::optional<LIST_ITEM_ITER>& aRow )
    {
        if( aRow )
        {
            const std::unique_ptr<LIST_ITEM>& listItem = *aRow.value();

            if( listItem->Parent() )
                itemChanged( listItem->Parent() );

            itemChanged( listItem.get() );
            listItem->ResetColumnChangedBits();
        }
    }

    void updateAllItems()
    {
        for( std::unique_ptr<LIST_ITEM>& i : m_items )
            itemChanged( i.get() );
    }

    void SetIsTimeDomain( const bool aIsTimeDomain ) { m_show_time_domain_details = aIsTimeDomain; }

    bool IsTimeDomain() const { return m_show_time_domain_details; }

    /// The index of an entry in the model (invalid for nullptr).
    QModelIndex indexOf( const LIST_ITEM* aItem, int aColumn = 0 ) const
    {
        if( !aItem )
            return QModelIndex();

        int row = aItem->Parent() ? childRow( aItem->Parent(), aItem ) : topLevelRow( aItem );

        if( row < 0 )
            return QModelIndex();

        return createIndex( row, aColumn, const_cast<LIST_ITEM*>( aItem ) );
    }

    static LIST_ITEM* itemOf( const QModelIndex& aIndex )
    {
        return aIndex.isValid() ? static_cast<LIST_ITEM*>( aIndex.internalPointer() ) : nullptr;
    }

    // The interface of QAbstractItemModel

    QModelIndex index( int aRow, int aColumn, const QModelIndex& aParent = QModelIndex() ) const override
    {
        if( aRow < 0 || aColumn < 0 || aColumn >= columnCount( aParent ) )
            return QModelIndex();

        if( !aParent.isValid() )
        {
            if( aRow >= static_cast<int>( m_topLevel.size() ) )
                return QModelIndex();

            return createIndex( aRow, aColumn, m_topLevel[aRow] );
        }

        const LIST_ITEM* parent = itemOf( aParent );

        if( !parent || aRow >= static_cast<int>( parent->ChildrenCount() ) )
            return QModelIndex();

        return createIndex( aRow, aColumn, *( parent->ChildrenBegin() + aRow ) );
    }

    QModelIndex parent( const QModelIndex& aIndex ) const override
    {
        const LIST_ITEM* item = itemOf( aIndex );

        if( !item || !item->Parent() )
            return QModelIndex();

        return indexOf( item->Parent() );
    }

    int rowCount( const QModelIndex& aParent = QModelIndex() ) const override
    {
        if( !aParent.isValid() )
            return static_cast<int>( m_topLevel.size() );

        if( aParent.column() != 0 )
            return 0;

        const LIST_ITEM* parent = itemOf( aParent );
        return parent && parent->GetIsGroup() ? static_cast<int>( parent->ChildrenCount() ) : 0;
    }

    int columnCount( const QModelIndex& aParent = QModelIndex() ) const override
    {
        return static_cast<int>( m_parent.m_columns.size() );
    }

    QVariant data( const QModelIndex& aIndex, int aRole = Qt::DisplayRole ) const override
    {
        const LIST_ITEM* item = itemOf( aIndex );

        if( !item )
            return QVariant();

        if( aRole == Qt::DisplayRole )
        {
            return ToQString( m_parent.itemText( item, aIndex.column(), m_show_time_domain_details ) );
        }
        else if( aRole == Qt::TextAlignmentRole )
        {
            if( aIndex.column() == COLUMN_NAME || aIndex.column() == COLUMN_NETCLASS )
                return QVariant::fromValue( Qt::AlignLeft | Qt::AlignVCenter );

            return QVariant::fromValue( Qt::AlignCenter );
        }

        return QVariant();
    }

    QVariant headerData( int aSection, Qt::Orientation aOrientation,
                         int aRole = Qt::DisplayRole ) const override
    {
        if( aOrientation != Qt::Horizontal || aSection < 0
            || aSection >= static_cast<int>( m_parent.m_columns.size() ) )
        {
            return QVariant();
        }

        if( aRole == Qt::DisplayRole )
            return ToQString( m_parent.m_columns[aSection].display_name );

        if( aRole == Qt::TextAlignmentRole && aSection != COLUMN_NAME && aSection != COLUMN_NETCLASS )
            return QVariant::fromValue( Qt::AlignCenter );

        return QVariant();
    }

    Qt::ItemFlags flags( const QModelIndex& aIndex ) const override
    {
        if( !aIndex.isValid() )
            return Qt::NoItemFlags;

        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }

    /// The columns changed (rebuilt for other layers or for the time domain).
    void resetColumns()
    {
        beginResetModel();
        endResetModel();
    }

private:
    int topLevelRow( const LIST_ITEM* aItem ) const
    {
        auto it = std::find( m_topLevel.begin(), m_topLevel.end(), aItem );
        return it == m_topLevel.end() ? -1 : static_cast<int>( it - m_topLevel.begin() );
    }

    static int childRow( const LIST_ITEM* aParent, const LIST_ITEM* aItem )
    {
        auto it = std::find( aParent->ChildrenBegin(), aParent->ChildrenEnd(), aItem );
        return it == aParent->ChildrenEnd() ? -1 : static_cast<int>( it - aParent->ChildrenBegin() );
    }

    void insertTopLevel( LIST_ITEM* aItem )
    {
        int row = static_cast<int>( m_topLevel.size() );

        beginInsertRows( QModelIndex(), row, row );
        m_topLevel.push_back( aItem );
        endInsertRows();
    }

    void removeTopLevel( LIST_ITEM* aItem )
    {
        int row = topLevelRow( aItem );

        if( row < 0 )
            return;

        beginRemoveRows( QModelIndex(), row, row );
        m_topLevel.erase( m_topLevel.begin() + row );
        endRemoveRows();
    }

    /// The values of an entry changed: the view shows them (and sorts it again).
    void itemChanged( const LIST_ITEM* aItem )
    {
        QModelIndex first = indexOf( aItem, 0 );

        if( !first.isValid() )
            return;

        Q_EMIT dataChanged( first, indexOf( aItem, std::max( 0, columnCount( QModelIndex() ) - 1 ) ) );
    }

private:
    PCB_NET_INSPECTOR_PANEL& m_parent;

    // primary container, sorted by netcode number.
    // groups have netcode < 0, so they always come first, in the order
    // of the filter strings as input by the user
    std::vector<std::unique_ptr<LIST_ITEM>> m_items;

    /// The entries without a parent, in the order of the rows of the model
    std::vector<LIST_ITEM*> m_topLevel;

    /// Map of custom group names to their representative list item
    std::map<wxString, LIST_ITEM*> m_custom_group_map;

    bool m_show_time_domain_details{ false };
};

#endif
