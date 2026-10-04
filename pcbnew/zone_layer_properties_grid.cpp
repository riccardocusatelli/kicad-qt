/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#include <zone_layer_properties_grid.h>

#include <board.h>
#include <lset.h>
#include <pcb_base_frame.h>
#include <settings/color_settings.h>
#include <widgets/color_swatch.h>
#include <zone_settings.h>
#include <wx/dataview.h>
#include <core/ki_assert.h>


#ifdef __WXOSX_MAC__
const static wxSize LAYER_BITMAP_SIZE( 28, 28 );  // wxCocoa impl unhappy if this isn't square...
#else
const static wxSize LAYER_BITMAP_SIZE( 24, 16 );
#endif

const static wxSize CHECKERBOARD_SIZE( 8, 8 );


void ZONE_SETTINGS::SetupLayersList( wxDataViewListCtrl* aList, PCB_BASE_FRAME* aFrame, LSET aLayers )
{
    BOARD* board = aFrame->GetBoard();
    COLOR4D backgroundColor = aFrame->GetColorSettings()->GetColor( LAYER_PCB_BACKGROUND );

    wxDataViewColumn* checkColumn = aList->AppendToggleColumn( wxEmptyString, wxDATAVIEW_CELL_ACTIVATABLE,
                                                               wxCOL_WIDTH_DEFAULT, wxALIGN_CENTER );

    wxDataViewColumn* layerColumn = aList->AppendIconTextColumn( wxEmptyString );
    wxDataViewColumn* layerIDColumn = aList->AppendTextColumn( wxEmptyString );
    layerIDColumn->SetHidden( true );

    int textWidth = 0;

    for( PCB_LAYER_ID layerID : aLayers.UIOrder() )
    {
        wxString layerName = board->GetLayerName( layerID );

        // wxCOL_WIDTH_AUTOSIZE doesn't work on all platforms, so we calculate width here
        textWidth = std::max( textWidth, KIUI::GetTextSize( layerName, aList ).x );

        COLOR4D layerColor = aFrame->GetColorSettings()->GetColor( layerID );
        auto bitmap = COLOR_SWATCH::MakeBitmap( layerColor, backgroundColor, LAYER_BITMAP_SIZE,
                                                CHECKERBOARD_SIZE, aList->GetBackgroundColour() );
        wxIcon icon;
        icon.CopyFromBitmap( bitmap );

        wxVector<wxVariant> row;
        row.push_back( wxVariant( m_Layers.test( layerID ) ) );
        row.push_back( wxVariant( wxDataViewIconText( layerName, icon ) ) );
        row.push_back( wxVariant( wxString::Format( wxT( "%i" ), layerID ) ) );
        aList->AppendItem( row );

        if( m_Layers.test( layerID ) )
            aList->SetToggleValue( true, (unsigned) aList->GetItemCount() - 1, 0 );
    }

    int checkColSize = aList->FromDIP( 22 );
    int layerColSize = textWidth + LAYER_BITMAP_SIZE.x + aList->FromDIP( 15 );

    // You'd think the fact that m_layers is a list would encourage wxWidgets not to save room
    // for the tree expanders... but you'd be wrong.  Force indent to 0.
    aList->SetIndent( 0 );

    checkColumn->SetWidth( checkColSize );
    layerColumn->SetWidth( layerColSize );

#ifdef __WXMAC__
    // TODO: something in wxWidgets 3.1.x pads checkbox columns with extra space.  (It used to
    // also be that the width of the column would get set too wide (to 30), but that's patched in
    // our local wxWidgets fork.)
    checkColSize += 50;
#endif

    aList->SetMinClientSize( wxSize( checkColSize + layerColSize, aList->GetMinClientSize().y ) );
}


LAYER_PROPERTIES_GRID_TABLE::LAYER_PROPERTIES_GRID_TABLE( PCB_BASE_FRAME* aFrame, std::function<LSET()> getLayers ) :
        m_frame( aFrame ),
        m_getLayersFunc( getLayers )
{
    m_frame->Bind( EDA_EVT_UNITS_CHANGED, &LAYER_PROPERTIES_GRID_TABLE::onUnitsChanged, this );
}


