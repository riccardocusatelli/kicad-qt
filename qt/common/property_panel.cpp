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

#include "property_panel.h"

#include <QHeaderView>


PROPERTY_PANEL::PROPERTY_PANEL( QWidget* aParent ) :
        QTreeWidget( aParent )
{
    setColumnCount( 2 );
    setHeaderLabels( { tr( "Property" ), tr( "Value" ) } );
    // Names and values share the width (long ones are elided, with a tooltip)
    header()->setSectionResizeMode( QHeaderView::Stretch );
    setTextElideMode( Qt::ElideRight );
    setRootIsDecorated( true );
    setAlternatingRowColors( true );
    setSelectionMode( QAbstractItemView::NoSelection );
    setUniformRowHeights( true );

    ShowProperties( QString(), {} );
}


void PROPERTY_PANEL::ShowProperties( const QString& aTitle, const std::vector<PROPERTY_ROW>& aRows )
{
    clear();

    if( aRows.empty() )
    {
        auto* item = new QTreeWidgetItem( this );
        item->setText( 0, aTitle.isEmpty() ? tr( "No selection" ) : aTitle );
        item->setFirstColumnSpanned( true );
        item->setDisabled( true );
        return;
    }

    if( !aTitle.isEmpty() )
    {
        auto* title = new QTreeWidgetItem( this );
        title->setText( 0, aTitle );
        title->setFirstColumnSpanned( true );

        QFont font = title->font( 0 );
        font.setBold( true );
        title->setFont( 0, font );
    }

    QTreeWidgetItem* group = nullptr;
    QString          groupName;

    for( const PROPERTY_ROW& row : aRows )
    {
        if( !group || row.m_Group != groupName )
        {
            groupName = row.m_Group;
            group = new QTreeWidgetItem( this );
            group->setText( 0, groupName.isEmpty() ? tr( "Basic Properties" ) : groupName );
            group->setFirstColumnSpanned( true );
            group->setExpanded( true );

            QFont font = group->font( 0 );
            font.setBold( true );
            group->setFont( 0, font );
        }

        auto* item = new QTreeWidgetItem( group );
        item->setText( 0, row.m_Name );
        item->setText( 1, row.m_Value );
        item->setToolTip( 0, row.m_Name );
        item->setToolTip( 1, row.m_Value );
    }

    expandAll();
}
