/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 2023 CERN
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

#include <wx/ffile.h>
#include <wx/filename.h>
#include <wx/string.h>
#include <raster_image.h>
#include "eda_3d_viewer_frame.h"
#include "dialogs/appearance_controls_3D.h"
#include <confirm.h>
#include <dialogs/eda_view_switcher.h>
#include <eda_3d_viewer_settings.h>
#include <3d_rendering/raytracing/render_3d_raytrace_ram.h>
#include <3d_viewer_id.h>
#include <3d_viewer/tools/eda_3d_actions.h>
#include <3d_viewer/tools/eda_3d_controller.h>
#include <3d_viewer/tools/eda_3d_conditions.h>
#include <board.h>
#include <advanced_config.h>
#include <bitmaps.h>
#include <board_design_settings.h>
#include <dpi_scaling_common.h>
#include <pgm_base.h>
#include <project.h>
#include <project/project_file.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <tool/action_manager.h>
#include <tool/common_control.h>
#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>
#include <tool/action_toolbar.h>
#include <widgets/wx_infobar.h>
#include <wildcards_and_files_ext.h>
#include <project_pcb.h>
#include <toolbars_3d.h>
#include <core/ki_log.h>
#include <core/ki_assert.h>

#if defined(__linux__) || defined(__FreeBSD__)
#include <spacenav/libspnav_driver.h>
#include <3d_spacenav/spnav_viewer_plugin.h>
#else
#include <3d_navlib/nl_3d_viewer_plugin.h>
#endif

/**
 * Flag to enable 3D viewer main frame window debug tracing.
 *
 * Use "KI_TRACE_EDA_3D_VIEWER" to enable.
 *
 * @ingroup trace_env_vars
 */
const wxChar* EDA_3D_VIEWER_FRAME::m_logTrace = wxT( "KI_TRACE_EDA_3D_VIEWER" );