LAYER_PROPERTIES_GRID_TABLE::~LAYER_PROPERTIES_GRID_TABLE()
{
    m_frame->Unbind( EDA_EVT_UNITS_CHANGED, &LAYER_PROPERTIES_GRID_TABLE::onUnitsChanged, this );
}


wxString LAYER_PROPERTIES_GRID_TABLE::GetValue( int aRow, int aCol )
{
    VECTOR2I offset = m_items[aRow].second.hatching_offset.value_or( VECTOR2I() );

    switch( aCol )
    {
    case 1:  return m_frame->StringFromValue( offset.x, true );
    case 2:  return m_frame->StringFromValue( offset.y, true );

    default:
        // we can't assert here because wxWidgets sometimes calls this without checking
        // the column type when trying to see if there's an overflow
        return wxT( "bad wxWidgets!" );
    }
}


void LAYER_PROPERTIES_GRID_TABLE::SetValue( int aRow, int aCol, const wxString& aValue )
{
    VECTOR2I offset = m_items[aRow].second.hatching_offset.value_or( VECTOR2I() );

    switch( aCol )
    {
    case 1:  offset.x = m_frame->ValueFromString( aValue );                           break;
    case 2:  offset.y = m_frame->ValueFromString( aValue );                           break;
    default: KI_FAIL_MSG( wxString::Format( wxT( "column %d isn't a long" ), aCol ) ); break;
    }

    m_items[aRow].second.hatching_offset = offset;
}


long LAYER_PROPERTIES_GRID_TABLE::GetValueAsLong( int aRow, int aCol )
{
    switch( aCol )
    {
    case 0:  return m_items[aRow].first;
    default: KI_FAIL_MSG( wxString::Format( wxT( "column %d isn't a long" ), aCol ) ); return -1;
    }
}


void LAYER_PROPERTIES_GRID_TABLE::SetValueAsLong( int aRow, int aCol, long aValue )
{
    switch( aCol )
    {
    case 0:  m_items[aRow].first = ToLAYER_ID( (int) aValue );                        break;
    default: KI_FAIL_MSG( wxString::Format( wxT( "column %d isn't a long" ), aCol ) ); break;
    }
}


void LAYER_PROPERTIES_GRID_TABLE::AddItem( PCB_LAYER_ID aLayer, const ZONE_LAYER_PROPERTIES& aProps )
{
    m_items.emplace_back( std::make_pair( aLayer, aProps ) );

    if( GetView() )
    {
        wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, 1 );
        GetView()->ProcessTableMessage( msg );
    }
}


bool LAYER_PROPERTIES_GRID_TABLE::AppendRows( size_t aNumRows )
{
    PCB_LAYER_ID next = UNDEFINED_LAYER;

    auto contains =
            [&]( PCB_LAYER_ID candidate )
            {
                for( const auto& [l, props] : m_items )
                {
                    if( candidate == l )
                        return true;
                }

                return false;
            };

    for( PCB_LAYER_ID layer : m_getLayersFunc().UIOrder() )
    {
        if( !contains( layer ) )
        {
            next = layer;
            break;
        }
    }

    if( next == UNDEFINED_LAYER )
        return false;

    AddItem( next, ZONE_LAYER_PROPERTIES( { VECTOR2I( 0, 0 ) } ) );

    return true;
}


bool LAYER_PROPERTIES_GRID_TABLE::DeleteRows( size_t aPos, size_t aNumRows )
{
    // aPos may be a large positive, e.g. size_t(-1), and the sum of
    // aPos+aNumRows may wrap here, so both ends of the range are tested.
    if( aPos < m_items.size() && aPos + aNumRows <= m_items.size() )
    {
        m_items.erase( m_items.begin() + (int) aPos, m_items.begin() + (int) aPos + (int) aNumRows );

        if( GetView() )
        {
            wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, (int) aPos, (int) aNumRows );
            GetView()->ProcessTableMessage( msg );
        }

        return true;
    }

    return false;
}


void LAYER_PROPERTIES_GRID_TABLE::onUnitsChanged( wxCommandEvent& aEvent )
{
    if( GetView() )
        GetView()->ForceRefresh();

    aEvent.Skip();
}


