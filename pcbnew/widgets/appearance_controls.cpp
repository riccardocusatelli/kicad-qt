/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
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
 * @file appearance_controls.cpp
 * The appearance panel of the board editors: the settings of its rows, the layer presets and
 * viewports, and what the controls do (without the GUI toolkit).  The widgets are in
 * appearance_controls_wx.cpp (and in the Qt build).
 */

#include <widgets/appearance_controls.h>

#include <board.h>
#include <board_design_settings.h>
#include <class_draw_panel_gal.h>
#include <footprint_editor_settings.h>
#include <pad.h>
#include <pcb_base_frame.h>
#include <pcb_display_options.h>
#include <pcb_edit_frame.h>
#include <pcb_painter.h>
#include <pcb_track.h>
#include <pcbnew_settings.h>
#include <project.h>
#include <project/net_settings.h>
#include <project/project_local_settings.h>
#include <settings/color_settings.h>
#include <settings/settings_manager.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <view/view.h>
#include <widgets/bitmap_toggle.h>
#include <widgets/color_swatch.h>
#include <core/ki_assert.h>

#include <set>


/// Template for object appearance settings
const std::vector<APPEARANCE_CONTROLS::APPEARANCE_SETTING> APPEARANCE_CONTROLS::s_objectSettings = {

#define RR  APPEARANCE_CONTROLS::APPEARANCE_SETTING   // Render Row abbreviation to reduce source width

    // clang-format off

    //        text                           id                            tooltip                opacity slider   visibility checkbox
    RR( _HKI( "Tracks" ),               LAYER_TRACKS,             _HKI( "Show tracks" ),              true ),
    RR( _HKI( "Vias" ),                 LAYER_VIAS,               _HKI( "Show all vias" ),            true ),
    RR( _HKI( "Pads" ),                 LAYER_PADS,               _HKI( "Show all pads" ),            true ),
    RR( _HKI( "Zones" ),                LAYER_ZONES,              _HKI( "Show copper zones" ),        true ),
    RR( _HKI( "Filled Shapes" ),        LAYER_FILLED_SHAPES,      _HKI( "Opacity of filled shapes" ), true,             false ),
    RR( _HKI( "Images" ),               LAYER_DRAW_BITMAPS,       _HKI( "Show user images" ),         true ),
    RR(),
    RR( _HKI( "Footprints Front" ),     LAYER_FOOTPRINTS_FR,      _HKI( "Show footprints that are on board's front" ) ),
    RR( _HKI( "Footprints Back" ),      LAYER_FOOTPRINTS_BK,      _HKI( "Show footprints that are on board's back" ) ),
    RR( _HKI( "Values" ),               LAYER_FP_VALUES,          _HKI( "Show footprint values" ) ),
    RR( _HKI( "References" ),           LAYER_FP_REFERENCES,      _HKI( "Show footprint references" ) ),
    RR( _HKI( "Footprint Text" ),       LAYER_FP_TEXT,            _HKI( "Show all footprint text" ) ),
    RR(),
    RR(),
    RR( _HKI( "Ratsnest" ),             LAYER_RATSNEST,           _HKI( "Show unconnected nets as a ratsnest") ),
    RR( _HKI( "DRC Warnings" ),         LAYER_DRC_WARNING,        _HKI( "DRC violations with a Warning severity" ) ),
    RR( _HKI( "DRC Errors" ),           LAYER_DRC_ERROR,          _HKI( "DRC violations with an Error severity" ) ),
    RR( _HKI( "DRC Exclusions" ),       LAYER_DRC_EXCLUSION,      _HKI( "DRC violations which have been individually excluded" ) ),
    RR( _HKI( "Anchors" ),              LAYER_ANCHOR,             _HKI( "Show footprint and text origins as a cross" ) ),
    RR( _HKI( "Points" ),               LAYER_POINTS,             _HKI( "Show explicit snap points as crosses" ) ),
    RR( _HKI( "Locked Item Shadow" ),   LAYER_LOCKED_ITEM_SHADOW, _HKI( "Show a shadow on locked items" ) ),
    RR( _HKI( "Colliding Courtyards" ), LAYER_CONFLICTS_SHADOW,   _HKI( "Show colliding footprint courtyards" ) ),
    RR( _HKI( "Board Area Shadow" ),    LAYER_BOARD_OUTLINE_AREA, _HKI( "Show board area shadow" ) ),
    RR( _HKI( "Drawing Sheet" ),        LAYER_DRAWINGSHEET,       _HKI( "Show drawing sheet borders and title block" ) ),
    RR( _HKI( "Grid" ),                 LAYER_GRID,               _HKI( "Show the (x,y) grid dots" ) )
    // clang-format on
};