EDA_3D_VIEWER_FRAME::EDA_3D_VIEWER_FRAME( KIWAY* aKiway, PCB_BASE_FRAME* aParent,
                                          const wxString& aTitle, long style ) :
        KIWAY_PLAYER( aKiway, aParent, FRAME_PCB_DISPLAY3D, aTitle, wxDefaultPosition,
                      wxDefaultSize, style, QUALIFIED_VIEWER3D_FRAMENAME( aParent ), unityScale ),
        m_canvas( nullptr ),
        m_currentCamera( m_trackBallCamera ),
        m_trackBallCamera( 2 * RANGE_SCALE_3D )
{
    KI_LOG_TRACE( m_logTrace, wxT( "EDA_3D_VIEWER_FRAME::EDA_3D_VIEWER_FRAME %s" ), aTitle );

    m_disable_ray_tracing = false;
    m_aboutTitle = _HKI( "KiCad 3D Viewer" );

    ANTIALIASING_MODE       aaMode = ANTIALIASING_MODE::AA_NONE;
    EDA_3D_VIEWER_SETTINGS* cfg = GetAppSettings<EDA_3D_VIEWER_SETTINGS>( "3d_viewer" );

    if( cfg )
        aaMode = static_cast<ANTIALIASING_MODE>( cfg->m_Render.opengl_AA_mode );

    m_canvas = new EDA_3D_CANVAS( this, OGL_ATT_LIST::GetAttributesList( aaMode, true ), m_boardAdapter,
                                  m_currentCamera, PROJECT_PCB::Get3DCacheManager( &Prj() ) );

    m_appearancePanel = new APPEARANCE_CONTROLS_3D( this, GetCanvas() );

    LoadSettings( GetAppSettings<EDA_3D_VIEWER_SETTINGS>( "3d_viewer" ) );
    loadCommonSettings();

    m_appearancePanel->SetUserViewports( Prj().GetProjectFile().m_Viewports3D );

    // Create the manager
    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( GetBoard(), nullptr, nullptr,
                                   GetAppSettings<EDA_3D_VIEWER_SETTINGS>( "3d_viewer" ), this );

    m_actions = new EDA_3D_ACTIONS();
    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager );
    m_canvas->SetEventDispatcher( m_toolDispatcher );

    // Register tools
    m_toolManager->RegisterTool( new COMMON_CONTROL );
    m_toolManager->RegisterTool( new EDA_3D_CONTROLLER );
    m_toolManager->InitTools();

    setupUIConditions();

    if( EDA_3D_CONTROLLER* ctrlTool = GetToolManager()->GetTool<EDA_3D_CONTROLLER>() )
        ctrlTool->SetRotationIncrement( cfg ? cfg->m_Camera.rotation_increment : 10.0 );

    // Run the viewer control tool, it is supposed to be always active
    m_toolManager->InvokeTool( "3DViewer.Control" );

    ReCreateMenuBar();

    m_toolbarSettings = Pgm().GetSettingsManager().GetToolbarSettings<EDA_3D_VIEWER_TOOLBAR_SETTINGS>( "3d_viewer-toolbars" );
    configureToolbars();
    RecreateToolbars();

    setupWindow( cfg );

    BindMenuCommand( wxID_CLOSE,
                     [this]()
                     {
                         KI_LOG_TRACE( m_logTrace, wxT( "EDA_3D_VIEWER_FRAME::Exit3DFrame" ) );
                         Close( true );
                     } );

    BindMenuCommand( ID_MENU3D_RESET_DEFAULTS,
                     [this]()
                     {
                         resetDefaults();
                     } );

    m_canvas->SetRaytracingUnsupportedHandler(
            [this]()
            {
                onDisableRayTracing();
            } );

    try
    {
#if defined(__linux__) || defined(__FreeBSD__)
        m_spaceMouse = std::make_unique<SPNAV_VIEWER_PLUGIN>( m_canvas );
#else
        m_spaceMouse = std::make_unique<NL_3D_VIEWER_PLUGIN>( m_canvas );
#endif
    }
    catch( const std::exception& e )
    {
        KI_LOG_TRACE( wxT( "KI_TRACE_NAVLIB" ), wxS( "%s" ), e.what() );
    }
    catch( ... )
    {
        KI_LOG_TRACE( wxT( "KI_TRACE_NAVLIB" ),
                    wxT( "Unknown exception during SpaceMouse initialization" ) );
    }

    // Fixes bug in Windows (XP and possibly others) where the canvas requires the focus
    // in order to receive mouse events.  Otherwise, the user has to click somewhere on
    // the canvas before it will respond to mouse wheel events.
    m_canvas->SetFocus();
}


EDA_3D_VIEWER_FRAME::~EDA_3D_VIEWER_FRAME()
{
    // Shutdown all running tools
    if( m_toolManager )
        m_toolManager->ShutdownAllTools();

    Prj().GetProjectFile().m_Viewports3D = m_appearancePanel->GetUserViewports();

    m_canvas->SetEventDispatcher( nullptr );

    releaseWindow();
}


APP_SETTINGS_BASE* EDA_3D_VIEWER_FRAME::config() const
{
    return GetAppSettings<EDA_3D_VIEWER_SETTINGS>( "3d_viewer" );
}


