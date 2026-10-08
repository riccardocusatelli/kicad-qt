/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 CERN
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
 * @file appearance_controls_3D.cpp
 * The appearance panel of the 3D viewer: the settings of its rows and what they do (without
 * the GUI toolkit).  Its widgets are in appearance_controls_3D_wx.cpp (and in the Qt build).
 */

#include <../3d_rendering/opengl/render_3d_opengl.h> // Must be included before any GL header

#include <dialogs/appearance_controls_3D.h>

#include <eda_3d_viewer_frame.h>
#include <tools/eda_3d_actions.h>
#include <widgets/color_swatch.h>

#include <algorithm>
#include <core/ki_assert.h>

/// Render Row abbreviation to reduce source width.
#define RR  APPEARANCE_CONTROLS_3D::APPEARANCE_SETTING_3D

/// Template for object appearance settings
const std::vector<APPEARANCE_CONTROLS_3D::APPEARANCE_SETTING_3D> APPEARANCE_CONTROLS_3D::s_layerSettings = {

    //        text                           id                        tooltip
    RR( _HKI( "Board Body" ),    LAYER_3D_BOARD,             _HKI( "Show board body" ) ),
    RR( _HKI( "Plated Barrels" ),LAYER_3D_PLATED_BARRELS,    _HKI( "Show barrels of plated through-holes and vias" ) ),
    RR(  wxS( "F.Cu" ),          LAYER_3D_COPPER_TOP,        _HKI( "Show front copper / surface finish color" ) ),
    RR(  wxS( "B.Cu" ),          LAYER_3D_COPPER_BOTTOM,     _HKI( "Show back copper / surface finish color" ) ),
    RR( _HKI( "Adhesive" ),      LAYER_3D_ADHESIVE,          _HKI( "Show adhesive" ) ),
    RR( _HKI( "Solder Paste" ),  LAYER_3D_SOLDERPASTE,       _HKI( "Show solder paste" ) ),
    RR(  wxS( "F.Silkscreen" ),  LAYER_3D_SILKSCREEN_TOP,    _HKI( "Show front silkscreen" ) ),
    RR(  wxS( "B.Silkscreen" ),  LAYER_3D_SILKSCREEN_BOTTOM, _HKI( "Show back silkscreen" ) ),
    RR(  wxS( "F.Mask" ),        LAYER_3D_SOLDERMASK_TOP,    _HKI( "Show front solder mask" ) ),
    RR(  wxS( "B.Mask" ),        LAYER_3D_SOLDERMASK_BOTTOM, _HKI( "Show back solder mask" ) ),
    RR(  wxS( "User.Drawings" ), LAYER_3D_USER_DRAWINGS,     _HKI( "Show user drawings layer" ) ),
    RR(  wxS( "User.Comments" ), LAYER_3D_USER_COMMENTS,     _HKI( "Show user comments layer" ) ),
    RR(  wxS( "User.Eco1" ),     LAYER_3D_USER_ECO1,         _HKI( "Show user ECO1 layer" ) ),
    RR(  wxS( "User.Eco2" ),     LAYER_3D_USER_ECO2,         _HKI( "Show user ECO2 layer" ) ),
    RR(  wxS( "User.1" ),        LAYER_3D_USER_1,            _HKI( "Show user defined layer 1" ) ),
    RR(  wxS( "User.2" ),        LAYER_3D_USER_2,            _HKI( "Show user defined layer 2" ) ),
    RR(  wxS( "User.3" ),        LAYER_3D_USER_3,            _HKI( "Show user defined layer 3" ) ),
    RR(  wxS( "User.4" ),        LAYER_3D_USER_4,            _HKI( "Show user defined layer 4" ) ),
    RR(  wxS( "User.5" ),        LAYER_3D_USER_5,            _HKI( "Show user defined layer 5" ) ),
    RR(  wxS( "User.6" ),        LAYER_3D_USER_6,            _HKI( "Show user defined layer 6" ) ),
    RR(  wxS( "User.7" ),        LAYER_3D_USER_7,            _HKI( "Show user defined layer 7" ) ),
    RR(  wxS( "User.8" ),        LAYER_3D_USER_8,            _HKI( "Show user defined layer 8" ) ),
    RR(  wxS( "User.9" ),        LAYER_3D_USER_9,            _HKI( "Show user defined layer 9" ) ),
    RR(  wxS( "User.10" ),       LAYER_3D_USER_10,           _HKI( "Show user defined layer 10" ) ),
    RR(  wxS( "User.11" ),       LAYER_3D_USER_11,           _HKI( "Show user defined layer 11" ) ),
    RR(  wxS( "User.12" ),       LAYER_3D_USER_12,           _HKI( "Show user defined layer 12" ) ),
    RR(  wxS( "User.13" ),       LAYER_3D_USER_13,           _HKI( "Show user defined layer 13" ) ),
    RR(  wxS( "User.14" ),       LAYER_3D_USER_14,           _HKI( "Show user defined layer 14" ) ),
    RR(  wxS( "User.15" ),       LAYER_3D_USER_15,           _HKI( "Show user defined layer 15" ) ),
    RR(  wxS( "User.16" ),       LAYER_3D_USER_16,           _HKI( "Show user defined layer 16" ) ),
    RR(  wxS( "User.17" ),       LAYER_3D_USER_17,           _HKI( "Show user defined layer 17" ) ),
    RR(  wxS( "User.18" ),       LAYER_3D_USER_18,           _HKI( "Show user defined layer 18" ) ),
    RR(  wxS( "User.19" ),       LAYER_3D_USER_19,           _HKI( "Show user defined layer 19" ) ),
    RR(  wxS( "User.20" ),       LAYER_3D_USER_20,           _HKI( "Show user defined layer 20" ) ),
    RR(  wxS( "User.21" ),       LAYER_3D_USER_21,           _HKI( "Show user defined layer 21" ) ),
    RR(  wxS( "User.22" ),       LAYER_3D_USER_22,           _HKI( "Show user defined layer 22" ) ),
    RR(  wxS( "User.23" ),       LAYER_3D_USER_23,           _HKI( "Show user defined layer 23" ) ),
    RR(  wxS( "User.24" ),       LAYER_3D_USER_24,           _HKI( "Show user defined layer 24" ) ),
    RR(  wxS( "User.25" ),       LAYER_3D_USER_25,           _HKI( "Show user defined layer 25" ) ),
    RR(  wxS( "User.26" ),       LAYER_3D_USER_26,           _HKI( "Show user defined layer 26" ) ),
    RR(  wxS( "User.27" ),       LAYER_3D_USER_27,           _HKI( "Show user defined layer 27" ) ),
    RR(  wxS( "User.28" ),       LAYER_3D_USER_28,           _HKI( "Show user defined layer 28" ) ),
    RR(  wxS( "User.29" ),       LAYER_3D_USER_29,           _HKI( "Show user defined layer 29" ) ),
    RR(  wxS( "User.30" ),       LAYER_3D_USER_30,           _HKI( "Show user defined layer 30" ) ),
    RR(  wxS( "User.31" ),       LAYER_3D_USER_31,           _HKI( "Show user defined layer 31" ) ),
    RR(  wxS( "User.32" ),       LAYER_3D_USER_32,           _HKI( "Show user defined layer 32" ) ),
    RR(  wxS( "User.33" ),       LAYER_3D_USER_33,           _HKI( "Show user defined layer 33" ) ),
    RR(  wxS( "User.34" ),       LAYER_3D_USER_34,           _HKI( "Show user defined layer 34" ) ),
    RR(  wxS( "User.35" ),       LAYER_3D_USER_35,           _HKI( "Show user defined layer 35" ) ),
    RR(  wxS( "User.36" ),       LAYER_3D_USER_36,           _HKI( "Show user defined layer 36" ) ),
    RR(  wxS( "User.37" ),       LAYER_3D_USER_37,           _HKI( "Show user defined layer 37" ) ),
    RR(  wxS( "User.38" ),       LAYER_3D_USER_38,           _HKI( "Show user defined layer 38" ) ),
    RR(  wxS( "User.39" ),       LAYER_3D_USER_39,           _HKI( "Show user defined layer 39" ) ),
    RR(  wxS( "User.40" ),       LAYER_3D_USER_40,           _HKI( "Show user defined layer 40" ) ),
    RR(  wxS( "User.41" ),       LAYER_3D_USER_41,           _HKI( "Show user defined layer 41" ) ),
    RR(  wxS( "User.42" ),       LAYER_3D_USER_42,           _HKI( "Show user defined layer 42" ) ),
    RR(  wxS( "User.43" ),       LAYER_3D_USER_43,           _HKI( "Show user defined layer 43" ) ),
    RR(  wxS( "User.44" ),       LAYER_3D_USER_44,           _HKI( "Show user defined layer 44" ) ),
    RR(  wxS( "User.45" ),       LAYER_3D_USER_45,           _HKI( "Show user defined layer 45" ) ),
    RR(),
    RR( _HKI( "Through-hole Models" ),    LAYER_3D_TH_MODELS,         EDA_3D_ACTIONS::showTHT ),
    RR( _HKI( "SMD Models" ),             LAYER_3D_SMD_MODELS,        EDA_3D_ACTIONS::showSMD ),
    RR( _HKI( "Virtual Models" ),         LAYER_3D_VIRTUAL_MODELS,    EDA_3D_ACTIONS::showVirtual ),
    RR( _HKI( "Models not in POS File" ), LAYER_3D_MODELS_NOT_IN_POS, EDA_3D_ACTIONS::showNotInPosFile ),
    RR( _HKI( "Models marked DNP" ),      LAYER_3D_MODELS_MARKED_DNP, EDA_3D_ACTIONS::showDNP ),
    RR( _HKI( "Model Bounding Boxes" ),   LAYER_3D_BOUNDING_BOXES,    EDA_3D_ACTIONS::showBBoxes ),
    RR(),
    RR( _HKI( "Values" ),               LAYER_FP_VALUES,           _HKI( "Show footprint values" ) ),
    RR( _HKI( "References" ),           LAYER_FP_REFERENCES,       _HKI( "Show footprint references" ) ),
    RR( _HKI( "Footprint Text" ),       LAYER_FP_TEXT,             _HKI( "Show all footprint text" ) ),
    RR( _HKI( "Off-board Silkscreen" ), LAYER_3D_OFF_BOARD_SILK,   _HKI( "Do not clip silk layers to board outline" ) ),
    RR(),
    RR( _HKI( "3D Navigator" ),         LAYER_3D_NAVIGATOR,         EDA_3D_ACTIONS::showNavigator ),
    RR( _HKI( "Background Start" ),     LAYER_3D_BACKGROUND_TOP,    _HKI( "Background gradient start color" ) ),
    RR( _HKI( "Background End" ),       LAYER_3D_BACKGROUND_BOTTOM, _HKI( "Background gradient end color" ) ),
};