/// These GAL layers are shown in the Objects tab in the footprint editor
bool APPEARANCE_CONTROLS::isAllowedInFpEditor( int aLayer )
{
    static const std::set<int> allowed =
        {
            LAYER_TRACKS,
            LAYER_VIAS,
            LAYER_PADS,
            LAYER_ZONES,
            LAYER_FILLED_SHAPES,
            LAYER_FP_VALUES,
            LAYER_FP_REFERENCES,
            LAYER_FP_TEXT,
            LAYER_DRAW_BITMAPS,
            LAYER_GRID,
            LAYER_POINTS,
        };

    return allowed.count( aLayer ) > 0;
}
// These are the built-in layer presets that cannot be deleted

LAYER_PRESET APPEARANCE_CONTROLS::presetNoLayers( _HKI( "No Layers" ), LSET(), false );

LAYER_PRESET APPEARANCE_CONTROLS::presetAllLayers( _HKI( "All Layers" ),
        LSET::AllLayersMask(), false );

LAYER_PRESET APPEARANCE_CONTROLS::presetAllCopper( _HKI( "All Copper Layers" ),
        LSET( LSET::AllCuMask() ).set( Edge_Cuts ), false );

LAYER_PRESET APPEARANCE_CONTROLS::presetInnerCopper( _HKI( "Inner Copper Layers" ),
        LSET( LSET::InternalCuMask() ).set( Edge_Cuts ), false );

LAYER_PRESET APPEARANCE_CONTROLS::presetFront( _HKI( "Front Layers" ),
        LSET( LSET::FrontMask() ).set( Edge_Cuts ), false );

LAYER_PRESET APPEARANCE_CONTROLS::presetFrontAssembly( _HKI( "Front Assembly View" ),
        LSET( LSET::FrontAssembly() ).set( Edge_Cuts ), GAL_SET::DefaultVisible(), F_SilkS, false );

LAYER_PRESET APPEARANCE_CONTROLS::presetBack( _HKI( "Back Layers" ),
        LSET( LSET::BackMask() ).set( Edge_Cuts ), true );

LAYER_PRESET APPEARANCE_CONTROLS::presetBackAssembly( _HKI( "Back Assembly View" ),
        LSET( LSET::BackAssembly() ).set( Edge_Cuts ), GAL_SET::DefaultVisible(), B_SilkS, true );

// this one is only used to store the object visibility settings  of the last used
// built-in layer preset
LAYER_PRESET APPEARANCE_CONTROLS::m_lastBuiltinPreset;


void APPEARANCE_CONTROLS::CommonSettingsChanged( int aFlags )
{
    if( aFlags & HOTKEYS_CHANGED )
        rebuildLayers();
}

void APPEARANCE_CONTROLS::OnBoardNetSettingsChanged( BOARD& aBoard )
{
    handleBoardItemsChanged();
}


bool APPEARANCE_CONTROLS::doesBoardItemNeedRebuild( BOARD_ITEM* aBoardItem )
{
    return aBoardItem->Type() == PCB_NETINFO_T;
}


bool APPEARANCE_CONTROLS::doesBoardItemNeedRebuild( std::vector<BOARD_ITEM*>& aBoardItems )
{
    bool rebuild = std::any_of( aBoardItems.begin(), aBoardItems.end(),
                                []( const BOARD_ITEM* a )
                                {
                                    return a->Type() == PCB_NETINFO_T;
                                } );

    return rebuild;
}


void APPEARANCE_CONTROLS::OnBoardItemAdded( BOARD& aBoard, BOARD_ITEM* aItem )
{
    if( doesBoardItemNeedRebuild( aItem ) )
        handleBoardItemsChanged();
}


void APPEARANCE_CONTROLS::OnBoardItemsAdded( BOARD& aBoard, std::vector<BOARD_ITEM*>& aItems )
{
    if( doesBoardItemNeedRebuild( aItems ) )
        handleBoardItemsChanged();
}


void APPEARANCE_CONTROLS::OnBoardItemRemoved( BOARD& aBoard, BOARD_ITEM* aItem )
{
    if( doesBoardItemNeedRebuild( aItem ) )
        handleBoardItemsChanged();
}


void APPEARANCE_CONTROLS::OnBoardItemsRemoved( BOARD& aBoard, std::vector<BOARD_ITEM*>& aItems )
{
    if( doesBoardItemNeedRebuild( aItems ) )
        handleBoardItemsChanged();
}


void APPEARANCE_CONTROLS::OnBoardItemChanged( BOARD& aBoard, BOARD_ITEM* aItem )
{
    if( doesBoardItemNeedRebuild( aItem ) )
        handleBoardItemsChanged();
}


void APPEARANCE_CONTROLS::OnBoardItemsChanged( BOARD& aBoard, std::vector<BOARD_ITEM*>& aItems )
{
    if( doesBoardItemNeedRebuild( aItems ) )
        handleBoardItemsChanged();
}


