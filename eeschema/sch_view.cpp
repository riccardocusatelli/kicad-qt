/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2018 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include <core/typeinfo.h>
#include <memory>
#include <view/view.h>
#include <view/view_group.h>
#include <view/view_rtree.h>
#include <drawing_sheet/ds_proxy_view_item.h>
#include <eda_text.h>
#include <layer_ids.h>
#include <lib_symbol.h>
#include <sch_connection.h>
#include <sch_screen.h>
#include <sch_shape.h>
#include <schematic.h>
#include <string_utils.h>

#include "sch_view.h"
#include <core/ki_assert.h>


namespace KIGFX {


SCH_VIEW::SCH_VIEW( SCH_VIEW_HOST* aHost ) :
    VIEW(),
    m_host( aHost )
{
}


SCH_VIEW::~SCH_VIEW()
{
}


void SCH_VIEW::Update( const KIGFX::VIEW_ITEM* aItem, int aUpdateFlags ) const
{
    if( aItem->IsSCH_ITEM() )
    {
        // The equivalent function in PCB_VIEW doesn't need to do this, but
        // that's only because RunOnChildren's constness is misleading in the PCB editor;
        // it doesn't modify target item, but our uses of it modify its children.
        SCH_ITEM* schItem = const_cast<SCH_ITEM*>( static_cast<const SCH_ITEM*>( aItem ) );

        if( schItem->Type() == SCH_TABLECELL_T )
        {
            VIEW::Update( schItem->GetParent() );
        }
        else
        {
            schItem->RunOnChildren(
                    [this, aUpdateFlags]( SCH_ITEM* child )
                    {
                        VIEW::Update( child, aUpdateFlags );
                    },
                    RECURSE_MODE::RECURSE );
        }
    }

    VIEW::Update( aItem, aUpdateFlags );
}


void SCH_VIEW::Update( const KIGFX::VIEW_ITEM* aItem ) const
{
    SCH_VIEW::Update( aItem, KIGFX::ALL );
}


void SCH_VIEW::Cleanup()
{
    Clear();
    m_drawingSheet.reset();
    m_preview.reset();
}


void SCH_VIEW::SetScale( double aScale, VECTOR2D aAnchor )
{
    VIEW::SetScale( aScale, aAnchor );

    // Redraw items whose rendering is dependent on zoom
    if( m_host )
        m_host->RefreshZoomDependentItems();
}


void SCH_VIEW::DisplaySheet( const SCH_SCREEN *aScreen )
{
    for( SCH_ITEM* item : aScreen->Items() )
        Add( item );

    m_drawingSheet.reset( new DS_PROXY_VIEW_ITEM( schIUScale, &aScreen->GetPageSettings(),
                                                  &aScreen->Schematic()->Project(),
                                                  &aScreen->GetTitleBlock(),
                                                  aScreen->Schematic()->GetProperties() ) );
    m_drawingSheet->SetPageNumber( TO_UTF8( aScreen->GetPageNumber() ) );
    m_drawingSheet->SetSheetCount( aScreen->GetPageCount() );
    m_drawingSheet->SetFileName( TO_UTF8( aScreen->GetFileName() ) );
    m_drawingSheet->SetColorLayer( LAYER_SCHEMATIC_DRAWINGSHEET );
    m_drawingSheet->SetPageBorderColorLayer( LAYER_SCHEMATIC_PAGE_LIMITS );

    wxString currentVariant = aScreen->Schematic()->GetCurrentVariant();
    wxString variantDesc = aScreen->Schematic()->GetVariantDescription( currentVariant );
    m_drawingSheet->SetVariantName( TO_UTF8( currentVariant ) );
    m_drawingSheet->SetVariantDesc( TO_UTF8( variantDesc ) );

    SCH_CURRENT_SHEET_INFO currentSheet;

    if( m_host && m_host->GetCurrentSheetInfo( currentSheet ) )
    {
        // The title block metadata (sheet name/path, first-page flag) is derived from the
        // current sheet, so the screen being displayed must be the current sheet's screen.
        KI_CHECK( currentSheet.m_SheetScreen == aScreen, /* void */ );

        syncDrawingSheetToCurrentSheet( currentSheet );
    }
    else
    {
        m_drawingSheet->SetIsFirstPage( aScreen->GetVirtualPageNumber() == 1 );
        m_drawingSheet->SetSheetName( "" );
        m_drawingSheet->SetSheetPath( "" );
    }

    Add( m_drawingSheet.get() );

    InitPreview();

    // Allow tools to add anything they require to the view (such as the selection VIEW_GROUP)
    if( m_host )
        m_host->OnSheetDisplayed();
}


void SCH_VIEW::syncDrawingSheetToCurrentSheet( const SCH_CURRENT_SHEET_INFO& aInfo )
{
    SCH_SCREEN* screen = aInfo.m_Screen;

    KI_CHECK( screen, /* void */ );

    m_drawingSheet->SetPageNumber( TO_UTF8( screen->GetPageNumber() ) );
    m_drawingSheet->SetSheetCount( screen->GetPageCount() );
    m_drawingSheet->SetFileName( TO_UTF8( screen->GetFileName() ) );

    // Use the sheet path's virtual page number rather than the screen's, because the screen's
    // value can be stale after operations like save that temporarily overwrite all screen page
    // numbers for serialization.
    m_drawingSheet->SetIsFirstPage( aInfo.m_VirtualPageNumber == 1 );
    m_drawingSheet->SetSheetName( TO_UTF8( aInfo.m_Name ) );
    m_drawingSheet->SetSheetPath( TO_UTF8( aInfo.m_Path ) );
}


void SCH_VIEW::RefreshDrawingSheetPageInfo()
{
    if( !m_drawingSheet )
        return;

    SCH_CURRENT_SHEET_INFO currentSheet;

    if( m_host && m_host->GetCurrentSheetInfo( currentSheet ) )
    {
        syncDrawingSheetToCurrentSheet( currentSheet );
        Update( m_drawingSheet.get(), KIGFX::REPAINT );
    }
}


void SCH_VIEW::DisplaySymbol( LIB_SYMBOL* aSymbol )
{
    Clear();

    if( !aSymbol )
        return;

    // Draw the fields.
    for( SCH_ITEM& item : aSymbol->GetDrawItems() )
    {
        if( item.Type() == SCH_FIELD_T )
            Add( &item );
    }

    LIB_SYMBOL* drawnSymbol = aSymbol;

    // Draw the parent items if the symbol is inherited from another symbol.
    if( aSymbol->IsDerived() )
    {
        if( std::shared_ptr< LIB_SYMBOL > parent = aSymbol->GetRootSymbol() )
            drawnSymbol = parent.get();
        else
        {
            KI_CHECK( false, /* void */ );
        }
    }

    for( SCH_ITEM& item : drawnSymbol->GetDrawItems() )
    {
        // Fields already drawn above.  (Besides, we don't want to show parent symbol fields as
        // users may be confused by shown fields that can not be edited.)
        if( item.Type() == SCH_FIELD_T )
            continue;

        Add( &item );
    }

    InitPreview();
}


void SCH_VIEW::UpdateWithParent( EDA_ITEM* aItem, bool aIsAddOrDelete )
{
    EDA_ITEM* parent = aItem->GetParent();

    if( aItem->Type() == SCH_SHEET_PIN_T )
    {
        // Sheet pins aren't in the view.  Refresh their parent.
        if( parent )
            Update( parent );

        return;
    }

    if( aItem->Type() == SCH_SHAPE_T )
        static_cast<SCH_SHAPE*>( aItem )->UpdateHatching();

    if( !aIsAddOrDelete )
        Update( aItem );

    // Some children are drawn from their parents.  Mark them for re-paint.
    if( parent && ( parent->Type() == SCH_SYMBOL_T
                    || parent->Type() == SCH_SHEET_T
                    || parent->Type() == SCH_LABEL_LOCATE_ANY_T
                    || parent->Type() == SCH_TABLE_T ) )
    {
        Update( parent, KIGFX::REPAINT );
    }
}


void SCH_VIEW::UpdateHopOveredWires( SCH_ITEM* aItem )
{
    std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> items;

    Query( aItem->GetBoundingBox(), items );

    for( const auto& it : items )
    {
        if( !it.first->IsSCH_ITEM() )
            continue;

        SCH_ITEM* item = static_cast<SCH_ITEM*>( it.first );

        if( item == aItem )
            continue;

        if( item->IsType( { SCH_ITEM_LOCATE_WIRE_T, SCH_ITEM_LOCATE_BUS_T } ) )
            Update( item );
    }
}


void SCH_VIEW::UpdateConnectionChanges( SCH_SCREEN* aScreen )
{
    UpdateAllItemsConditionally(
            [&]( KIGFX::VIEW_ITEM* aItem ) -> int
            {
                int             flags = 0;
                SCH_ITEM*       item = dynamic_cast<SCH_ITEM*>( aItem );
                SCH_CONNECTION* connection = item ? item->Connection() : nullptr;

                auto invalidateTextVars =
                        [&flags]( EDA_TEXT* text )
                        {
                            if( text->HasTextVars() )
                            {
                                text->ClearRenderCache();
                                text->ClearBoundingBoxCache();
                                flags |= KIGFX::GEOMETRY | KIGFX::REPAINT;
                            }
                        };

                if( connection && connection->HasDriverChanged() )
                {
                    connection->ClearDriverChanged();
                    flags |= KIGFX::REPAINT;
                }

                if( item )
                {
                    item->RunOnChildren(
                            [&invalidateTextVars]( SCH_ITEM* aChild )
                            {
                                if( EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( aChild ) )
                                    invalidateTextVars( text );
                            },
                            RECURSE_MODE::NO_RECURSE );

                    if( flags & KIGFX::GEOMETRY )
                        aScreen->Update( item, false );     // Refresh RTree
                }

                if( EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( aItem ) )
                    invalidateTextVars( text );

                return flags;
            } );
}


void SCH_VIEW::ClearHiddenFlags()
{
    for( VIEW_ITEM* item : *m_allItems )
    {
        if( !item )
            continue;

        Hide( item, false );
    }
}


void SCH_VIEW::HideDrawingSheet()
{
    //    SetVisible( m_drawingSheet.get(), false );
}


void SCH_VIEW::SetDefaultLayerOrder()
{
    for( int i = 0; (unsigned) i < sizeof( SCH_LAYER_ORDER ) / sizeof( int ); ++i )
    {
        int layer = SCH_LAYER_ORDER[i];
        KI_ASSERT( layer < VIEW::VIEW_MAX_LAYERS );

        SetLayerOrder( layer, i );
    }
}


void SCH_VIEW::SetDefaultLayerDeps( bool aCached )
{
    // caching makes no sense for Cairo and other software renderers
    RENDER_TARGET target = aCached ? TARGET_CACHED : TARGET_NONCACHED;

    for( int i = 0; i < VIEW::VIEW_MAX_LAYERS; i++ )
        SetLayerTarget( i, target );

    SetLayerTarget( LAYER_SCHEMATIC_ANCHOR, TARGET_NONCACHED );
    SetLayerDisplayOnly( LAYER_SCHEMATIC_ANCHOR );

    // Bitmaps are draw on a non cached GAL layer:
    SetLayerTarget( LAYER_DRAW_BITMAPS, TARGET_NONCACHED );

    // Some draw layers need specific settings
    SetLayerTarget( LAYER_GP_OVERLAY, TARGET_OVERLAY );
    SetLayerDisplayOnly( LAYER_GP_OVERLAY ) ;

    SetLayerTarget( LAYER_SELECT_OVERLAY, TARGET_OVERLAY );
    SetLayerDisplayOnly( LAYER_SELECT_OVERLAY ) ;

    SetLayerTarget( LAYER_DRAWINGSHEET, TARGET_NONCACHED );
    SetLayerDisplayOnly( LAYER_DRAWINGSHEET ) ;

    SetLayerTarget( LAYER_OP_VOLTAGES, TARGET_OVERLAY );
    SetLayerDisplayOnly( LAYER_OP_VOLTAGES );
    SetLayerTarget( LAYER_OP_CURRENTS, TARGET_OVERLAY );
    SetLayerDisplayOnly( LAYER_OP_CURRENTS );

    SetLayerTarget( LAYER_SELECTION_SHADOWS, TARGET_OVERLAY );
    SetLayerDisplayOnly( LAYER_SELECTION_SHADOWS ) ;

    SetLayerDisplayOnly( LAYER_NET_COLOR_HIGHLIGHT );
    SetLayerDisplayOnly( LAYER_DANGLING );
}


}; // namespace KIGFX
