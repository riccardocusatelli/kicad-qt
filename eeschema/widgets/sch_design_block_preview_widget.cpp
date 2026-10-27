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
 * @file sch_design_block_preview_widget.cpp
 * What SCH_DESIGN_BLOCK_PREVIEW_WIDGET does without the GUI toolkit: the schematic of a design
 * block on the canvas, fitted to it.  The widgets are in sch_design_block_preview_widget_wx.cpp
 * (and in qt/eeschema/widgets/sch_design_block_preview_widget_qt.cpp).
 */

#include <schematic.h>
#include <sch_sheet.h>
#include <sch_view.h>
#include <sch_screen.h>
#include <gal/graphics_abstraction_layer.h>
#include <math/vector2wx.h>
#include <design_block.h>
#include <eeschema_helpers.h>
#include <sch_field.h>
#include <widgets/sch_design_block_preview_widget.h>
#include <wx/filefn.h>


SCH_DESIGN_BLOCK_PREVIEW_WIDGET::~SCH_DESIGN_BLOCK_PREVIEW_WIDGET()
{
    if( m_previewItem )
        m_preview->GetView()->Remove( m_previewItem );

    delete m_previewItem;
}


void SCH_DESIGN_BLOCK_PREVIEW_WIDGET::fitOnDrawArea()
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


void SCH_DESIGN_BLOCK_PREVIEW_WIDGET::DisplayDesignBlock( DESIGN_BLOCK* aDesignBlock )
{
    KIGFX::VIEW* view = m_preview->GetView();

    if( m_previewItem )
    {
        view->Clear();
        view->MarkDirty();
        delete m_previewItem;
        m_previewItem = nullptr;
    }

    if( aDesignBlock && wxFileExists( aDesignBlock->GetSchematicFile() ) )
    {
        m_previewItem = EESCHEMA_HELPERS::LoadSchematic( aDesignBlock->GetSchematicFile(), SCH_IO_MGR::SCH_KICAD,
                                                         false, true, nullptr, false );
        BOX2I bBox;

        if( m_previewItem )
        {
            for( EDA_ITEM* item : m_previewItem->CurrentSheet().LastScreen()->Items() )
            {
                view->Add( item );

                if( item->Type() == SCH_FIELD_T )
                {
                    if( !static_cast<const SCH_FIELD*>( item )->IsVisible() )
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
