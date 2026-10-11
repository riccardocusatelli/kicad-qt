/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file dialog_design_block_properties_qt.cpp
 * DIALOG_DESIGN_BLOCK_PROPERTIES with Qt (common/dialogs/dialog_design_block_properties.cpp with
 * wxWidgets): its fields grid is a QTableWidget, edited in place.
 */

#include <dialogs/dialog_design_block_properties.h>

#include <QHeaderView>
#include <QLineEdit>
#include <QPlainTextEdit>

#include <bitmaps.h>
#include <confirm.h>
#include <design_block.h>
#include <lib_id.h>
#include <widgets/std_bitmap_button.h>
#include <widgets/wx_grid.h>

#include <qt_strings.h>


DIALOG_DESIGN_BLOCK_PROPERTIES::DIALOG_DESIGN_BLOCK_PROPERTIES( QWidget*      aParent,
                                                                DESIGN_BLOCK* aDesignBlock,
                                                                bool          aDisableName ) :
        DIALOG_DESIGN_BLOCK_PROPERTIES_BASE( aParent ),
        m_designBlock( aDesignBlock )
{
    m_textName->setReadOnly( aDisableName );

    SetupStandardButtons();

    // Configure button logos
    m_bpAdd->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_bpDelete->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_bpMoveUp->SetBitmap( KiBitmapBundle( BITMAPS::small_up ) );
    m_bpMoveDown->SetBitmap( KiBitmapBundle( BITMAPS::small_down ) );

    // The fields are edited in place; the value column takes the width of the grid
    m_fieldsGrid->setEditTriggers( QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed
                                   | QAbstractItemView::AnyKeyPressed );
    m_fieldsGrid->horizontalHeader()->setStretchLastSection( true );
}


DIALOG_DESIGN_BLOCK_PROPERTIES::~DIALOG_DESIGN_BLOCK_PROPERTIES()
{
}


bool DIALOG_DESIGN_BLOCK_PROPERTIES::TransferDataToWindow()
{
    m_textName->setText( ToQString( m_designBlock->GetLibId().GetLibItemName() ) );
    m_textKeywords->setText( ToQString( m_designBlock->GetKeywords() ) );
    m_textDescription->setPlainText( ToQString( m_designBlock->GetLibDescription() ) );

    // Typical assignment operator does not work here because of the ordered_map
    auto source = m_designBlock->GetFields();
    m_fields.clear();

    for( const auto& field : source )
        m_fields[field.first] = field.second;

    return TransferDataToGrid();
}


bool DIALOG_DESIGN_BLOCK_PROPERTIES::TransferDataFromWindow()
{
    wxString     name = ToWxString( m_textName->text() );
    unsigned int illegalCh = LIB_ID::FindIllegalLibraryNameChar( name );

    // Also check for / in the name, since this is a path character which is illegal for
    // design blocks and footprints but not symbols
    if( illegalCh == 0 && name.Find( '/' ) != wxNOT_FOUND )
        illegalCh = '/';

    if( illegalCh )
    {
        DisplayError( this, wxString::Format( _( "Illegal character '%c' in name '%s'." ), illegalCh, name ) );
        return false;
    }

    m_designBlock->SetLibId( LIB_ID( m_designBlock->GetLibId().GetLibNickname(), name ) );
    m_designBlock->SetLibDescription( ToWxString( m_textDescription->toPlainText() ) );
    m_designBlock->SetKeywords( ToWxString( m_textKeywords->text() ) );

    return TransferDataFromGrid();
}


void DIALOG_DESIGN_BLOCK_PROPERTIES::OnAddField( QWidget* aSource )
{
    int row = m_fieldsGrid->rowCount();
    m_fieldsGrid->insertRow( row );

    m_fieldsGrid->setItem( row, 0, new QTableWidgetItem( ToQString( _( "Untitled Field" ) ) ) );
    m_fieldsGrid->setItem( row, 1, new QTableWidgetItem() );

    m_fieldsGrid->setCurrentCell( row, 0 );
    m_fieldsGrid->editItem( m_fieldsGrid->item( row, 0 ) );
}