void APPEARANCE_CONTROLS::OnBoardCompositeUpdate( BOARD&                    aBoard,
                                                  std::vector<BOARD_ITEM*>& aAddedItems,
                                                  std::vector<BOARD_ITEM*>& aRemovedItems,
                                                  std::vector<BOARD_ITEM*>& aChangedItems )
{
    if( doesBoardItemNeedRebuild( aAddedItems ) || doesBoardItemNeedRebuild( aRemovedItems )
        || doesBoardItemNeedRebuild( aChangedItems ) )
    {
        handleBoardItemsChanged();
    }
}


void APPEARANCE_CONTROLS::OnColorThemeChanged()
{
    if( !m_frame->GetBoard() )
        return;

    syncColorsAndVisibility();
    syncObjectSettings();
}


void APPEARANCE_CONTROLS::SetLayerVisible( int aLayer, bool isVisible )
{
    LSET         visible = getVisibleLayers();
    PCB_LAYER_ID layer   = ToLAYER_ID( aLayer );

    if( visible.test( layer ) == isVisible )
        return;

    visible.set( layer, isVisible );
    setVisibleLayers( visible );

    m_frame->GetCanvas()->GetView()->SetLayerVisible( layer, isVisible );

    syncColorsAndVisibility();
}


void APPEARANCE_CONTROLS::SetObjectVisible( GAL_LAYER_ID aLayer, bool isVisible )
{
    if( m_objectSettingsMap.count( aLayer ) )
    {
        APPEARANCE_SETTING* setting = m_objectSettingsMap.at( aLayer );

        if( setting->can_control_visibility )
            setting->ctl_visibility->SetValue( isVisible );
    }

    BOARD* board = m_frame->GetBoard();

    if( !board )
        return;

    board->SetElementVisibility( aLayer, isVisible );

    m_frame->Update3DView( true, m_frame->GetPcbNewSettings()->m_Display.m_Live3DRefresh );

    m_frame->GetCanvas()->GetView()->SetLayerVisible( aLayer, isVisible );
    m_frame->GetCanvas()->Refresh();
}


void APPEARANCE_CONTROLS::setVisibleLayers( const LSET& aLayers )
{
    KIGFX::VIEW* view = m_frame->GetCanvas()->GetView();

    if( m_isFpEditor )
    {
        for( PCB_LAYER_ID layer : LSET::AllLayersMask().Seq() )
            view->SetLayerVisible( layer, aLayers.Contains( layer ) );
    }
    else if( BOARD* board = m_frame->GetBoard() )
    {
        board->SetVisibleLayers( aLayers );

        // Note: KIGFX::REPAINT isn't enough for things that go from invisible to visible as
        // they won't be found in the view layer's itemset for repainting.
        view->UpdateAllItemsConditionally( KIGFX::ALL,
                []( KIGFX::VIEW_ITEM* aItem ) -> bool
                {
                    // Items rendered to composite layers (such as LAYER_PAD_TH) must be redrawn
                    // whether they're optionally flashed or not (as the layer being hidden/shown
                    // might be the last layer the item is visible on).
                    return dynamic_cast<PCB_VIA*>( aItem ) || dynamic_cast<PAD*>( aItem );
                } );

        m_frame->Update3DView( true, m_frame->GetPcbNewSettings()->m_Display.m_Live3DRefresh );
    }
}


bool APPEARANCE_CONTROLS::isLayerEnabled( PCB_LAYER_ID aLayer ) const
{
    // This used to be used for disabling some layers in the footprint editor, but
    // now all layers are enabled in the footprint editor.
    // But this function is the place to add logic if you do need to grey out a layer
    // from the appearance panel for some reason.
    return true;
}


void APPEARANCE_CONTROLS::setVisibleObjects( GAL_SET aLayers )
{
    KIGFX::VIEW* view = m_frame->GetCanvas()->GetView();

    if( m_isFpEditor )
    {
        for( size_t i = 0; i < GAL_LAYER_INDEX( LAYER_ZONE_START ); i++ )
            view->SetLayerVisible( GAL_LAYER_ID_START + GAL_LAYER_ID( i ), aLayers.test( i ) );
    }
    else
    {
        // Ratsnest visibility is controlled by the ratsnest option, and not by the preset
        if( m_frame->IsType( FRAME_PCB_EDITOR ) )
            aLayers.set( LAYER_RATSNEST, m_frame->GetPcbNewSettings()->m_Display.m_ShowGlobalRatsnest );

        BOARD* board = m_frame->GetBoard();

        if( !board )
            return;

        m_frame->SetGridVisibility( aLayers.test( LAYER_GRID - GAL_LAYER_ID_START ) );
        board->SetVisibleElements( aLayers );

        // Update VIEW layer visibility to stay in sync with board settings
        for( size_t i = 0; i < GAL_LAYER_INDEX( LAYER_ZONE_START ) && i < aLayers.size(); i++ )
        {
            // Warning: all GAL layers are not handled by the apparence panel (i.e. LAYER_SELECT_OVERLAY)
            // but only some, only set visiblity if the layer is handled by the APPEARANCE_CONTROLS
            GAL_LAYER_ID gal_ly = GAL_LAYER_ID( i ) + GAL_LAYER_ID_START;

            if( gal_ly == LAYER_RATSNEST )
                continue;

            for( const APPEARANCE_SETTING& s_setting : s_objectSettings )
            {
                // See if this gal layer is handled
                if( s_setting.id == gal_ly )
                {
                    view->SetLayerVisible( gal_ly, aLayers.test( i ) );
                    break;
                }
            }
        }

        m_frame->Update3DView( true, m_frame->GetPcbNewSettings()->m_Display.m_Live3DRefresh );
    }
}


