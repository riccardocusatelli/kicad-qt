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
 * @file vertex_editor_pane.cpp
 * PCB_VERTEX_EDITOR_PANE of the Qt build (see pcbnew/widgets/vertex_editor_pane.cpp).
 */

#include "vertex_editor_pane.h"

#include <limits>

#include <QHeaderView>
#include <QTableWidget>
#include <QVBoxLayout>

#include <wx/crt.h>
#include <wx/translation.h>

#include <board_item.h>
#include <class_draw_panel_gal.h>
#include <eda_units.h>
#include <pcb_base_edit_frame.h>
#include <pcb_shape.h>
#include <tool/tool_manager.h>
#include <zone.h>

#include <view/view.h>

#include <qt_strings.h>


PCB_VERTEX_EDITOR_PANE::PCB_VERTEX_EDITOR_PANE( PCB_BASE_EDIT_FRAME* aFrame ) :
        QWidget( aFrame ),
        m_frame( aFrame ),
        m_item( nullptr ),
        m_zone( nullptr ),
        m_shape( nullptr ),
        m_grid( nullptr ),
        m_updatingGrid( false )
{
    QVBoxLayout* topSizer = new QVBoxLayout( this );
    topSizer->setContentsMargins( 5, 5, 5, 5 );

    m_grid = new QTableWidget( 0, 2, this );
    m_grid->verticalHeader()->hide();
    m_grid->setHorizontalHeaderLabels( { ToQString( _( "X coord" ) ),
                                         ToQString( _( "Y coord" ) ) } );
    m_grid->horizontalHeader()->setSectionResizeMode( QHeaderView::Stretch );
    m_grid->horizontalHeader()->setMinimumSectionSize( 120 );
    m_grid->verticalHeader()->setSectionResizeMode( QHeaderView::Fixed );
    m_grid->setSelectionBehavior( QAbstractItemView::SelectRows );
    m_grid->setSelectionMode( QAbstractItemView::SingleSelection );
    m_grid->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    m_grid->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );

    topSizer->addWidget( m_grid, 1 );

    QObject::connect( m_grid, &QTableWidget::cellChanged, this,
                      [this]( int aRow, int aCol )
                      {
                          onGridCellChange( aRow, aCol );
                      } );

    QObject::connect( m_grid, &QTableWidget::currentCellChanged, this,
                      [this]( int aRow, int, int, int )
                      {
                          updateHighlight( aRow );
                      } );
}


PCB_VERTEX_EDITOR_PANE::~PCB_VERTEX_EDITOR_PANE()
{
    if( m_frame )
        m_frame->OnVertexEditorPaneClosed( this );
}


void PCB_VERTEX_EDITOR_PANE::SetFocus()
{
    m_grid->setFocus();
}


void PCB_VERTEX_EDITOR_PANE::SetItem( BOARD_ITEM* aItem )
{
    m_item = aItem;
    m_zone = dynamic_cast<ZONE*>( aItem );
    m_shape = dynamic_cast<PCB_SHAPE*>( aItem );

    refreshGrid();

    if( m_grid->rowCount() > 0 )
    {
        m_grid->setCurrentCell( 0, 0 );
        m_grid->selectRow( 0 );
        updateHighlight( 0 );
    }
}


void PCB_VERTEX_EDITOR_PANE::ClearItem()
{
    m_item = nullptr;
    m_zone = nullptr;
    m_shape = nullptr;

    m_updatingGrid = true;
    m_grid->setRowCount( 0 );
    m_rows.clear();
    m_updatingGrid = false;
}


void PCB_VERTEX_EDITOR_PANE::OnSelectionChanged( BOARD_ITEM* aNewItem )
{
    // Check if the new item is a polygon or zone
    if( aNewItem )
    {
        ZONE* zone = dynamic_cast<ZONE*>( aNewItem );
        PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( aNewItem );

        bool isPolygon = ( zone != nullptr ) || ( shape && shape->GetShape() == SHAPE_T::POLY );

        if( isPolygon )
        {
            // Update to the new item
            SetItem( aNewItem );
        }
        else
        {
            // Not a polygon/zone, clear the editor
            ClearItem();
        }
    }
    else
    {
        // Nothing selected, clear the editor
        ClearItem();
    }
}


void PCB_VERTEX_EDITOR_PANE::refreshGrid()
{
    m_updatingGrid = true;

    m_grid->setRowCount( 0 );
    m_rows.clear();

    SHAPE_POLY_SET* poly = getPoly();

    if( poly )
    {
        for( auto it = poly->CIterateWithHoles(); it; ++it )
        {
            m_rows.push_back( it.GetIndex() );
            m_grid->insertRow( m_grid->rowCount() );
            updateRow( m_grid->rowCount() - 1 );
        }
    }

    m_updatingGrid = false;
}


