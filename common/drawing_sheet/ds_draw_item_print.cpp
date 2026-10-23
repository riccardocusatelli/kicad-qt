/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 Jean-Pierre Charras <jp.charras at wanadoo.fr>.
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
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

/*
 * the class DS_DATA_ITEM (and DS_DATA_ITEM_TEXT) defines
 * a basic shape of a drawing sheet (frame references and title block)
 * Basic shapes are line, rect and texts
 * the DS_DATA_ITEM coordinates units is the mm, and are relative to
 * one of 4 page corners.
 *
 * These items cannot be drawn or plot "as this". they should be converted
 * to a "draw list" (DS_DRAW_ITEM_BASE and derived items)

 * The list of these items is stored in a DS_DATA_MODEL instance.
 *
 * When building the draw list:
 * the DS_DATA_MODEL is used to create a DS_DRAW_ITEM_LIST
 *  coordinates are converted to draw/plot coordinates.
 *  texts are expanded if they contain format symbols.
 *  Items with m_RepeatCount > 1 are created m_RepeatCount times
 *
 * the DS_DATA_MODEL is created only once.
 * the DS_DRAW_ITEM_LIST is created each time the drawing sheet is plotted/drawn
 *
 * the DS_DATA_MODEL instance is created from a S expression which
 * describes the drawing sheet (can be the default drawing sheet or a custom file).
 */

/**
 * @file ds_draw_item_print.cpp
 * @brief Printing of the drawing sheet items on a wxDC, GUI only.
 */

#include <drawing_sheet/ds_draw_item.h>

#include <bitmap_base.h>
#include <drawing_sheet/ds_data_item.h>
#include <drawing_sheet/ds_painter.h>
#include <gr_basic.h>
#include <render_settings.h>
#include <wx/dc.h>


void DS_DRAW_ITEM_BASE::PrintWsItem( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset )
{
    switch( Type() )
    {
    case WSG_LINE_T:   static_cast<DS_DRAW_ITEM_LINE*>( this )->PrintWsItem( aSettings, aOffset );   break;
    case WSG_RECT_T:   static_cast<DS_DRAW_ITEM_RECT*>( this )->PrintWsItem( aSettings, aOffset );   break;
    case WSG_POLY_T:   static_cast<DS_DRAW_ITEM_POLYPOLYGONS*>( this )->PrintWsItem( aSettings, aOffset ); break;
    case WSG_TEXT_T:   static_cast<DS_DRAW_ITEM_TEXT*>( this )->PrintWsItem( aSettings, aOffset );   break;
    case WSG_BITMAP_T: static_cast<DS_DRAW_ITEM_BITMAP*>( this )->PrintWsItem( aSettings, aOffset ); break;
    default:           break;   // the page is not printed
    }
}


void DS_DRAW_ITEM_TEXT::PrintWsItem( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset )
{
    COLOR4D color = GetTextColor();

    if( color == COLOR4D::UNSPECIFIED )
        color = aSettings->GetLayerColor( LAYER_DRAWINGSHEET );

    Print( aSettings, aOffset, color );
}


void DS_DRAW_ITEM_POLYPOLYGONS::PrintWsItem( const RENDER_SETTINGS* aSettings,
                                             const VECTOR2I&        aOffset )
{
    wxDC*   DC = aSettings->GetPrintDC();
    COLOR4D color = aSettings->GetLayerColor( LAYER_DRAWINGSHEET );
    int     penWidth = std::max( GetPenWidth(), aSettings->GetDefaultPenWidth() );

    std::vector<VECTOR2I> points_moved;

    for( int idx = 0; idx < m_Polygons.OutlineCount(); ++idx )
    {
        points_moved.clear();
        SHAPE_LINE_CHAIN& outline = m_Polygons.Outline( idx );

        for( int ii = 0; ii < outline.PointCount(); ii++ )
        {
            points_moved.emplace_back( outline.CPoint( ii ).x + aOffset.x,
                                       outline.CPoint( ii ).y + aOffset.y );
        }

        GRPoly( DC, (int) points_moved.size(), &points_moved[0], true, penWidth, color, color );
    }
}


void DS_DRAW_ITEM_RECT::PrintWsItem( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset )
{
    wxDC*   DC = aSettings->GetPrintDC();
    COLOR4D color = aSettings->GetLayerColor( LAYER_DRAWINGSHEET );
    int     penWidth = std::max( GetPenWidth(), aSettings->GetDefaultPenWidth() );

    GRRect( DC, GetStart() + aOffset, GetEnd() + aOffset, penWidth, color );
}


void DS_DRAW_ITEM_LINE::PrintWsItem( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset )
{
    wxDC*   DC = aSettings->GetPrintDC();
    COLOR4D color = aSettings->GetLayerColor( LAYER_DRAWINGSHEET );
    int     penWidth = std::max( GetPenWidth(), aSettings->GetDefaultPenWidth() );

    GRLine( DC, GetStart() + aOffset, GetEnd() + aOffset, penWidth, color );
}


void DS_DRAW_ITEM_BITMAP::PrintWsItem( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset )
{
    DS_DATA_ITEM_BITMAP* bitmap = (DS_DATA_ITEM_BITMAP*) GetPeer();

    if( !bitmap->m_ImageBitmap )
        return;

    bitmap->m_ImageBitmap->DrawBitmap( aSettings->GetPrintDC(), m_pos + aOffset,
                                       aSettings->GetBackgroundColor() );
}


void DS_DRAW_ITEM_LIST::Print( const RENDER_SETTINGS* aSettings )
{
    std::vector<DS_DRAW_ITEM_BASE*> second_items;

    for( DS_DRAW_ITEM_BASE* item = GetFirst(); item; item = GetNext() )
    {
        if( item->Type() == WSG_BITMAP_T )
            item->PrintWsItem( aSettings );
        else
            second_items.push_back( item );
    }

    for( DS_DRAW_ITEM_BASE* item : second_items )
        item->PrintWsItem( aSettings );
}


void PrintDrawingSheet( const RENDER_SETTINGS* aSettings, const PAGE_INFO& aPageInfo,
                        const wxString& aSheetName, const wxString& aSheetPath,
                        const wxString& aFileName, const TITLE_BLOCK& aTitleBlock,
                        const std::map<wxString, wxString>* aProperties, int aSheetCount,
                        const wxString& aPageNumber, double aMils2Iu, const PROJECT* aProject,
                        const wxString& aSheetLayer, bool aIsFirstPage )
{
    DS_DRAW_ITEM_LIST drawList( unityScale );

    drawList.SetDefaultPenSize( aSettings->GetDefaultPenWidth() );
    drawList.SetPlotterMilsToIUfactor( aMils2Iu );
    drawList.SetPageNumber( aPageNumber );
    drawList.SetSheetCount( aSheetCount );
    drawList.SetFileName( aFileName );
    drawList.SetSheetName( aSheetName );
    drawList.SetSheetPath( aSheetPath );
    drawList.SetSheetLayer( aSheetLayer );
    drawList.SetProject( aProject );
    drawList.SetIsFirstPage( aIsFirstPage );
    drawList.SetProperties( aProperties );

    drawList.BuildDrawItemsList( aPageInfo, aTitleBlock );

    // Draw item list
    drawList.Print( aSettings );
}