void EDA_3D_VIEWER_FRAME::setupUIConditions()
{
    EDA_BASE_FRAME::setupUIConditions();

    ACTION_MANAGER*   mgr = m_toolManager->GetActionManager();
    EDA_3D_CONDITIONS cond( &m_boardAdapter );

// Helper to define check conditions
#define GridSizeCheck( x ) ACTION_CONDITIONS().Check( cond.GridSize( x ) )

    auto raytracing =
            [this]( const SELECTION& aSel )
            {
                return m_boardAdapter.m_Cfg->m_Render.engine != RENDER_ENGINE::OPENGL;
            };
    auto showTH =
            [this]( const SELECTION& aSel )
            {
                return m_boardAdapter.m_Cfg->m_Render.show_footprints_normal;
            };
    auto showSMD =
            [this]( const SELECTION& aSel )
            {
                return m_boardAdapter.m_Cfg->m_Render.show_footprints_insert;
            };
    auto showVirtual =
            [this]( const SELECTION& aSel )
            {
                return m_boardAdapter.m_Cfg->m_Render.show_footprints_virtual;
            };
    auto show_NotInPosfile =
            [this]( const SELECTION& aSel )
            {
                return m_boardAdapter.m_Cfg->m_Render.show_footprints_not_in_posfile;
            };
    auto show_DNP =
            [this]( const SELECTION& aSel )
            {
                return m_boardAdapter.m_Cfg->m_Render.show_footprints_dnp;
            };
    auto showBBoxes =
            [this]( const SELECTION& aSel )
            {
                return m_boardAdapter.m_Cfg->m_Render.show_model_bbox;
            };
    auto showNavig =
            [this]( const SELECTION& aSel )
            {
                return m_boardAdapter.m_Cfg->m_Render.show_navigator;
            };
    auto ortho =
            [this]( const SELECTION& )
            {
                return m_currentCamera.GetProjection() == PROJECTION_TYPE::ORTHO;
            };

    auto appearances =
            [this]( const SELECTION& aSel )
            {
                return m_boardAdapter.m_Cfg->m_AuiPanels.show_layer_manager;
            };

    mgr->SetConditions( EDA_3D_ACTIONS::toggleRaytacing, ACTION_CONDITIONS().Check( raytracing ) );

    mgr->SetConditions( EDA_3D_ACTIONS::showTHT, ACTION_CONDITIONS().Check( showTH ) );
    mgr->SetConditions( EDA_3D_ACTIONS::showSMD, ACTION_CONDITIONS().Check( showSMD ) );
    mgr->SetConditions( EDA_3D_ACTIONS::showVirtual, ACTION_CONDITIONS().Check( showVirtual ) );
    mgr->SetConditions( EDA_3D_ACTIONS::showNotInPosFile,
                        ACTION_CONDITIONS().Check( show_NotInPosfile ) );
    mgr->SetConditions( EDA_3D_ACTIONS::showDNP, ACTION_CONDITIONS().Check( show_DNP ) );

    mgr->SetConditions( EDA_3D_ACTIONS::showBBoxes, ACTION_CONDITIONS().Check( showBBoxes ) );
    mgr->SetConditions( EDA_3D_ACTIONS::showNavigator, ACTION_CONDITIONS().Check( showNavig ) );

    mgr->SetConditions( EDA_3D_ACTIONS::noGrid, GridSizeCheck( GRID3D_TYPE::NONE ) );
    mgr->SetConditions( EDA_3D_ACTIONS::show10mmGrid, GridSizeCheck( GRID3D_TYPE::GRID_10MM ) );
    mgr->SetConditions( EDA_3D_ACTIONS::show5mmGrid, GridSizeCheck( GRID3D_TYPE::GRID_5MM ) );
    mgr->SetConditions( EDA_3D_ACTIONS::show2_5mmGrid, GridSizeCheck( GRID3D_TYPE::GRID_2P5MM ) );
    mgr->SetConditions( EDA_3D_ACTIONS::show1mmGrid, GridSizeCheck( GRID3D_TYPE::GRID_1MM ) );
    mgr->SetConditions( EDA_3D_ACTIONS::toggleOrtho, ACTION_CONDITIONS().Check( ortho ) );

    mgr->SetConditions( EDA_3D_ACTIONS::showLayersManager,
                        ACTION_CONDITIONS().Check( appearances ) );

#undef GridSizeCheck
}