LSET APPEARANCE_CONTROLS::getVisibleLayers()
{
    if( m_isFpEditor )
    {
        KIGFX::VIEW* view = m_frame->GetCanvas()->GetView();
        LSET set;

        for( PCB_LAYER_ID layer : LSET::AllLayersMask().Seq() )
            set.set( layer, view->IsLayerVisible( layer ) );

        return set;
    }
    else if( BOARD* board = m_frame->GetBoard() )
    {
        return board->GetVisibleLayers();
    }

    return LSET();
}


GAL_SET APPEARANCE_CONTROLS::getVisibleObjects()
{
    if( m_isFpEditor )
    {
        KIGFX::VIEW* view = m_frame->GetCanvas()->GetView();
        GAL_SET set;
        set.reset();

        for( size_t i = 0; i < set.size(); i++ )
            set.set( i, view->IsLayerVisible( GAL_LAYER_ID_START + GAL_LAYER_ID( i ) ) );

        return set;
    }
    else if( BOARD* board = m_frame->GetBoard() )
    {
        return board->GetVisibleElements();
    }

    return GAL_SET();
}


std::vector<LAYER_PRESET> APPEARANCE_CONTROLS::GetUserLayerPresets() const
{
    std::vector<LAYER_PRESET> ret;

    for( const std::pair<const wxString, LAYER_PRESET>& pair : m_layerPresets )
    {
        if( !pair.second.readOnly )
            ret.emplace_back( pair.second );
    }

    return ret;
}


void APPEARANCE_CONTROLS::SetUserLayerPresets( std::vector<LAYER_PRESET>& aPresetList )
{
    // Reset to defaults
    loadDefaultLayerPresets();

    for( const LAYER_PRESET& preset : aPresetList )
    {
        if( m_layerPresets.count( preset.name ) )
            continue;

        m_layerPresets[preset.name] = preset;

        m_presetMRU.Add( preset.name );
    }

    rebuildLayerPresetsWidget( true );
}


void APPEARANCE_CONTROLS::loadDefaultLayerPresets()
{
    m_layerPresets.clear();

    // Load the read-only defaults
    for( const LAYER_PRESET& preset : { presetAllLayers,
                                        presetNoLayers,
                                        presetAllCopper,
                                        presetInnerCopper,
                                        presetFront,
                                        presetFrontAssembly,
                                        presetBack,
                                        presetBackAssembly } )
    {
        m_layerPresets[preset.name]          = preset;
        m_layerPresets[preset.name].readOnly = true;
    }
}


void APPEARANCE_CONTROLS::ApplyLayerPreset( const LAYER_PRESET& aPreset )
{
    if( m_layerPresets.count( aPreset.name ) )
        m_currentPreset = &m_layerPresets[aPreset.name];
    else
        m_currentPreset = nullptr;

    m_lastSelectedUserPreset = ( m_currentPreset && !m_currentPreset->readOnly ) ? m_currentPreset
                                                                                 : nullptr;

    updateLayerPresetSelection( aPreset.name );
    doApplyLayerPreset( aPreset );
}


std::vector<VIEWPORT> APPEARANCE_CONTROLS::GetUserViewports() const
{
    std::vector<VIEWPORT> ret;

    for( const std::pair<const wxString, VIEWPORT>& pair : m_viewports )
        ret.emplace_back( pair.second );

    return ret;
}


void APPEARANCE_CONTROLS::SetUserViewports( std::vector<VIEWPORT>& aViewportList )
{
    m_viewports.clear();

    for( const VIEWPORT& viewport : aViewportList )
    {
        if( m_viewports.count( viewport.name ) )
            continue;

        m_viewports[viewport.name] = viewport;

        m_viewportMRU.Add( viewport.name );
    }

    rebuildViewportsWidget();
}


void APPEARANCE_CONTROLS::ApplyViewport( const VIEWPORT& aViewport )
{
    updateViewportSelection( aViewport.name );
    doApplyViewport( aViewport );
}