// The list of IDs that can have colors coming from the board stackup, and cannot be
// modified if use colors from stackup is activated
const std::vector<int> APPEARANCE_CONTROLS_3D::s_inStackupColors{ LAYER_3D_BOARD,
                                         LAYER_3D_COPPER_TOP, LAYER_3D_COPPER_BOTTOM,
                                         LAYER_3D_SOLDERPASTE,
                                         LAYER_3D_SILKSCREEN_TOP, LAYER_3D_SILKSCREEN_BOTTOM,
                                         LAYER_3D_SOLDERMASK_TOP, LAYER_3D_SOLDERMASK_BOTTOM
                                        };

void APPEARANCE_CONTROLS_3D::CommonSettingsChanged()
{
    rebuildControls();

    UpdateLayerCtls();

    const wxString& currentPreset = m_frame->GetAdapter().m_Cfg->m_CurrentPreset;

    if( currentPreset == FOLLOW_PCB || currentPreset == FOLLOW_PLOT_SETTINGS )
        updateLayerPresetWidget( currentPreset );
    else
        syncLayerPresetSelection();
}


void APPEARANCE_CONTROLS_3D::ApplyLayerPreset( const wxString& aPresetName )
{
    doApplyLayerPreset( aPresetName );

    // Move to front of MRU list
    if( m_presetMRU.Index( aPresetName ) != wxNOT_FOUND )
        m_presetMRU.Remove( aPresetName );

    m_presetMRU.Insert( aPresetName, 0 );

    updateLayerPresetWidget( aPresetName );
}