bool EDA_3D_VIEWER_FRAME::handleViewSwitcherKey( int aSwitchKey )
{
    static bool s_presetSwitcherShown = false;
    static bool s_viewportSwitcherShown = false;

    if( aSwitchKey == PRESET_SWITCH_KEY && !s_presetSwitcherShown )
    {
        if( m_appearancePanel && this->IsActive() )
        {
            wxArrayString mru = m_appearancePanel->GetLayerPresetsMRU();

            if( mru.size() > 0 )
            {
                for( wxString& str : mru )
                {
                    if( str == FOLLOW_PCB )
                        str = _( "Follow PCB Editor" );
                    else if( str == FOLLOW_PLOT_SETTINGS )
                        str = _( "Follow PCB Plot Settings" );
                }

                EDA_VIEW_SWITCHER switcher( this, mru, PRESET_SWITCH_KEY );

                s_presetSwitcherShown = true;
                switcher.ShowModal();
                s_presetSwitcherShown = false;

                int idx = switcher.GetSelection();

                if( idx >= 0 && idx < (int) mru.size() )
                {
                    wxString internalName = m_appearancePanel->GetLayerPresetsMRU()[idx];
                    m_appearancePanel->ApplyLayerPreset( internalName );
                }

                return true;
            }
        }
    }
    else if( aSwitchKey == VIEWPORT_SWITCH_KEY && !s_viewportSwitcherShown )
    {
        if( this->IsActive() )
        {
            const wxArrayString& viewportMRU = m_appearancePanel->GetViewportsMRU();

            if( viewportMRU.size() > 0 )
            {
                EDA_VIEW_SWITCHER switcher( this, viewportMRU, VIEWPORT_SWITCH_KEY );

                s_viewportSwitcherShown = true;
                switcher.ShowModal();
                s_viewportSwitcherShown = false;

                int idx = switcher.GetSelection();

                if( idx >= 0 && idx < (int) viewportMRU.size() )
                    m_appearancePanel->ApplyViewport( viewportMRU[idx] );

                return true;
            }
        }
    }

    return false;
}


void EDA_3D_VIEWER_FRAME::handleIconizeEvent( bool aIconized )
{
    KIWAY_PLAYER::handleIconizeEvent( aIconized );

    if( m_spaceMouse && aIconized )
        m_spaceMouse->SetFocus( false );
}


void EDA_3D_VIEWER_FRAME::ReloadRequest()
{
    // This will schedule a request to load later
    // ReloadRequest also updates the board pointer so always call it first
    if( m_canvas )
        m_canvas->ReloadRequest( GetBoard(), PROJECT_PCB::Get3DCacheManager( &Prj() ) );

    if( m_appearancePanel )
        m_appearancePanel->UpdateLayerCtls();
}


void EDA_3D_VIEWER_FRAME::NewDisplay( bool aForceImmediateRedraw )
{
    if( m_canvas )
        m_canvas->ReloadRequest( GetBoard(), PROJECT_PCB::Get3DCacheManager( &Prj() ) );

    // After the ReloadRequest call, the refresh often takes a bit of time,
    // and it is made here only on request.
    if( m_canvas && aForceImmediateRedraw )
        m_canvas->Refresh();
}


void EDA_3D_VIEWER_FRAME::Redraw()
{
    // Only update in OpenGL for an interactive interaction
    if( m_boardAdapter.m_Cfg->m_Render.engine == RENDER_ENGINE::OPENGL )
        m_canvas->Request_refresh( true );
}


void EDA_3D_VIEWER_FRAME::refreshRender()
{
    if( m_boardAdapter.m_Cfg->m_Render.engine == RENDER_ENGINE::OPENGL )
        m_canvas->Request_refresh();
    else
        NewDisplay( true );
}


void EDA_3D_VIEWER_FRAME::resetDefaults()
{
    if( m_canvas == nullptr )
        return;

    m_boardAdapter.SetLayerColors( m_boardAdapter.GetDefaultColors() );

    if( EDA_3D_VIEWER_SETTINGS* cfg = GetAppSettings<EDA_3D_VIEWER_SETTINGS>( "3d_viewer" ) )
        cfg->ResetToDefaults();

    LoadSettings( GetAppSettings<EDA_3D_VIEWER_SETTINGS>( "3d_viewer" ) );

    // Tell canvas that we (may have) changed the render engine
    RenderEngineChanged();
    NewDisplay( true );
}