void APPEARANCE_CONTROLS::onLayerVisibilityToggled( PCB_LAYER_ID aLayer )
{
    LSET visibleLayers = getVisibleLayers();

    visibleLayers.set( aLayer, !visibleLayers.test( aLayer ) );
    setVisibleLayers( visibleLayers );
    m_frame->GetCanvas()->GetView()->SetLayerVisible( aLayer, visibleLayers.test( aLayer ) );

    syncLayerPresetSelection();
    m_frame->GetCanvas()->Refresh();
}


void APPEARANCE_CONTROLS::onObjectVisibilityChanged( GAL_LAYER_ID aLayer, bool isVisible, bool isFinal )
{
    // Special-case controls
    switch( aLayer )
    {
    case LAYER_RATSNEST:
    {
        // don't touch the layers. ratsnest is enabled on per-item basis.
        m_frame->GetCanvas()->GetView()->MarkTargetDirty( KIGFX::TARGET_NONCACHED );
        m_frame->GetCanvas()->GetView()->SetLayerVisible( aLayer, true );

        if( m_frame->IsType( FRAME_PCB_EDITOR ) )
        {
            m_frame->GetPcbNewSettings()->m_Display.m_ShowGlobalRatsnest = isVisible;

            if( m_frame->GetBoard() )
                m_frame->GetBoard()->SetElementVisibility( aLayer, isVisible );

            m_frame->OnDisplayOptionsChanged();
            m_frame->GetCanvas()->RedrawRatsnest();
        }

        break;
    }

    case LAYER_GRID:
        m_frame->SetGridVisibility( isVisible );
        m_frame->GetCanvas()->Refresh();
        syncLayerPresetSelection();
        break;

    case LAYER_FP_TEXT:
        // Because Footprint Text is a meta-control that also can disable values/references,
        // drag them along here so that the user is less likely to be confused.
        if( isFinal )
        {
            // Should only trigger when you actually click the Footprint Text button
            // Otherwise it goes into infinite recursive loop with the following case section
            onObjectVisibilityChanged( LAYER_FP_REFERENCES, isVisible, false );
            onObjectVisibilityChanged( LAYER_FP_VALUES, isVisible, false );
            m_objectSettingsMap[LAYER_FP_REFERENCES]->ctl_visibility->SetValue( isVisible );
            m_objectSettingsMap[LAYER_FP_VALUES]->ctl_visibility->SetValue( isVisible );
        }
        break;

    case LAYER_FP_REFERENCES:
    case LAYER_FP_VALUES:
        // In case that user changes Footprint Value/References when the Footprint Text
        // meta-control is disabled, we should put it back on.
        if( isVisible )
        {
            onObjectVisibilityChanged( LAYER_FP_TEXT, isVisible, false );
            m_objectSettingsMap[LAYER_FP_TEXT]->ctl_visibility->SetValue( isVisible );
        }
        break;

    default:
        break;
    }

    GAL_SET visible = getVisibleObjects();

    if( visible.Contains( aLayer ) != isVisible )
    {
        visible.set( aLayer, isVisible );
        setVisibleObjects( visible );
        m_frame->GetCanvas()->GetView()->SetLayerVisible( aLayer, isVisible );
        syncLayerPresetSelection();
    }

    if( isFinal )
    {
        m_frame->GetCanvas()->Refresh();
        passOnFocus();
    }
}


void APPEARANCE_CONTROLS::doApplyLayerPreset( const LAYER_PRESET& aPreset )
{
    BOARD* board = m_frame->GetBoard();

    if( !board )
        return;

    setVisibleLayers( aPreset.layers );
    setVisibleObjects( aPreset.renderLayers );

    // If the preset doesn't have an explicit active layer to restore, we can at least
    // force the active layer to be something in the preset's layer set
    PCB_LAYER_ID activeLayer = UNSELECTED_LAYER;

    if( aPreset.activeLayer != UNSELECTED_LAYER )
        activeLayer = aPreset.activeLayer;
    else if( aPreset.layers.any() && !aPreset.layers.test( m_frame->GetActiveLayer() ) )
        activeLayer = *aPreset.layers.Seq().begin();

    LSET boardLayers = board->GetLayerSet();

    if( activeLayer != UNSELECTED_LAYER && boardLayers.Contains( activeLayer ) )
        m_frame->SetActiveLayer( activeLayer );

    if( !m_isFpEditor )
        m_frame->GetCanvas()->SyncLayersVisibility( board );

    PCB_DISPLAY_OPTIONS options = m_frame->GetDisplayOptions();
    options.m_FlipBoardView = aPreset.flipBoard;
    m_frame->SetDisplayOptions( options, false );

    m_frame->GetCanvas()->Refresh();

    syncColorsAndVisibility();
    UpdateDisplayOptions();
}


