/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include "pcb_draw_panel_gal.h"

#include <settings/color_settings.h>
#include <pcb_view.h>
#include <pcb_painter.h>
#include <drawing_sheet/ds_proxy_view_item.h>
#include <connectivity/connectivity_data.h>

#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <macros.h>
#include <pcb_generator.h>
#include <pcb_marker.h>
#include <pcb_point.h>
#include <pcb_base_frame.h>
#include <pcbnew_settings.h>
#include <ratsnest/ratsnest_data.h>
#include <ratsnest/ratsnest_view_item.h>
#include <pcb_board_outline.h>

#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <confirm.h>
#include <progress_reporter.h>
#include <string_utils.h>

#include <gal/graphics_abstraction_layer.h>
#include <zoom_defines.h>

#include <functional>
#include <memory>
#include <thread>
#include <fmt/format.h>
#include <core/ki_assert.h>
#include <pcb_grid_item.h>

using namespace std::placeholders;


PCB_DRAW_PANEL_GAL::PCB_DRAW_PANEL_GAL( TOOLKIT_WINDOW_ARG aParentWindow, wxWindowID aWindowId,
                                        const wxPoint& aPosition, const wxSize& aSize,
                                        KIGFX::GAL_DISPLAY_OPTIONS& aOptions, GAL_TYPE aGalType ) :
        EDA_DRAW_PANEL_GAL( aParentWindow, aWindowId, aPosition, aSize, aOptions, aGalType )
{
    m_view = new KIGFX::PCB_VIEW();
    m_view->SetGAL( m_gal );

    FRAME_T frameType = FRAME_FOOTPRINT_PREVIEW;

    if( EDA_BASE_FRAME* frame = dynamic_cast<EDA_BASE_FRAME*>( static_cast<TOOLKIT_WINDOW*>( aParentWindow ) ) )
        frameType = frame->GetFrameType();

    m_painter = std::make_unique<KIGFX::PCB_PAINTER>( m_gal, frameType );
    m_view->SetPainter( m_painter.get() );

    // This fixes the zoom in and zoom out limits:
    m_view->SetScaleLimits( ZOOM_MAX_LIMIT_PCBNEW, ZOOM_MIN_LIMIT_PCBNEW );

    setDefaultLayerOrder();
    setDefaultLayerDeps();

    // View controls is the first in the event handler chain, so the Tool Framework operates
    // on updated viewport data.
    createViewControls();

    // Load display options (such as filled/outline display of items).
    // Can be made only if the parent window is an EDA_DRAW_FRAME (or a derived class)
    // which is not always the case (namely when it is used from a wxDialog like the pad editor)
    if( !IsDialogPreview() )
    {
        KIGFX::PCB_VIEW* view = static_cast<KIGFX::PCB_VIEW*>( m_view );
        PCB_BASE_FRAME*  frame = dynamic_cast<PCB_BASE_FRAME*>( GetParentEDAFrame() );

        if( frame )
            view->UpdateDisplayOptions( frame->GetDisplayOptions() );
    }
}



PCB_DRAW_PANEL_GAL::~PCB_DRAW_PANEL_GAL()
{
}



void PCB_DRAW_PANEL_GAL::DisplayBoard( BOARD* aBoard, PROGRESS_REPORTER* aReporter )
{
    if( m_drawingSheet )
        m_drawingSheet->SetFileName( TO_UTF8( aBoard->GetFileName() ) );

    GetView()->ShowBoard( aBoard, aReporter );

    // Ratsnest
    if( !aBoard->IsFootprintHolder() )
        addRatsnest( aBoard->GetConnectivity() );
}



void PCB_DRAW_PANEL_GAL::addRatsnest( std::shared_ptr<CONNECTIVITY_DATA> aConnectivity )
{
    // A retained scene caches the board's lines, so the lines of moving items, which change every
    // frame, get their own overlay item
    if( m_gal->HasRetainedScene() )
    {
        m_ratsnest = std::make_unique<RATSNEST_VIEW_ITEM>( aConnectivity, RATSNEST_VIEW_ITEM::PART::BOARD );
        m_localRatsnest = std::make_unique<RATSNEST_VIEW_ITEM>( aConnectivity, RATSNEST_VIEW_ITEM::PART::LOCAL );
        m_view->Add( m_localRatsnest.get() );
    }
    else
    {
        m_ratsnest = std::make_unique<RATSNEST_VIEW_ITEM>( aConnectivity );
        m_localRatsnest.reset();
    }

    m_view->Add( m_ratsnest.get() );
}