void DIALOG_DESIGN_BLOCK_PROPERTIES::OnDeleteField( QWidget* aSource )
{
    std::vector<int> rows;

    for( const QModelIndex& index : m_fieldsGrid->selectionModel()->selectedRows() )
        rows.push_back( index.row() );

    if( rows.empty() && m_fieldsGrid->currentRow() >= 0 )
        rows.push_back( m_fieldsGrid->currentRow() );

    // From the last row, so that the other rows keep their numbers
    std::sort( rows.rbegin(), rows.rend() );

    for( int row : rows )
        m_fieldsGrid->removeRow( row );
}


void DIALOG_DESIGN_BLOCK_PROPERTIES::OnMoveFieldUp( QWidget* aSource )
{
    int row = m_fieldsGrid->currentRow();

    if( row > 0 )
    {
        swapRows( row, row - 1 );
        m_fieldsGrid->setCurrentCell( row - 1, m_fieldsGrid->currentColumn() );
    }
}


void DIALOG_DESIGN_BLOCK_PROPERTIES::OnMoveFieldDown( QWidget* aSource )
{
    int row = m_fieldsGrid->currentRow();

    if( row >= 0 && row + 1 < m_fieldsGrid->rowCount() )
    {
        swapRows( row, row + 1 );
        m_fieldsGrid->setCurrentCell( row + 1, m_fieldsGrid->currentColumn() );
    }
}


void DIALOG_DESIGN_BLOCK_PROPERTIES::swapRows( int aRow1, int aRow2 )
{
    for( int col = 0; col < m_fieldsGrid->columnCount(); ++col )
    {
        QTableWidgetItem* item1 = m_fieldsGrid->takeItem( aRow1, col );
        QTableWidgetItem* item2 = m_fieldsGrid->takeItem( aRow2, col );

        m_fieldsGrid->setItem( aRow1, col, item2 );
        m_fieldsGrid->setItem( aRow2, col, item1 );
    }
}


bool DIALOG_DESIGN_BLOCK_PROPERTIES::TransferDataToGrid()
{
    m_fieldsGrid->setRowCount( 0 );
    m_fieldsGrid->setRowCount( static_cast<int>( m_fields.size() ) );

    int row = 0;

    for( const auto& [fieldName, fieldValue] : m_fields )
    {
        m_fieldsGrid->setItem( row, 0, new QTableWidgetItem( ToQString( fieldName ) ) );
        m_fieldsGrid->setItem( row, 1, new QTableWidgetItem( ToQString( fieldValue ) ) );
        row++;
    }

    return true;
}


bool DIALOG_DESIGN_BLOCK_PROPERTIES::TransferDataFromGrid()
{
    // The cell being edited
    if( QWidget* editor = m_fieldsGrid->focusWidget(); editor && editor != m_fieldsGrid )
        m_fieldsGrid->setFocus();

    m_designBlock->GetFields().clear();

    auto cellText =
            [this]( int aRow, int aCol ) -> wxString
            {
                QTableWidgetItem* item = m_fieldsGrid->item( aRow, aCol );
                return item ? ToWxString( item->text() ) : wxString();
            };

    for( int row = 0; row < m_fieldsGrid->rowCount(); row++ )
    {
        wxString fieldName = cellText( row, 0 ).Strip();
        fieldName.Replace( wxT( "\n" ), wxT( "" ), true );  // strip all newlines
        fieldName.Replace( wxT( "  " ), wxT( " " ), true ); // double space to single

        if( m_designBlock->GetFields().count( fieldName ) )
        {
            DisplayError( this, _( "Duplicate fields are not allowed." ) );
            return false;
        }

        m_designBlock->GetFields()[fieldName] = cellText( row, 1 );
    }

    return true;
}