void APPEARANCE_CONTROLS::doApplyViewport( const VIEWPORT& aViewport )
{
    m_frame->GetCanvas()->GetView()->SetViewport( aViewport.rect );
    m_frame->GetCanvas()->Refresh();
}


void APPEARANCE_CONTROLS::onObjectOpacitySlider( int aLayer, float aOpacity )
{
    PCB_DISPLAY_OPTIONS options = m_frame->GetDisplayOptions();

    switch( aLayer )
    {
    case static_cast<int>( LAYER_TRACKS ):        options.m_TrackOpacity       = aOpacity; break;
    case static_cast<int>( LAYER_VIAS ):          options.m_ViaOpacity         = aOpacity; break;
    case static_cast<int>( LAYER_PADS ):          options.m_PadOpacity         = aOpacity; break;
    case static_cast<int>( LAYER_ZONES ):         options.m_ZoneOpacity        = aOpacity; break;
    case static_cast<int>( LAYER_DRAW_BITMAPS ):  options.m_ImageOpacity       = aOpacity; break;
    case static_cast<int>( LAYER_FILLED_SHAPES ): options.m_FilledShapeOpacity = aOpacity; break;
    default: return;
    }

    m_frame->SetDisplayOptions( options );
    passOnFocus();
}


void APPEARANCE_CONTROLS::showNetclass( const wxString& aClassName, bool aShow )
{
    BOARD* board = m_frame->GetBoard();

    if( !board )
        return;

    m_togglingNetclassRatsnestVisibility = true;

    for( NETINFO_ITEM* net : board->GetNetInfo() )
    {
        if( net->GetNetClass()->ContainsNetclassWithName( aClassName ) )
        {
            m_frame->GetToolManager()->RunAction( aShow ? PCB_ACTIONS::showNetInRatsnest
                                                        : PCB_ACTIONS::hideNetInRatsnest,
                                                  net->GetNetCode() );

            netGridSetVisibility( net->GetNetCode(), aShow );
        }
    }

    PROJECT_LOCAL_SETTINGS& localSettings = m_frame->Prj().GetLocalSettings();

    if( !aShow )
        localSettings.m_HiddenNetclasses.insert( aClassName );
    else
        localSettings.m_HiddenNetclasses.erase( aClassName );

    netGridRefresh();
    m_frame->GetCanvas()->RedrawRatsnest();
    m_frame->GetCanvas()->Refresh();
    m_togglingNetclassRatsnestVisibility = false;
}


bool APPEARANCE_CONTROLS::IsTogglingNetclassRatsnestVisibility()
{
    return m_togglingNetclassRatsnestVisibility;
}


void APPEARANCE_CONTROLS::doLayerContextMenu( int aId )
{
    BOARD* board = m_frame->GetBoard();

    if( !board )
        return;

    LSET visible = getVisibleLayers();

    PCB_LAYER_ID current = m_frame->GetActiveLayer();

    // The new preset. We keep the visibility state of objects:
    LAYER_PRESET preset;
    preset.renderLayers = getVisibleObjects();
    preset.flipBoard = m_frame->GetDisplayOptions().m_FlipBoardView;

    switch( aId )
    {
    case ID_PRESET_NO_LAYERS:
        preset.layers = presetNoLayers.layers;
        ApplyLayerPreset( preset );
        return;

    case ID_PRESET_ALL_LAYERS:
        preset.layers = presetAllLayers.layers;
        ApplyLayerPreset( preset );
        return;

    case ID_SHOW_ALL_COPPER_LAYERS:
        visible |= presetAllCopper.layers;
        setVisibleLayers( visible );
        break;

    case ID_HIDE_ALL_BUT_ACTIVE:
        preset.layers = presetNoLayers.layers | LSET( { current } );
        ApplyLayerPreset( preset );
        break;

    case ID_HIDE_ALL_COPPER_LAYERS:
        visible &= ~presetAllCopper.layers;

        if( !visible.test( current ) && visible.count() > 0 )
            m_frame->SetActiveLayer( *visible.Seq().begin() );

        setVisibleLayers( visible );
        break;

    case ID_HIDE_ALL_NON_COPPER:
        visible &= presetAllCopper.layers;

        if( !visible.test( current ) && visible.count() > 0 )
            m_frame->SetActiveLayer( *visible.Seq().begin() );

        setVisibleLayers( visible );
        break;

    case ID_SHOW_ALL_NON_COPPER:
        visible |= ~presetAllCopper.layers;

        setVisibleLayers( visible );
        break;

    case ID_PRESET_FRONT_ASSEMBLY:
        preset.layers = presetFrontAssembly.layers;
        ApplyLayerPreset( preset );
        return;

    case ID_PRESET_FRONT:
        preset.layers = presetFront.layers;
        ApplyLayerPreset( preset );
        return;

    case ID_PRESET_INNER_COPPER:
        preset.layers = presetInnerCopper.layers;
        ApplyLayerPreset( preset );
        return;

    case ID_PRESET_BACK:
        preset.layers = presetBack.layers;
        ApplyLayerPreset( preset );
        return;

    case ID_PRESET_BACK_ASSEMBLY:
        preset.layers = presetBackAssembly.layers;
        ApplyLayerPreset( preset );
        return;
    }

    syncLayerPresetSelection();
    syncColorsAndVisibility();

    if( !m_isFpEditor )
        m_frame->GetCanvas()->SyncLayersVisibility( board );

    m_frame->GetCanvas()->Refresh();
}