void PCB_VERTEX_EDITOR_PANE::updateRow( int aRow )
{
    if( aRow < 0 || aRow >= m_grid->rowCount() )
        return;

    SHAPE_POLY_SET* poly = getPoly();

    if( !poly || aRow >= (int) m_rows.size() )
        return;

    const SHAPE_POLY_SET::VERTEX_INDEX& idx = m_rows[aRow];
    VECTOR2I vertex = poly->CVertex( idx );

    bool wasUpdating = m_updatingGrid;
    m_updatingGrid = true;

    const ORIGIN_TRANSFORMS::COORD_TYPES_T types[2] = { ORIGIN_TRANSFORMS::ABS_X_COORD,
                                                        ORIGIN_TRANSFORMS::ABS_Y_COORD };
    const int values[2] = { vertex.x, vertex.y };

    for( int col = 0; col < 2; ++col )
    {
        QTableWidgetItem* cell = m_grid->item( aRow, col );

        if( !cell )
        {
            cell = new QTableWidgetItem();
            cell->setTextAlignment( Qt::AlignLeft | Qt::AlignVCenter );
            m_grid->setItem( aRow, col, cell );
        }

        cell->setText( ToQString( formatCoord( values[col], types[col] ) ) );
    }

    m_updatingGrid = wasUpdating;
}


void PCB_VERTEX_EDITOR_PANE::updateHighlight( int aRow )
{
    // Highlighting is now handled by the point editor tool
    // We just refresh the view
    if( m_frame && m_frame->GetCanvas() && m_item )
    {
        m_frame->GetCanvas()->GetView()->Update( m_item );
        m_frame->GetCanvas()->Refresh();
    }
}


bool PCB_VERTEX_EDITOR_PANE::parseCellValue( const wxString& aText,
                                             ORIGIN_TRANSFORMS::COORD_TYPES_T aCoordType,
                                             int& aResult ) const
{
    // ValueFromString() returns 0 both for a literal "0" and for unparseable input, so it cannot
    // by itself tell a valid zero from garbage. Require the leading numeric token to hold at least
    // one digit before trusting the parse; otherwise blank or non-numeric entries would silently
    // collapse the vertex onto the display origin instead of being rejected.
    bool hasDigit = false;

    for( wxUniChar ch : aText.Strip( wxString::both ) )
    {
        if( wxIsdigit( ch ) )
        {
            hasDigit = true;
            break;
        }

        if( ch != '+' && ch != '-' && ch != '.' && ch != ',' )
            break;
    }

    if( !hasDigit )
        return false;

    long long displayValue = EDA_UNIT_UTILS::UI::ValueFromString( pcbIUScale, m_frame->GetUserUnits(), aText );

    long long internal = m_frame->GetOriginTransforms().FromDisplay( displayValue, aCoordType );

    if( internal < std::numeric_limits<int>::min() || internal > std::numeric_limits<int>::max() )
        return false;

    aResult = static_cast<int>( internal );
    return true;
}


wxString PCB_VERTEX_EDITOR_PANE::formatCoord( int aValue,
                                              ORIGIN_TRANSFORMS::COORD_TYPES_T aCoordType ) const
{
    int displayValue = m_frame->GetOriginTransforms().ToDisplay( aValue, aCoordType );
    return m_frame->MessageTextFromValue( displayValue );
}


SHAPE_POLY_SET* PCB_VERTEX_EDITOR_PANE::getPoly()
{
    if( m_zone )
        return m_zone->Outline();

    if( m_shape && m_shape->GetShape() == SHAPE_T::POLY )
        return &m_shape->GetPolyShape();

    return nullptr;
}


const SHAPE_POLY_SET* PCB_VERTEX_EDITOR_PANE::getPoly() const
{
    if( m_zone )
        return m_zone->Outline();

    if( m_shape && m_shape->GetShape() == SHAPE_T::POLY )
        return &m_shape->GetPolyShape();

    return nullptr;
}


void PCB_VERTEX_EDITOR_PANE::onGridCellChange( int aRow, int aCol )
{
    if( m_updatingGrid )
        return;

    if( aRow < 0 || aRow >= (int) m_rows.size() )
    {
        updateHighlight( -1 );
        return;
    }

    SHAPE_POLY_SET* poly = getPoly();

    if( !poly || !m_item )
        return;

    const SHAPE_POLY_SET::VERTEX_INDEX& idx = m_rows[aRow];
    VECTOR2I vertex = poly->CVertex( idx );
    int newValue;

    ORIGIN_TRANSFORMS::COORD_TYPES_T coordType =
            ( aCol == 0 ) ? ORIGIN_TRANSFORMS::ABS_X_COORD : ORIGIN_TRANSFORMS::ABS_Y_COORD;

    QTableWidgetItem* cell = m_grid->item( aRow, aCol );

    if( !cell || !parseCellValue( ToWxString( cell->text() ), coordType, newValue ) )
    {
        updateRow( aRow );
        return;
    }

    int& target = ( aCol == 0 ) ? vertex.x : vertex.y;

    if( target == newValue )
    {
        updateRow( aRow );
        return;
    }

    // Create a new commit for each edit
    BOARD_COMMIT commit( m_frame );
    commit.Modify( m_item );

    target = newValue;
    poly->SetVertex( idx, vertex );

    if( m_zone )
    {
        m_zone->UnFill();
        m_zone->SetNeedRefill( true );
        m_zone->HatchBorder();
    }

    commit.Push( _( "Edit Vertex" ) );

    // Update the view after the commit
    if( m_frame && m_frame->GetCanvas() )
    {
        m_frame->GetCanvas()->GetView()->Update( m_item );
        m_frame->GetCanvas()->Refresh();
    }

    updateRow( aRow );
    updateHighlight( aRow );
}