std::vector<VIEWPORT3D> APPEARANCE_CONTROLS_3D::GetUserViewports() const
{
    std::vector<VIEWPORT3D> ret;

    for( const auto& [name, viewport] : m_viewports )
        ret.emplace_back( viewport );

    return ret;
}


void APPEARANCE_CONTROLS_3D::SetUserViewports( std::vector<VIEWPORT3D>& aViewportList )
{
    m_viewports.clear();

    for( const VIEWPORT3D& viewport : aViewportList )
    {
        if( m_viewports.count( viewport.name ) )
            continue;

        m_viewports[viewport.name] = viewport;

        m_viewportMRU.Add( viewport.name );
    }

    rebuildViewportsWidget();

    // Now is as good a time as any to initialize the layer presets as well.
    rebuildLayerPresetsWidget( true );

    m_presetMRU.Add( FOLLOW_PCB );
    m_presetMRU.Add( FOLLOW_PLOT_SETTINGS );

    for( const LAYER_PRESET_3D& preset : m_frame->GetAdapter().m_Cfg->m_LayerPresets )
        m_presetMRU.Add( preset.name );
}


void APPEARANCE_CONTROLS_3D::OnLayerVisibilityChanged( int aLayer, bool isVisible )
{
    std::bitset<LAYER_3D_END>     visibleLayers = m_frame->GetAdapter().GetVisibleLayers();
    const std::map<int, COLOR4D>& colors = m_frame->GetAdapter().GetLayerColors();
    bool                          killFollow = false;
    bool                          doFastRefresh = false;    // true to just refresh the display

    // Special-case controls
    switch( aLayer )
    {
    case LAYER_FP_TEXT:
        // Because Footprint Text is a meta-control that also can disable values/references,
        // drag them along here so that the user is less likely to be confused.
        if( !isVisible )
        {
            visibleLayers.set( LAYER_FP_REFERENCES, false );
            visibleLayers.set( LAYER_FP_VALUES, false );
        }

        visibleLayers.set( LAYER_FP_TEXT, isVisible );
        killFollow = true;
        break;

    case LAYER_FP_REFERENCES:
    case LAYER_FP_VALUES:
        // In case that user changes Footprint Value/References when the Footprint Text
        // meta-control is disabled, we should put it back on.
        if( isVisible )
            visibleLayers.set( LAYER_FP_TEXT, true );

        visibleLayers.set( aLayer, isVisible );
        killFollow = true;
        break;

    case LAYER_3D_BOARD:
    case LAYER_3D_COPPER_TOP:
    case LAYER_3D_COPPER_BOTTOM:
    case LAYER_3D_PLATED_BARRELS:
    case LAYER_3D_SILKSCREEN_BOTTOM:
    case LAYER_3D_SILKSCREEN_TOP:
    case LAYER_3D_SOLDERMASK_BOTTOM:
    case LAYER_3D_SOLDERMASK_TOP:
    case LAYER_3D_SOLDERPASTE:
    case LAYER_3D_ADHESIVE:
    case LAYER_3D_USER_COMMENTS:
    case LAYER_3D_USER_DRAWINGS:
    case LAYER_3D_USER_ECO1:
    case LAYER_3D_USER_ECO2:
        visibleLayers.set( aLayer, isVisible );
        killFollow = true;
        break;

    case LAYER_3D_TH_MODELS:
    case LAYER_3D_SMD_MODELS:
    case LAYER_3D_VIRTUAL_MODELS:
    case LAYER_3D_MODELS_NOT_IN_POS:
    case LAYER_3D_MODELS_MARKED_DNP:
        doFastRefresh = true;
        visibleLayers.set( aLayer, isVisible );
        break;

    default:
        visibleLayers.set( aLayer, isVisible );

        if( aLayer >= LAYER_3D_USER_1 && aLayer <= LAYER_3D_USER_45 )
            killFollow = true;

        break;
    }

    m_frame->GetAdapter().SetVisibleLayers( visibleLayers );
    m_frame->GetAdapter().SetLayerColors( colors );

    const wxString& currentPreset = m_frame->GetAdapter().m_Cfg->m_CurrentPreset;

    if( ( currentPreset != FOLLOW_PCB && currentPreset != FOLLOW_PLOT_SETTINGS ) || killFollow )
        syncLayerPresetSelection();

    UpdateLayerCtls();

    if( doFastRefresh && m_frame->GetAdapter().m_Cfg->m_Render.engine == RENDER_ENGINE::OPENGL )
    {
        RENDER_3D_OPENGL* renderer = static_cast<RENDER_3D_OPENGL*>( m_frame->GetCanvas()->GetCurrentRender() );
        renderer->Load3dModelsIfNeeded();
        m_frame->GetCanvas()->Request_refresh();
    }
    else
    {
        m_frame->NewDisplay( true );
    }
}