void EDA_3D_VIEWER_FRAME::onDisableRayTracing()
{
    KI_LOG_TRACE( m_logTrace, wxT( "EDA_3D_VIEWER_FRAME::%s disabling ray tracing." ), __func__ );

    m_disable_ray_tracing = true;
    m_boardAdapter.m_Cfg->m_Render.engine = RENDER_ENGINE::OPENGL;
}


void EDA_3D_VIEWER_FRAME::handleActivateEvent( bool aActive )
{
    KI_LOG_TRACE( m_logTrace, wxT( "EDA_3D_VIEWER_FRAME::OnActivate" ) );

    KIWAY_PLAYER::handleActivateEvent( aActive );

    if( aActive && m_canvas )
    {
        // Reload data if 3D frame shows a board,
        // because it can be changed since last frame activation
        if( m_canvas->IsReloadRequestPending() )
            m_canvas->Request_refresh();

        // Activates again the focus of the canvas so it will catch mouse and key events
        m_canvas->SetFocus();
    }

    if( m_spaceMouse )
        m_spaceMouse->SetFocus( aActive );
}


void EDA_3D_VIEWER_FRAME::LoadSettings( APP_SETTINGS_BASE *aCfg )
{
    EDA_BASE_FRAME::LoadSettings( aCfg );

    // Dynamic_cast here will fail on Mac when called from CvPCB.
    EDA_3D_VIEWER_SETTINGS* cfg = static_cast<EDA_3D_VIEWER_SETTINGS*>( aCfg );

    KI_LOG_TRACE( m_logTrace, wxT( "EDA_3D_VIEWER_FRAME::LoadSettings" ) );

    if( cfg )
    {
        applySettings( cfg );

        m_boardAdapter.SetBoard( GetBoard() );

        // When opening the 3D viewer, we use the OpenGL mode, never the ray tracing engine
        // because the ray tracing is very time consuming, and can be seen as not working
        // (freeze window) with large boards.
        m_boardAdapter.m_Cfg->m_Render.engine = RENDER_ENGINE::OPENGL;

        if( cfg->m_CurrentPreset == LEGACY_PRESET_FLAG )
        {
            wxString legacyColorsPresetName = _( "legacy colors" );

            cfg->m_UseStackupColors = false;

            if( !cfg->FindPreset( legacyColorsPresetName ) )
            {
                cfg->m_LayerPresets.emplace_back( legacyColorsPresetName,
                                                  GetAdapter().GetDefaultVisibleLayers(),
                                                  GetAdapter().GetDefaultColors() );
            }

            cfg->m_CurrentPreset = FOLLOW_PLOT_SETTINGS;
        }

        m_boardAdapter.InitSettings( nullptr, nullptr );

        if( m_appearancePanel )
            m_appearancePanel->CommonSettingsChanged();
    }
}


void EDA_3D_VIEWER_FRAME::SaveSettings( APP_SETTINGS_BASE *aCfg )
{
    EDA_BASE_FRAME::SaveSettings( GetAppSettings<EDA_3D_VIEWER_SETTINGS>( "3d_viewer" ) );

    if( EDA_3D_VIEWER_SETTINGS* cfg = GetAppSettings<EDA_3D_VIEWER_SETTINGS>( "3d_viewer" ) )
    {
        cfg->m_AuiPanels.right_panel_width    = m_appearancePanel->GetSize().x;

        cfg->m_Camera.animation_enabled       = m_canvas->GetAnimationEnabled();
        cfg->m_Camera.moving_speed_multiplier = m_canvas->GetMovingSpeedMultiplier();
        cfg->m_Camera.projection_mode = m_canvas->GetProjectionMode();
    }
}