void PCB_DRAW_PANEL_GAL::beforeRedraw()
{
    // Nothing else updates the cached board lines when a move starts or ends, a highlight changes
    // or a display option changes
    if( m_localRatsnest && m_ratsnest->BoardInputsChanged( m_view ) )
        m_view->Update( m_ratsnest.get() );
}



void PCB_DRAW_PANEL_GAL::SetDrawingSheet( DS_PROXY_VIEW_ITEM* aDrawingSheet )
{
    m_drawingSheet.reset( aDrawingSheet );
    m_view->Add( m_drawingSheet.get() );
}



void PCB_DRAW_PANEL_GAL::UpdateColors()
{
    COLOR_SETTINGS* cs = ::GetColorSettings( DEFAULT_THEME );
    PCB_BASE_FRAME* frame = dynamic_cast<PCB_BASE_FRAME*>( GetParentEDAFrame() );

    if( frame )
        cs = frame->GetColorSettings();
    else if( PCBNEW_SETTINGS* cfg = GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" ) )
        cs = ::GetColorSettings( cfg->m_ColorTheme );

    KI_CHECK_RET( cs, wxT( "null COLOR_SETTINGS" ) );

    GetView()->LoadColors( cs );
}



void PCB_DRAW_PANEL_GAL::prepareGridSources()
{
    std::vector<KIGFX::GRID_SOURCE> sources;

    BOARD* board = nullptr;

    if( PCB_BASE_FRAME* frame = dynamic_cast<PCB_BASE_FRAME*>( GetParentEDAFrame() ) )
        board = frame->GetBoard();

    // The "Grid Items" object visibility toggle hides the rendered grid
    // content; the items themselves (selection decorations, snap) follow
    // the same flag at their own sites.
    if( !board || !board->IsElementVisible( LAYER_SUBGRIDS ) )
    {
        m_gal->SetGridSources( std::move( sources ) );
        return;
    }

    // Grid content color comes from the "Grid Items" entry of the color
    // theme, falling back to the global grid color.
    COLOR4D gridItemColor = m_gal->GetGridColor();

    if( PCB_BASE_FRAME* frame = dynamic_cast<PCB_BASE_FRAME*>( GetParentEDAFrame() ) )
    {
        if( COLOR_SETTINGS* cs = frame->GetColorSettings() )
            gridItemColor = cs->GetColor( LAYER_SUBGRIDS );
    }

    for( BOARD_ITEM* item : board->Drawings() )
    {
        if( item->Type() != PCB_GRID_ITEM_T )
            continue;

        PCB_GRID_ITEM* grid = static_cast<PCB_GRID_ITEM*>( item );

        // Priority 0 is the background grid's; a grid item must never sink behind it.
        KI_ASSERT( grid->GetAssignedPriority() > 0 );

        KIGFX::GRID_SOURCE src;
        static_cast<GRID_GEOMETRY&>( src ) = grid->AsGridGeometry();

        // Pitch is a loop step in the renderer; the spacing setters keep it positive.
        KI_ASSERT( src.pitch.x > 0.0 && src.pitch.y > 0.0 );

        src.tick = grid->GetTickInterval();
        src.highlighted = grid->IsSelected();
        src.snapCursor = grid->Affects().cursor && !grid->IsSelected();
        src.color = gridItemColor;
        src.style = m_gal->GetGridStyle(); // inherit display style

        sources.push_back( src );
    }

    m_gal->SetGridSources( std::move( sources ) );
}



void PCB_DRAW_PANEL_GAL::SetHighContrastLayer( PCB_LAYER_ID aLayer )
{
    applyHighContrastLayer( aLayer, false );
}



void PCB_DRAW_PANEL_GAL::ChangeHighContrastLayer( PCB_LAYER_ID aLayer )
{
    applyHighContrastLayer( aLayer, true );
}



void PCB_DRAW_PANEL_GAL::applyHighContrastLayer( PCB_LAYER_ID aLayer, bool aOnlyLayerChanged )
{
    // The high contrast layers, brought on top
    GetView()->SetHighContrastLayer( aLayer );

    KIGFX::RENDER_SETTINGS* rSettings = m_view->GetPainter()->GetSettings();

    // High contrast hides and shows items as well as recolouring them
    const bool contrast = rSettings->GetHighContrast();
    auto*      pcbSettings = static_cast<KIGFX::PCB_RENDER_SETTINGS*>( rSettings );
    const bool hidden = contrast && pcbSettings->m_ContrastModeDisplay == HIGH_CONTRAST_MODE::HIDDEN;

    // Layers the canvas fades keep their item colours, unless high contrast hides inactive layers
    // now or did at the last switch
    if( aOnlyLayerChanged && pcbSettings->HasCanvasFadedLayers() && !hidden && !m_highContrastHidden )
    {
        m_view->UpdateLayersColor( contrast || m_highContrastShown,
                                   [pcbSettings]( int aViewLayer )
                                   {
                                       return !pcbSettings->IsCanvasFadedLayer( aViewLayer );
                                   } );
    }
    else if( contrast || m_highContrastShown )
    {
        m_view->UpdateAllLayersColorAndVisibility();
    }
    else
    {
        m_view->UpdateAllLayersColor();
    }

    m_highContrastShown = contrast;
    m_highContrastHidden = hidden;
}



void PCB_DRAW_PANEL_GAL::SetTopLayer( PCB_LAYER_ID aLayer )
{
    GetView()->SetActiveLayerOnTop( aLayer );
}



void PCB_DRAW_PANEL_GAL::SyncLayersVisibility( const BOARD* aBoard )
{
    PCB_LAYER_ID    activeLayer = UNDEFINED_LAYER;
    PCB_BASE_FRAME* frame = dynamic_cast<PCB_BASE_FRAME*>( GetParentEDAFrame() );

    // A preview canvas edits no layer, so it shows no clearance outlines
    if( frame && frame->GetCanvas() == this )
        activeLayer = frame->GetActiveLayer();

    GetView()->SyncLayersVisibility( aBoard, activeLayer );
}



void PCB_DRAW_PANEL_GAL::SyncLayersVisibility( KIGFX::VIEW& aView, const BOARD* aBoard, PCB_LAYER_ID aActiveLayer )
{
    SyncPcbLayersVisibility( aView, aBoard, aActiveLayer );
}



void PCB_DRAW_PANEL_GAL::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame,
                                          std::vector<MSG_PANEL_ITEM>& aList )
{
    BOARD*        board = static_cast<PCB_BASE_FRAME*>( GetParentEDAFrame() )->GetBoard();
    int           padCount = 0;
    int           viaCount = 0;
    int           trackSegmentCount = 0;
    std::set<int> netCodes;
    int           unconnected = (int) board->GetConnectivity()->GetUnconnectedCount( true );

    for( PCB_TRACK* item : board->Tracks() )
    {
        if( item->Type() == PCB_VIA_T )
            viaCount++;
        else
            trackSegmentCount++;

        if( item->GetNetCode() > 0 )
            netCodes.insert( item->GetNetCode() );
    }

    for( FOOTPRINT* footprint : board->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            padCount++;

            if( pad->GetNetCode() > 0 )
                netCodes.insert( pad->GetNetCode() );
        }
    }

    aList.emplace_back( _( "Pads" ), fmt::format( "{}", padCount ) );
    aList.emplace_back( _( "Vias" ), fmt::format( "{}", viaCount ) );
    aList.emplace_back( _( "Track Segments" ), fmt::format( "{}", trackSegmentCount ) );
    aList.emplace_back( _( "Nets" ), fmt::format( "{}", (int) netCodes.size() ) );
    aList.emplace_back( _( "Unrouted" ), fmt::format( "{}", unconnected ) );
}