void APPEARANCE_CONTROLS_3D::onColorSwatchChanged( COLOR_SWATCH* aSwatch )
{
    std::bitset<LAYER_3D_END> visibleLayers = m_frame->GetAdapter().GetVisibleLayers();
    std::map<int, COLOR4D>    colors = m_frame->GetAdapter().GetLayerColors();

    m_frame->GetAdapter().SetVisibleLayers( visibleLayers );

    int     layer = aSwatch->GetId();
    COLOR4D newColor = aSwatch->GetSwatchColor();

    colors[ layer ] = newColor;

    // The internals of the 3D viewer only supports a single color for copper, which must
    // be applied to all copper layers.
    COLOR_SWATCH* otherSwatch = nullptr;

    const std::vector<int> copperIndices{
        LAYER_3D_COPPER_TOP,
        LAYER_3D_COPPER_BOTTOM,
        LAYER_3D_PLATED_BARRELS,
    };

    // If the changed swatch is one of the copper layers, we need to update the other copper layers
    if( std::find( copperIndices.begin(), copperIndices.end(), layer ) != copperIndices.end() )
    {
        for( int index : copperIndices )
        {
            if( layer != index && colors.count( index ) )
            {
                colors[index] = newColor;
                otherSwatch = m_layerSettingsMap[index]->m_Ctl_color;
            }
        }
    }

    if( otherSwatch )
    {
        // Don't send an event, because that will cause an event loop
        otherSwatch->SetSwatchColor( newColor, false );
    }


    m_frame->GetAdapter().SetLayerColors( colors );

    const wxString& currentPreset = m_frame->GetAdapter().m_Cfg->m_CurrentPreset;

    if( currentPreset == FOLLOW_PCB || currentPreset == FOLLOW_PLOT_SETTINGS )
        updateLayerPresetWidget( currentPreset );
    else
        syncLayerPresetSelection();

    m_frame->NewDisplay( true );
}