void EDA_3D_VIEWER_FRAME::CommonSettingsChanged( int aFlags )
{
    KI_LOG_TRACE( m_logTrace, wxT( "EDA_3D_VIEWER_FRAME::CommonSettingsChanged" ) );

    // Regen menu bars, etc
    EDA_BASE_FRAME::CommonSettingsChanged( aFlags );

    loadCommonSettings();

    EDA_3D_VIEWER_SETTINGS* cfg = GetAppSettings<EDA_3D_VIEWER_SETTINGS>( "3d_viewer" );
    applySettings( cfg );

    if( EDA_3D_CONTROLLER* ctrlTool = GetToolManager()->GetTool<EDA_3D_CONTROLLER>() )
        ctrlTool->SetRotationIncrement( cfg ? cfg->m_Camera.rotation_increment : 10.0 );

    m_appearancePanel->CommonSettingsChanged();

    NewDisplay( true );
}


void EDA_3D_VIEWER_FRAME::OnDarkModeToggle()
{
    if( m_appearancePanel )
        m_appearancePanel->OnDarkModeToggle();
}


void EDA_3D_VIEWER_FRAME::TakeScreenshot( EDA_3D_VIEWER_EXPORT_FORMAT aFormat )
{
    wxString fullFileName;

    if( aFormat != EDA_3D_VIEWER_EXPORT_FORMAT::CLIPBOARD )
    {
        if( !getExportFileName( aFormat, fullFileName ) )
            return;
    }

    RASTER_IMAGE screenshotImage = captureCurrentViewScreenshot();

    if( screenshotImage.IsOk() )
    {
        saveOrCopyImage( screenshotImage, aFormat, fullFileName );
    }
}


RASTER_IMAGE EDA_3D_VIEWER_FRAME::captureCurrentViewScreenshot()
{
    // Ensure we have the latest 3D view (remember 3D view is buffered)
    // Also ensure any highlighted item is not highlighted when creating screen shot
    EDA_3D_VIEWER_SETTINGS::RENDER_SETTINGS& cfg = m_boardAdapter.m_Cfg->m_Render;
    bool original_highlight = cfg.highlight_on_rollover;
    cfg.highlight_on_rollover = false;

    m_canvas->DoRePaint();      // init first buffer
    m_canvas->DoRePaint();      // init second buffer

    RASTER_IMAGE screenshotImage;

    if( m_canvas )
    {
        // Build image from the 3D buffer
        screenshotImage = m_canvas->GetScreenshot();
    }

    // Restore highlight setting
    cfg.highlight_on_rollover = original_highlight;

    return screenshotImage;
}


void EDA_3D_VIEWER_FRAME::ExportImage( EDA_3D_VIEWER_EXPORT_FORMAT aFormat, const wxSize& aSize )
{
    wxString fullFileName;

    if( aFormat != EDA_3D_VIEWER_EXPORT_FORMAT::CLIPBOARD )
    {
        if( !getExportFileName( aFormat, fullFileName ) )
            return;
    }

    RASTER_IMAGE screenshotImage = captureScreenshot( aSize );

    if( screenshotImage.IsOk() )
    {
        saveOrCopyImage( screenshotImage, aFormat, fullFileName );
    }
}


RASTER_IMAGE EDA_3D_VIEWER_FRAME::captureScreenshot( const wxSize& aSize )
{
    TRACK_BALL    camera  = m_trackBallCamera;
    camera.SetCurWindowSize( aSize );

    EDA_3D_VIEWER_SETTINGS* cfg = GetAppSettings<EDA_3D_VIEWER_SETTINGS>( "3d_viewer" );
    EDA_3D_VIEWER_SETTINGS* backupCfg = m_boardAdapter.m_Cfg;

    auto configRestorer = std::unique_ptr<void, std::function<void(void*)>>(
        reinterpret_cast<void*>(1),
        [&](void*) { m_boardAdapter.m_Cfg = backupCfg; }
    );

    if( cfg )
        m_boardAdapter.m_Cfg = cfg;

    if( cfg && cfg->m_Render.engine == RENDER_ENGINE::RAYTRACING )
        return captureRaytracingScreenshot( m_boardAdapter, camera, aSize );
    else
        return captureOpenGLScreenshot( m_boardAdapter, camera, aSize );
}


void EDA_3D_VIEWER_FRAME::setupRenderingConfig( BOARD_ADAPTER& aAdapter )
{
    EDA_3D_VIEWER_SETTINGS* cfg = GetAppSettings<EDA_3D_VIEWER_SETTINGS>( "3d_viewer" );

    if( cfg )
        aAdapter.m_Cfg = cfg;
}