void APPEARANCE_CONTROLS::onLayerColorChanged( int layer, const COLOR4D& newColor )
{
    COLOR_SETTINGS* cs = m_frame->GetColorSettings();

    cs->SetColor( layer, newColor );
    m_frame->GetSettingsManager()->SaveColorSettings( cs, "board" );

    m_frame->GetCanvas()->UpdateColors();

    KIGFX::VIEW* view = m_frame->GetCanvas()->GetView();
    view->UpdateLayerColor( layer );
    view->UpdateLayerColor( GetNetnameLayer( layer ) );

    if( IsCopperLayer( layer ) )
    {
        view->UpdateLayerColor( ZONE_LAYER_FOR( layer ) );
        view->UpdateLayerColor( VIA_COPPER_LAYER_FOR( layer ) );
        view->UpdateLayerColor( PAD_COPPER_LAYER_FOR( layer ) );
        view->UpdateLayerColor( CLEARANCE_LAYER_FOR( layer ) );
    }

    // Update the bitmap of the layer box
    if( m_frame->IsType( FRAME_PCB_EDITOR ) )
        static_cast<PCB_EDIT_FRAME*>( m_frame )->ReCreateLayerBox( false );

    m_frame->GetCanvas()->Refresh();

    if( layer == LAYER_PCB_BACKGROUND )
        m_frame->SetDrawBgColor( newColor );

    passOnFocus();
}


void APPEARANCE_CONTROLS::setNetclassColor( const wxString& netclassName, const COLOR4D& aColor )
{
    BOARD* board = m_frame->GetBoard();

    if( !board )
        return;

    std::shared_ptr<NET_SETTINGS>& netSettings = board->GetDesignSettings().m_NetSettings;
    std::shared_ptr<NETCLASS>      nc = netSettings->GetNetClassByName( netclassName );

    nc->SetPcbColor( aColor );
    netSettings->RecomputeEffectiveNetclasses();

    m_frame->GetCanvas()->GetView()->UpdateAllLayersColor();
    m_frame->GetCanvas()->RedrawRatsnest();
    m_frame->GetCanvas()->Refresh();
}


void APPEARANCE_CONTROLS::setNetColorMode( NET_COLOR_MODE aMode )
{
    PCB_DISPLAY_OPTIONS options = m_frame->GetDisplayOptions();
    options.m_NetColorMode = aMode;

    m_frame->SetDisplayOptions( options );
    m_frame->GetCanvas()->GetView()->UpdateAllLayersColor();
    passOnFocus();
}


void APPEARANCE_CONTROLS::setRatsnestDisplay( bool aShow, RATSNEST_MODE aMode )
{
    if( PCBNEW_SETTINGS* cfg = m_frame->GetPcbNewSettings() )
    {
        cfg->m_Display.m_ShowGlobalRatsnest = aShow;

        if( aShow )
            cfg->m_Display.m_RatsnestMode = aMode;
    }

    if( PCB_EDIT_FRAME* editframe = dynamic_cast<PCB_EDIT_FRAME*>( m_frame ) )
    {
        if( PCBNEW_SETTINGS* cfg = m_frame->GetPcbNewSettings() )
            editframe->SetElementVisibility( LAYER_RATSNEST, cfg->m_Display.m_ShowGlobalRatsnest );

        editframe->OnDisplayOptionsChanged();
        editframe->GetCanvas()->RedrawRatsnest();
        editframe->GetCanvas()->Refresh();
    }

    passOnFocus();
}


