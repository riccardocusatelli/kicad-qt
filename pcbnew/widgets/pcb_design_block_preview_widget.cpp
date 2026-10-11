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
 * @file pcb_design_block_preview_widget.cpp
 * What PCB_DESIGN_BLOCK_PREVIEW_WIDGET does without the GUI toolkit: the board of a design block
 * on the canvas, fitted to it.  The widgets are in pcb_design_block_preview_widget_wx.cpp (and in
 * qt/pcbnew/widgets/pcb_design_block_preview_widget_qt.cpp).
 */

#include <board.h>
#include <confirm.h>
#include <pcb_view.h>
#include <gal/graphics_abstraction_layer.h>
#include <math/vector2wx.h>
#include <design_block.h>
#include <pcb_draw_panel_gal.h>
#include <pcb_field.h>
#include <widgets/pcb_design_block_preview_widget.h>
#include <wx/filefn.h>
#include <core/ki_assert.h>



PCB_DESIGN_BLOCK_PREVIEW_WIDGET::~PCB_DESIGN_BLOCK_PREVIEW_WIDGET()
{
    if( m_previewItem )
        m_preview->GetView()->Remove( m_previewItem );

    delete m_previewItem;
}


void PCB_DESIGN_BLOCK_PREVIEW_WIDGET::fitOnDrawArea()
{
    if( !m_previewItem )
        return;

    // set the view scale to fit the item on screen
    KIGFX::VIEW* view = m_preview->GetView();

    // Calculate the drawing area size, in internal units, for a scaling factor = 1.0
    view->SetScale( 1.0 );
    VECTOR2D clientSize = view->ToWorld( ToVECTOR2D( m_preview->GetClientSize() ), false );
    // Calculate the draw scale to fit the drawing area
    double scale =
            std::min( fabs( clientSize.x / m_itemBBox.GetWidth() ), fabs( clientSize.y / m_itemBBox.GetHeight() ) );

    // Above calculation will yield an exact fit; add a bit of whitespace around block
    scale /= 1.2;

    // Now fix the best scale
    view->SetScale( scale );
    view->SetCenter( m_itemBBox.Centre() );
}


void PCB_DESIGN_BLOCK_PREVIEW_WIDGET::DisplayDesignBlock( DESIGN_BLOCK* aDesignBlock )
{
    KIGFX::VIEW* view = m_preview->GetView();

    if( m_previewItem )
    {
        view->Clear();
        delete m_previewItem;
        m_previewItem = nullptr;
    }

    if( aDesignBlock && wxFileExists( aDesignBlock->GetBoardFile() ) )
    {
        try
        {
            m_previewItem = loadBoard( aDesignBlock->GetBoardFile() );
        }
        catch( const IO_ERROR& ioe )
        {
            // You wouldn't think boardFn.GetFullPath() would throw, but we get a stack buffer
            // underflow from ASAN.  While it's probably an ASAN error, a second try/catch doesn't
            // cost us much.
            try
            {
                if( ioe.Problem() != wxT( "CANCEL" ) )
                {
                    wxString msg =
                            wxString::Format( _( "Error loading board file:\n%s" ), aDesignBlock->GetBoardFile() );
                    DisplayErrorMessage( this, msg, ioe.What() );
                }
            }
            catch( ... )
            {
                // That was already our best-efforts
            }
        }


        BOX2I bBox;

        if( m_previewItem )
        {
            // The preview panel was built around a 2-layer dummy board. Re-sync layer
            // visibility to the loaded block so inner copper layers are shown.
            static_cast<PCB_DRAW_PANEL_GAL*>( m_preview )->SyncLayersVisibility( m_previewItem );

            for( BOARD_ITEM* item : m_previewItem->GetItemSet() )
            {
                view->Add( item );

                if( item->Type() == PCB_FIELD_T )
                {
                    if( !static_cast<const PCB_FIELD*>( item )->IsVisible() )
                        continue;
                }

                bBox.Merge( item->GetBoundingBox() );
            }
        }

        m_itemBBox = bBox;

        // The canvas in place of the status, with its size up to date
        showCanvas();

        // Calculate the draw scale to fit the drawing area
        fitOnDrawArea();
    }

    m_preview->ForceRefresh();
    showCanvas();
}