RASTER_IMAGE EDA_3D_VIEWER_FRAME::captureRaytracingScreenshot( BOARD_ADAPTER& aAdapter,
                                                                TRACK_BALL& aCamera,
                                                                const wxSize& aSize )
{
    BOARD_ADAPTER tempadapter;
    tempadapter.SetBoard( GetBoard() );
    tempadapter.m_Cfg = aAdapter.m_Cfg;
    tempadapter.InitSettings( nullptr, nullptr );
    tempadapter.Set3dCacheManager( aAdapter.Get3dCacheManager() );

    RENDER_3D_RAYTRACE_RAM raytrace( tempadapter, aCamera );
    raytrace.SetCurWindowSize( aSize );

    while( raytrace.Redraw( false, nullptr, nullptr ) );

    uint8_t* rgbaBuffer = raytrace.GetBuffer();
    wxSize   realSize   = raytrace.GetRealBufferSize();

    if( !rgbaBuffer )
        return RASTER_IMAGE();

    return convertRGBAToImage( rgbaBuffer, realSize );
}


RASTER_IMAGE EDA_3D_VIEWER_FRAME::convertRGBAToImage( uint8_t* aRGBABuffer, const wxSize& aRealSize )
{
    RASTER_IMAGE screenshotImage( aRealSize.x, aRealSize.y );
    screenshotImage.InitAlpha();

    unsigned char* rgbPtr   = screenshotImage.GetData();
    unsigned char* alphaPtr = screenshotImage.GetAlpha();

    // The rows of the buffer go from the bottom to the top: the image is mirrored top-bottom
    for( int y = 0; y < aRealSize.y; y++ )
    {
        const unsigned char* rgbaPtr = aRGBABuffer + (size_t) ( aRealSize.y - 1 - y ) * aRealSize.x * 4;

        for( int x = 0; x < aRealSize.x; x++ )
        {
            rgbPtr[0]   = rgbaPtr[0];
            rgbPtr[1]   = rgbaPtr[1];
            rgbPtr[2]   = rgbaPtr[2];
            alphaPtr[0] = rgbaPtr[3];

            rgbaPtr  += 4;
            rgbPtr   += 3;
            alphaPtr += 1;
        }
    }

    return screenshotImage;
}


RASTER_IMAGE EDA_3D_VIEWER_FRAME::captureOpenGLScreenshot( BOARD_ADAPTER& aAdapter,
                                                            TRACK_BALL& aCamera,
                                                            const wxSize& aSize )
{
    EDA_3D_VIEWER_SETTINGS* cfg = aAdapter.m_Cfg;
    ANTIALIASING_MODE aaMode = cfg ? static_cast<ANTIALIASING_MODE>( cfg->m_Render.opengl_AA_mode )
                                   : ANTIALIASING_MODE::AA_NONE;

    std::unique_ptr<TOOLKIT_WINDOW> temp = TOOLKIT::CreateHiddenWindow( this, aSize );

    BOARD_ADAPTER tempadapter;
    tempadapter.SetBoard( GetBoard() );
    tempadapter.m_Cfg = aAdapter.m_Cfg;
    tempadapter.InitSettings( nullptr, nullptr );
    tempadapter.Set3dCacheManager( aAdapter.Get3dCacheManager() );

    auto canvas = std::make_unique<EDA_3D_CANVAS>( temp.get(),
                                               OGL_ATT_LIST::GetAttributesList( aaMode, true ),
                                               tempadapter, aCamera,
                                               aAdapter.Get3dCacheManager() );

    canvas->ResizeCanvas( aSize.x, aSize.y );
    configureCanvas( canvas, cfg );

    // Temporarily disable highlight during screenshot
    EDA_3D_VIEWER_SETTINGS::RENDER_SETTINGS& renderCfg = aAdapter.m_Cfg->m_Render;
    bool original_highlight = renderCfg.highlight_on_rollover;
    bool original_navigator = renderCfg.show_navigator;
    renderCfg.show_navigator = false;
    renderCfg.highlight_on_rollover = false;

    std::vector<unsigned char> buffer(aSize.x * aSize.y * 4); // RGBA format
    canvas->RenderToFrameBuffer( buffer.data(), aSize.x, aSize.y );
    RASTER_IMAGE result = convertRGBAToImage( buffer.data(), aSize );

    // Restore highlight setting
    renderCfg.highlight_on_rollover = original_highlight;
    renderCfg.show_navigator = original_navigator;

    return result;
}