void PCB_DRAW_PANEL_GAL::OnShow()
{
    PCB_BASE_FRAME* frame = nullptr;

    if( !IsDialogPreview() )
        frame = dynamic_cast<PCB_BASE_FRAME*>( GetParentEDAFrame() );

    EDA_DRAW_PANEL_GAL::OnShow();

    if( frame )
    {
        SetTopLayer( frame->GetActiveLayer() );

        KIGFX::PCB_PAINTER* painter = static_cast<KIGFX::PCB_PAINTER*>( m_view->GetPainter() );
        KIGFX::PCB_RENDER_SETTINGS* settings = painter->GetSettings();

        settings->LoadDisplayOptions( frame->GetDisplayOptions() );
        settings->m_ForceShowFieldsWhenFPSelected = frame->GetPcbNewSettings()->m_Display.m_ForceShowFieldsWhenFPSelected;
    }
}



void PCB_DRAW_PANEL_GAL::setDefaultLayerOrder()
{
    GetView()->SetDefaultLayerOrder();
}



bool PCB_DRAW_PANEL_GAL::SwitchBackend( GAL_TYPE aGalType )
{
    bool rv = EDA_DRAW_PANEL_GAL::SwitchBackend( aGalType );
    setDefaultLayerDeps();
    m_gal->SetWorldUnitLength( 1e-9 /* 1 nm */ / 0.0254 /* 1 inch in meters */ );

    // Whether the ratsnest is split depends on the canvas
    if( m_ratsnest && ( m_localRatsnest != nullptr ) != m_gal->HasRetainedScene() )
    {
        std::shared_ptr<CONNECTIVITY_DATA> connectivity = m_ratsnest->GetConnectivity();

        m_view->Remove( m_ratsnest.get() );

        if( m_localRatsnest )
            m_view->Remove( m_localRatsnest.get() );

        addRatsnest( connectivity );
    }

    return rv;
}