void APPEARANCE_CONTROLS::doNetclassContextMenu( int aId )
{
    BOARD* board = m_frame->GetBoard();

    if( !board )
        return;

    KIGFX::VIEW*                   view = m_frame->GetCanvas()->GetView();
    KIGFX::PCB_RENDER_SETTINGS*    rs =
            static_cast<KIGFX::PCB_RENDER_SETTINGS*>( view->GetPainter()->GetSettings() );

    std::shared_ptr<NET_SETTINGS>& netSettings = board->GetDesignSettings().m_NetSettings;
    APPEARANCE_SETTING*            setting = nullptr;

    auto it = m_netclassSettingsMap.find( m_contextMenuNetclass );

    if( it != m_netclassSettingsMap.end() )
        setting = it->second;

    auto runOnNetsOfClass =
            [&]( const wxString& netClassName, std::function<void( NETINFO_ITEM* )> aFunction )
            {
                for( NETINFO_ITEM* net : board->GetNetInfo() )
                {
                    if( net->GetNetClass()->ContainsNetclassWithName( netClassName ) )
                        aFunction( net );
                }
            };

    switch( aId )
    {
        case ID_SET_NET_COLOR:
        {
            if( setting )
            {
                setting->ctl_color->GetNewSwatchColor();

                COLOR4D color = setting->ctl_color->GetSwatchColor();

                if( color != COLOR4D::UNSPECIFIED )
                {
                    netSettings->GetNetClassByName( m_contextMenuNetclass )->SetPcbColor( color );
                    netSettings->RecomputeEffectiveNetclasses();
                }

                view->UpdateAllLayersColor();
            }

            break;
        }

        case ID_CLEAR_NET_COLOR:
        {
            if( setting )
            {
                setting->ctl_color->SetSwatchColor( COLOR4D( 0, 0, 0, 0 ), true );

                netSettings->GetNetClassByName( m_contextMenuNetclass )->SetPcbColor( COLOR4D::UNSPECIFIED );
                netSettings->RecomputeEffectiveNetclasses();

                view->UpdateAllLayersColor();
            }

            break;
        }

        case ID_USE_SCHEMATIC_NET_COLOR:
        {
            if( setting )
            {
                std::shared_ptr<NETCLASS> nc = netSettings->GetNetClassByName( m_contextMenuNetclass );
                const KIGFX::COLOR4D ncColor = nc->GetSchematicColor();

                setting->ctl_color->SetSwatchColor( ncColor, true );

                netSettings->GetNetClassByName( m_contextMenuNetclass )->SetPcbColor( ncColor );
                netSettings->RecomputeEffectiveNetclasses();

                view->UpdateAllLayersColor();
            }

            break;
        }

        case ID_HIGHLIGHT_NET:
        {
            if( !m_contextMenuNetclass.IsEmpty() )
            {
                runOnNetsOfClass( m_contextMenuNetclass,
                        [&]( NETINFO_ITEM* aItem )
                        {
                            static bool first = true;
                            int code = aItem->GetNetCode();

                            if( first )
                            {
                                board->SetHighLightNet( code );
                                rs->SetHighlight( true, code );
                                first = false;
                            }
                            else
                            {
                                board->SetHighLightNet( code, true );
                                rs->SetHighlight( true, code, true );
                            }
                        } );

                view->UpdateAllLayersColor();
                board->HighLightON();
            }

            break;
        }

        case ID_SELECT_NET:
        case ID_DESELECT_NET:
        {
            if( !m_contextMenuNetclass.IsEmpty() )
            {
                TOOL_MANAGER* toolMgr = m_frame->GetToolManager();
                TOOL_ACTION&  action = aId == ID_SELECT_NET ? PCB_ACTIONS::selectNet
                                                                       : PCB_ACTIONS::deselectNet;

                runOnNetsOfClass( m_contextMenuNetclass,
                        [&]( NETINFO_ITEM* aItem )
                        {
                            toolMgr->RunAction( action, aItem->GetNetCode() );
                        } );
            }
            break;
        }


        case ID_SHOW_ALL_NETS:
        {
            showNetclass( NETCLASS::Default );
            KI_ASSERT( m_netclassSettingsMap.count( NETCLASS::Default ) );
            m_netclassSettingsMap.at( NETCLASS::Default )->ctl_visibility->SetValue( true );

            for( const auto& [name, netclass] : netSettings->GetNetclasses() )
            {
                showNetclass( name );

                if( m_netclassSettingsMap.count( name ) )
                    m_netclassSettingsMap.at( name )->ctl_visibility->SetValue( true );
            }

            break;
        }

        case ID_HIDE_OTHER_NETS:
        {
            bool showDefault = m_contextMenuNetclass == NETCLASS::Default;
            showNetclass( NETCLASS::Default, showDefault );
            KI_ASSERT( m_netclassSettingsMap.count( NETCLASS::Default ) );
            m_netclassSettingsMap.at( NETCLASS::Default )->ctl_visibility->SetValue( showDefault );

            for( const auto& [name, netclass] : netSettings->GetNetclasses() )
            {
                bool show = ( name == m_contextMenuNetclass );

                showNetclass( name, show );

                if( m_netclassSettingsMap.count( name ) )
                    m_netclassSettingsMap.at( name )->ctl_visibility->SetValue( show );
            }

            break;
        }

        default:
            break;
    }

    m_frame->GetCanvas()->RedrawRatsnest();
    m_frame->GetCanvas()->Refresh();

    m_contextMenuNetclass.clear();
}
