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
 * @file eda_list_dialog.cpp
 * EDA_LIST_DIALOG of the Qt build (see common/dialogs/eda_list_dialog.cpp).
 */

#include <eda_list_dialog.h>

#include <algorithm>

#include <QCheckBox>
#include <QHeaderView>

#include <lib_tree_pinning.h>
#include <macros.h>
#include <string_utils.h>
#include <core/ki_assert.h>


// Note, these are "standard DPI" widths
static int DEFAULT_SINGLE_COL_WIDTH = 260;
static int DEFAULT_COL_WIDTHS[] = { 200, 300 };


EDA_LIST_DIALOG::EDA_LIST_DIALOG( TOOLKIT_WINDOW* aParent, const wxString& aTitle,
                                  const wxArrayString& aItemHeaders,
                                  const std::vector<wxArrayString>& aItemList,
                                  const wxString& aPreselectText, bool aSortList ) :
        EDA_LIST_DIALOG_BASE( aParent, aTitle ),
        m_sortList( aSortList )
{
    m_filterBox->setPlaceholderText( ToQString( _( "Filter" ) ) );

    initDialog( aItemHeaders, aItemList, aPreselectText );

    if( aItemList.size() > 16 )
        m_listBox->setMinimumHeight( KiROUND( m_listBox->minimumHeight() * 1.66 ) );

    // DIALOG_SHIM needs a unique hash_key because classname is not sufficient
    // because so many dialogs share this same class, with different numbers of
    // columns, different column names, and column widths.
    m_hash_key = TO_UTF8( aTitle );

    SetupStandardButtons();

    adjustSize();
}


EDA_LIST_DIALOG::EDA_LIST_DIALOG( TOOLKIT_WINDOW* aParent, const wxString& aTitle,
                                  bool aSortList ) :
        EDA_LIST_DIALOG_BASE( aParent, aTitle ),
        m_sortList( aSortList )
{
    m_filterBox->setPlaceholderText( ToQString( _( "Filter" ) ) );
}


void EDA_LIST_DIALOG::AddExtraCheckbox( const wxString& aLabel, bool* aValuePtr )
{
    KI_CHECK2_MSG( aValuePtr, return, wxT( "Null pointer for checkbox value." ) );

    QCheckBox* cb = new QCheckBox( ToQString( aLabel ), this );
    cb->setChecked( *aValuePtr );
    m_ExtrasSizer->addWidget( cb );
    m_extraCheckboxMap[cb] = aValuePtr;
}


void EDA_LIST_DIALOG::initDialog( const wxArrayString& aItemHeaders,
                                  const std::vector<wxArrayString>& aItemList,
                                  const wxString& aPreselectText )
{
    QStringList headers;

    for( const wxString& header : aItemHeaders )
        headers.append( ToQString( header ) );

    m_listBox->setColumnCount( headers.size() );
    m_listBox->setHeaderLabels( headers );
    m_listBox->setSelectionMode( QAbstractItemView::SingleSelection );

    if( aItemHeaders.Count() == 1 )
    {
        m_listBox->setColumnWidth( 0, DEFAULT_SINGLE_COL_WIDTH );
        m_listBox->setMinimumSize( DEFAULT_SINGLE_COL_WIDTH, 200 );
        setMinimumSize( DEFAULT_COL_WIDTHS[0], 220 );
    }
    else if( aItemHeaders.Count() == 2 )
    {
        for( unsigned i = 0; i < aItemHeaders.Count(); i++ )
            m_listBox->setColumnWidth( i, DEFAULT_COL_WIDTHS[i] );

        m_listBox->setMinimumSize( DEFAULT_COL_WIDTHS[0] * 3, 200 );
        setMinimumSize( DEFAULT_COL_WIDTHS[0] * 2, 220 );
    }

    m_itemsList = aItemList;
    InsertItems( m_itemsList, 0 );

    if( !aPreselectText.IsEmpty() )
    {
        QList<QTreeWidgetItem*> found = m_listBox->findItems( ToQString( aPreselectText ),
                                                              Qt::MatchExactly, 0 );

        if( !found.isEmpty() )
        {
            m_listBox->setCurrentItem( found.first() );
            m_listBox->scrollToItem( found.first() );
        }
    }
}