void PCB_DRAW_PANEL_GAL::RedrawRatsnest()
{
    if( m_ratsnest )
        m_view->Update( m_ratsnest.get() );

    if( m_localRatsnest )
        m_view->Update( m_localRatsnest.get() );
}



BOX2I PCB_DRAW_PANEL_GAL::GetDefaultViewBBox() const
{
    if( m_drawingSheet && m_view->IsLayerVisible( LAYER_DRAWINGSHEET ) )
        return m_drawingSheet->ViewBBox();

    return BOX2I();
}



void PCB_DRAW_PANEL_GAL::setDefaultLayerDeps()
{
    const bool retainedScene = m_gal && m_gal->HasRetainedScene();

    // Cairo keeps cached groups in screen coordinates, so caching makes no sense for it
    GetView()->SetDefaultLayerDeps( IsAccelerated( m_backend ), retainedScene );

    // A retained scene caches the board's ratsnest, blended as on the legacy overlay
    if( retainedScene )
        m_gal->SetSceneLayerOverlayAlpha( LAYER_RATSNEST );

    updateCanvasFadedLayers();
}



void PCB_DRAW_PANEL_GAL::updateCanvasFadedLayers()
{
    auto* settings = static_cast<KIGFX::PCB_RENDER_SETTINGS*>( m_view->GetPainter()->GetSettings() );
    std::bitset<MAX_LAYERS_FOR_VIEW> layers;

    if( m_gal && m_gal->HasRetainedScene() )
    {
        for( int layer = 0; layer < PCB_LAYER_ID_COUNT; ++layer )
        {
            for( int viewLayer : { layer, static_cast<int>( ZONE_LAYER_FOR( layer ) ),
                                   static_cast<int>( PAD_COPPER_LAYER_FOR( layer ) ),
                                   static_cast<int>( VIA_COPPER_LAYER_FOR( layer ) ),
                                   static_cast<int>( CLEARANCE_LAYER_FOR( layer ) ) } )
            {
                if( m_view->IsCached( viewLayer ) )
                    layers.set( viewLayer );
            }
        }
    }

    settings->SetCanvasFadedLayers( layers );
}



KIGFX::PCB_VIEW* PCB_DRAW_PANEL_GAL::GetView() const
{
    return static_cast<KIGFX::PCB_VIEW*>( m_view );
}