void APPEARANCE_CONTROLS_3D::doApplyLayerPreset( const wxString& aPresetName )
{
    BOARD_ADAPTER& adapter = m_frame->GetAdapter();

    if( aPresetName == FOLLOW_PCB || aPresetName == FOLLOW_PLOT_SETTINGS )
    {
        adapter.m_Cfg->m_CurrentPreset = aPresetName;
        adapter.SetVisibleLayers( adapter.GetVisibleLayers() );
    }
    else if( LAYER_PRESET_3D* preset = adapter.m_Cfg->FindPreset( aPresetName ) )
    {
        adapter.m_Cfg->m_CurrentPreset = aPresetName;
        adapter.SetVisibleLayers( preset->layers );
        adapter.SetLayerColors( preset->colors );

        if( preset->name.Lower() == _( "legacy colors" ) )
            adapter.m_Cfg->m_UseStackupColors = false;
    }
    else
    {
        return;
    }

    UpdateLayerCtls();
    m_frame->NewDisplay( true );
}


void APPEARANCE_CONTROLS_3D::doApplyViewport( const VIEWPORT3D& aViewport )
{
    m_frame->GetCurrentCamera().SetViewMatrix( aViewport.matrix );

    if( m_frame->GetAdapter().m_Cfg->m_Render.engine == RENDER_ENGINE::OPENGL )
        m_frame->GetCanvas()->Request_refresh();
    else
        m_frame->GetCanvas()->RenderRaytracingRequest();
}