void EDA_3D_VIEWER_FRAME::configureCanvas( std::unique_ptr<EDA_3D_CANVAS>& aCanvas, EDA_3D_VIEWER_SETTINGS* aCfg )
{
    if( aCfg )
    {
        aCanvas->SetAnimationEnabled( aCfg->m_Camera.animation_enabled );
        aCanvas->SetMovingSpeedMultiplier( aCfg->m_Camera.moving_speed_multiplier );
        aCanvas->SetProjectionMode( aCfg->m_Camera.projection_mode );
    }

    aCanvas->SetVcSettings( EDA_DRAW_PANEL_GAL::GetVcSettings() );
}


void EDA_3D_VIEWER_FRAME::saveOrCopyImage( const RASTER_IMAGE& aScreenshotImage,
                                           EDA_3D_VIEWER_EXPORT_FORMAT aFormat,
                                           const wxString& aFullFileName )
{
    if( aFormat == EDA_3D_VIEWER_EXPORT_FORMAT::CLIPBOARD )
    {
        copyImageToClipboard( aScreenshotImage );
    }
    else
    {
        saveImageToFile( aScreenshotImage, aFormat, aFullFileName );
    }
}


void EDA_3D_VIEWER_FRAME::saveImageToFile( const RASTER_IMAGE& aScreenshotImage,
                                           EDA_3D_VIEWER_EXPORT_FORMAT aFormat,
                                           const wxString& aFullFileName )
{
    bool fmt_is_jpeg = ( aFormat == EDA_3D_VIEWER_EXPORT_FORMAT::JPEG );

    std::vector<unsigned char> data;
    bool                       ok = aScreenshotImage.Save( data, fmt_is_jpeg ? RASTER_IMAGE::TYPE::JPEG
                                                                             : RASTER_IMAGE::TYPE::PNG );

    if( ok )
    {
        wxFFile file( aFullFileName, wxT( "wb" ) );

        ok = file.IsOpened() && file.Write( data.data(), data.size() ) == data.size();
    }

    if( !ok )
        DisplayError( this, _( "Can't save file" ) );
}


void EDA_3D_VIEWER_FRAME::RenderEngineChanged()
{
    KI_LOG_TRACE( m_logTrace, wxT( "EDA_3D_VIEWER_FRAME::RenderEngineChanged()" ) );

    if( m_canvas )
        m_canvas->RenderEngineChanged();
}


void EDA_3D_VIEWER_FRAME::loadCommonSettings()
{
    KI_CHECK_RET( m_canvas, wxT( "Cannot load settings to null canvas" ) );

    COMMON_SETTINGS* settings = Pgm().GetCommonSettings();

    // TODO(JE) use all control options
    m_boardAdapter.m_MousewheelPanning = settings->m_Input.scroll_modifier_zoom != 0;
}


void EDA_3D_VIEWER_FRAME::applySettings( EDA_3D_VIEWER_SETTINGS* cfg )
{
    m_boardAdapter.m_Cfg = cfg;

    m_canvas->SetAnimationEnabled( cfg->m_Camera.animation_enabled );
    m_canvas->SetMovingSpeedMultiplier( cfg->m_Camera.moving_speed_multiplier );
    m_canvas->SetProjectionMode( cfg->m_Camera.projection_mode );

    m_canvas->SetVcSettings( EDA_DRAW_PANEL_GAL::GetVcSettings() );
}
