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

#include <QComboBox>
#include <QHeaderView>
#include <QStyledItemDelegate>


/// Role of the value items holding the index of their row in PROPERTY_PANEL::m_rows
static constexpr int ROW_ROLE = Qt::UserRole;

/// Role of the value items holding the value shown before an edit
static constexpr int SHOWN_ROLE = Qt::UserRole + 1;


namespace
{

/// The editors of the values: a line for the texts, a list for the choices.
class VALUE_DELEGATE : public QStyledItemDelegate
{
public:
    VALUE_DELEGATE( const std::vector<PROPERTY_ROW>& aRows, QObject* aParent ) :
            QStyledItemDelegate( aParent ),
            m_rows( aRows )
    {
    }

    QWidget* createEditor( QWidget* aParent, const QStyleOptionViewItem& aOption,
                           const QModelIndex& aIndex ) const override
    {
        if( const PROPERTY_ROW* row = rowOf( aIndex ) )
        {
            if( row->m_Editor == PROPERTY_ROW::EDITOR::CHOICE )
            {
                auto* combo = new QComboBox( aParent );
                combo->addItems( row->m_Choices );
                return combo;
            }
        }

        return QStyledItemDelegate::createEditor( aParent, aOption, aIndex );
    }

    void setEditorData( QWidget* aEditor, const QModelIndex& aIndex ) const override
    {
        if( auto* combo = qobject_cast<QComboBox*>( aEditor ) )
            combo->setCurrentText( aIndex.data( Qt::DisplayRole ).toString() );
        else
            QStyledItemDelegate::setEditorData( aEditor, aIndex );
    }

    void setModelData( QWidget* aEditor, QAbstractItemModel* aModel,
                       const QModelIndex& aIndex ) const override
    {
        if( auto* combo = qobject_cast<QComboBox*>( aEditor ) )
            aModel->setData( aIndex, combo->currentText(), Qt::EditRole );
        else
            QStyledItemDelegate::setModelData( aEditor, aModel, aIndex );
    }

private:
    const PROPERTY_ROW* rowOf( const QModelIndex& aIndex ) const
    {
        QVariant row = aIndex.data( ROW_ROLE );

        if( !row.isValid() || row.toInt() < 0 || row.toInt() >= static_cast<int>( m_rows.size() ) )
            return nullptr;

        return &m_rows[row.toInt()];
    }

    const std::vector<PROPERTY_ROW>& m_rows;
};

} // namespace


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
    setSelectionMode( QAbstractItemView::SingleSelection );
    setUniformRowHeights( true );
    setEditTriggers( QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked
                     | QAbstractItemView::EditKeyPressed );
    setItemDelegateForColumn( 1, new VALUE_DELEGATE( m_rows, this ) );

    connect( this, &QTreeWidget::itemChanged, this, &PROPERTY_PANEL::onItemChanged );

    ShowProperties( QString(), {} );
}


void PROPERTY_PANEL::ShowProperties( const QString& aTitle, const std::vector<PROPERTY_ROW>& aRows )
{
    m_filling = true;

    clear();
    m_rows = aRows;

    if( m_rows.empty() )
    {
        auto* item = new QTreeWidgetItem( this );
        item->setText( 0, aTitle.isEmpty() ? tr( "No selection" ) : aTitle );
        item->setFirstColumnSpanned( true );
        item->setDisabled( true );
        m_filling = false;
        return;
    }

    if( !aTitle.isEmpty() )
    {
        auto* title = new QTreeWidgetItem( this );
        title->setText( 0, aTitle );
        title->setFirstColumnSpanned( true );
        title->setFlags( Qt::ItemIsEnabled );

        QFont font = title->font( 0 );
        font.setBold( true );
        title->setFont( 0, font );
    }

    QTreeWidgetItem* group = nullptr;
    QString          groupName;

    for( size_t i = 0; i < m_rows.size(); ++i )
    {
        const PROPERTY_ROW& row = m_rows[i];

        if( !group || row.m_Group != groupName )
        {
            groupName = row.m_Group;
            group = new QTreeWidgetItem( this );
            group->setText( 0, groupName.isEmpty() ? tr( "Basic Properties" ) : groupName );
            group->setFirstColumnSpanned( true );
            group->setFlags( Qt::ItemIsEnabled );
            group->setExpanded( true );

            QFont font = group->font( 0 );
            font.setBold( true );
            group->setFont( 0, font );
        }

        auto* item = new QTreeWidgetItem( group );
        item->setText( 0, row.m_Name );
        item->setToolTip( 0, row.m_Name );
        item->setData( 1, ROW_ROLE, static_cast<int>( i ) );

        Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

        switch( row.m_Editor )
        {
        case PROPERTY_ROW::EDITOR::CHECK:
            flags |= Qt::ItemIsUserCheckable;
            item->setCheckState( 1, row.m_Mixed ? Qt::PartiallyChecked
                                                : row.m_Value == QLatin1String( "1" ) ? Qt::Checked
                                                                                      : Qt::Unchecked );
            break;

        case PROPERTY_ROW::EDITOR::TEXT:
        case PROPERTY_ROW::EDITOR::CHOICE:
            flags |= Qt::ItemIsEditable;
            item->setText( 1, row.m_Value );
            break;

        case PROPERTY_ROW::EDITOR::NONE:
            item->setText( 1, row.m_Value );
            item->setForeground( 1, palette().brush( QPalette::Disabled, QPalette::Text ) );
            break;
        }

        item->setFlags( flags );
        item->setData( 1, SHOWN_ROLE, item->text( 1 ) );
        item->setToolTip( 1, row.m_Mixed ? tr( "The selected items have different values" )
                                         : row.m_Value );
    }

    expandAll();
    m_filling = false;
}


void PROPERTY_PANEL::onItemChanged( QTreeWidgetItem* aItem, int aColumn )
{
    if( m_filling || aColumn != 1 )
        return;

    QVariant rowIndex = aItem->data( 1, ROW_ROLE );

    if( !rowIndex.isValid() || rowIndex.toInt() < 0 || rowIndex.toInt() >= static_cast<int>( m_rows.size() ) )
        return;

    const PROPERTY_ROW& row = m_rows[rowIndex.toInt()];
    QString             value;

    if( row.m_Editor == PROPERTY_ROW::EDITOR::CHECK )
    {
        if( aItem->checkState( 1 ) == Qt::PartiallyChecked )
            return;

        value = aItem->checkState( 1 ) == Qt::Checked ? QStringLiteral( "1" ) : QStringLiteral( "0" );
    }
    else
    {
        // Only a value really changed: an edit closed without a change keeps the value shown
        value = aItem->text( 1 );

        if( value == aItem->data( 1, SHOWN_ROLE ).toString() )
            return;
    }

    // The change shows the properties again: not while the tree is notifying its item
    const void* key = row.m_Key;

    QMetaObject::invokeMethod( this, [this, key, value]() { Q_EMIT valueEdited( key, value ); },
                               Qt::QueuedConnection );
}