void EDA_LIST_DIALOG::SetListLabel( const wxString& aLabel )
{
    m_listLabel->setText( ToQString( aLabel ) );
    m_listBox->setHeaderHidden( true );
}


void EDA_LIST_DIALOG::SetOKLabel( const wxString& aLabel )
{
    m_sdbSizerOK->setText( ToQString( aLabel ) );
}


void EDA_LIST_DIALOG::HideFilter()
{
    m_filterBox->hide();
}


void EDA_LIST_DIALOG::textChangeInFilterBox( QWidget* aSource )
{
    wxString filter;
    wxString itemName;

    filter = wxT( "*" ) + ToWxString( m_filterBox->text() ).MakeLower() + wxT( "*" );

    m_listBox->clear();

    for( const wxArrayString& row : m_itemsList )
    {
        itemName = row.Item( 0 );

        if( itemName.MakeLower().Matches( filter ) )
            Append( row );
    }

    sortList();
}


long EDA_LIST_DIALOG::GetSelection()
{
    QTreeWidgetItem* item = m_listBox->currentItem();

    if( !item || !item->isSelected() )
        return -1;

    return m_listBox->indexOfTopLevelItem( item );
}


wxString EDA_LIST_DIALOG::GetTextSelection( int aColumn )
{
    KI_CHECK_MSG( unsigned( aColumn ) < unsigned( m_listBox->columnCount() ), wxEmptyString,
                  wxT( "Invalid list control column." ) );

    QTreeWidgetItem* item = m_listBox->currentItem();
    wxString         text;

    if( item && item->isSelected() )     // if something is selected.
    {
        text = ToWxString( item->text( aColumn ) );

        if( text.StartsWith( LibTreePinningSymbol() ) )
            text = text.substr( LibTreePinningSymbol().length() );
    }

    return text;
}


void EDA_LIST_DIALOG::GetExtraCheckboxValues()
{
    for( const auto& [checkbox, valuePtr] : m_extraCheckboxMap )
        *valuePtr = checkbox->isChecked();
}


void EDA_LIST_DIALOG::Append( const wxArrayString& itemList )
{
    QTreeWidgetItem* item = new QTreeWidgetItem( m_listBox );

    for( unsigned i = 0; i < itemList.size(); i++ )
        item->setText( i, ToQString( itemList[i] ) );
}


void EDA_LIST_DIALOG::InsertItems( const std::vector< wxArrayString >& itemList, int position )
{
    for( unsigned row = 0; row < itemList.size(); row++ )
    {
        KI_ASSERT( (int) itemList[row].GetCount() == m_listBox->columnCount() );

        QTreeWidgetItem* item = new QTreeWidgetItem();

        for( unsigned col = 0; col < itemList[row].GetCount(); col++ )
            item->setText( col, ToQString( itemList[row].Item( col ) ) );

        m_listBox->insertTopLevelItem( std::min( (int) row + position,
                                                 m_listBox->topLevelItemCount() ),
                                       item );
    }

    sortList();
}


void EDA_LIST_DIALOG::onListItemActivated( QWidget* aSource )
{
    EndModal( wxID_OK );
}


void EDA_LIST_DIALOG::onSize( QWidget* aSource )
{
    int width = m_listBox->viewport()->width();

    if( m_listBox->columnCount() == 1 )
    {
        m_listBox->setColumnWidth( 0, width );
    }
    else if( m_listBox->columnCount() == 2 )
    {
        int first = KiROUND( width * 0.42 );
        m_listBox->setColumnWidth( 0, first );
        m_listBox->setColumnWidth( 1, width - first );
    }
}


void EDA_LIST_DIALOG::sortList()
{
    if( !m_sortList )
        return;

    // Sort alphabetically (the first column), case insensitive, the numbers by value
    QList<QTreeWidgetItem*> items;

    while( m_listBox->topLevelItemCount() > 0 )
        items.append( m_listBox->takeTopLevelItem( 0 ) );

    std::stable_sort( items.begin(), items.end(),
                      []( QTreeWidgetItem* aItem1, QTreeWidgetItem* aItem2 )
                      {
                          return StrNumCmp( ToWxString( aItem1->text( 0 ) ),
                                            ToWxString( aItem2->text( 0 ) ), true ) < 0;
                      } );

    m_listBox->addTopLevelItems( items );
}
