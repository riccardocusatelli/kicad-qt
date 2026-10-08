/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
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

/**
 * @file  eda_3d_viewer_frame.h
 * @brief Declaration of the eda_3d_viewer class
 */

#ifndef EDA_3D_VIEWER_H
#define EDA_3D_VIEWER_H

#include "3d_canvas/eda_3d_canvas.h"
#include "3d_canvas/board_adapter.h"
#include "3d_rendering/track_ball.h"
#include <kiway_player.h>
#include <pcb_base_frame.h>
#include <toolkit/gui_services.h>
#include <wx/filename.h>


#define KICAD_DEFAULT_3D_DRAWFRAME_STYLE    (wxDEFAULT_FRAME_STYLE | wxWANTS_CHARS)

// Forward declarations
class RASTER_IMAGE;

#if defined(__linux__) || defined(__FreeBSD__)
class SPNAV_VIEWER_PLUGIN;
#else
class NL_3D_VIEWER_PLUGIN;
#endif
class APPEARANCE_CONTROLS_3D;


enum class EDA_3D_VIEWER_EXPORT_FORMAT
{
    CLIPBOARD,
    IMAGE,
    PNG,
    JPEG
};

/**
 * Create and handle a window for the 3d viewer connected to a Kiway and a pcbboard
 *
 * The frame does not depend on the GUI toolkit; the parts of the window that do (the layout
 * of its panes, the file dialog, the clipboard) are in eda_3d_viewer_frame_wx.cpp and in the
 * Qt build.
 */
class EDA_3D_VIEWER_FRAME : public KIWAY_PLAYER
{
public:
    EDA_3D_VIEWER_FRAME( KIWAY* aKiway, PCB_BASE_FRAME* aParent, const wxString& aTitle,
                         long style = KICAD_DEFAULT_3D_DRAWFRAME_STYLE );

    ~EDA_3D_VIEWER_FRAME();

    PCB_BASE_FRAME* Parent() const
    {
        return static_cast<PCB_BASE_FRAME*>( TOOLKIT::GetParent( this ) );
    }

    BOARD* GetBoard() { return Parent()->GetBoard(); }

    TOOLKIT_WINDOW* GetToolCanvas() const override { return m_canvas; }

    /**
     * Request reloading the 3D view.
     *
     * However the request will be executed only when the 3D canvas is refreshed.  It allows
     * one to prepare changes and request for 3D rebuild only when all changes are committed.
     * This is made because the 3D rebuild can take a long time, and this rebuild cannot
     * always made after each change, for calculation time reason.
     */
    void ReloadRequest();

    // !TODO: review this function: it need a way to tell what changed,
    // to only reload/rebuild things that have really changed
    /**
     * Reload and refresh (rebuild) the 3D scene.
     *
     * @warning Rebuilding the 3D scene can take a bit of time, so rebuilding the scene can
     *          be immediate, or made later, during the next 3D canvas refresh (on zoom for
     *           instance).
     *
     * @param aForceImmediateRedraw true to immediately rebuild the 3D scene or false to wait
     *                              refresh later.
     */
    void NewDisplay( bool aForceImmediateRedraw = false );

    void Redraw();

    BOARD_ADAPTER& GetAdapter() { return m_boardAdapter; }
    CAMERA& GetCurrentCamera() { return m_currentCamera; }

    EDA_3D_CANVAS* GetCanvas()  { return m_canvas; }

    void SaveSettings( APP_SETTINGS_BASE* aCfg ) override;
    void LoadSettings( APP_SETTINGS_BASE* aCfg ) override;

    /**
     * Notification that common settings are updated.
     *
     * This would be private (and only called by the Kiway), but we need to do this manually
     * from the PCB frame because the 3D viewer isn't updated via the #KIWAY.
     */
    void CommonSettingsChanged( int aFlags ) override;
    void ShowChangedLanguage() override;

    APPEARANCE_CONTROLS_3D* GetAppearanceManager() { return m_appearancePanel; }

    void ToggleAppearanceManager();

    void OnDarkModeToggle();

    /**
     *  Create a Screenshot of the current 3D view.
     *  Output file format is png or jpeg, or image is copied to the clipboard
     */
    void TakeScreenshot( EDA_3D_VIEWER_EXPORT_FORMAT aFormat );

    /**
     * Export 3D viewer image to file or clipboard
     * @param aFormat - Export format (JPEG, PNG, or CLIPBOARD)
     * @param aSize - Size of the exported image
     */
    void ExportImage( EDA_3D_VIEWER_EXPORT_FORMAT aFormat, const wxSize& aSize );

protected:
    APP_SETTINGS_BASE* config() const override;

    void setupUIConditions() override;

    void handleIconizeEvent( bool aIconized ) override;

    void handleActivateEvent( bool aActive ) override;

    bool handleViewSwitcherKey( int aSwitchKey ) override;

private:
    /**
     * Create the parts of the window that depend on the GUI toolkit: the icon, the status
     * bar, the infobar and the layout of the toolbar, the appearance panel and the canvas.
     */
    void setupWindow( EDA_3D_VIEWER_SETTINGS* aCfg );

    /// Release the parts of the window that depend on the GUI toolkit (the layout).
    void releaseWindow();

    /// Reset the settings of the viewer to their default values (menu Preferences).
    void resetDefaults();

    void onDisableRayTracing();

    void doReCreateMenuBar() override;

    /**
     * @brief RenderEngineChanged - Update toolbar icon and call canvas RenderEngineChanged
     */
    void RenderEngineChanged();

    void refreshRender();

    /**
     * Load configuration from common settings.
     */
    void loadCommonSettings();

    void applySettings( EDA_3D_VIEWER_SETTINGS* aSettings );

        /**
     * Get export filename through file dialog
     * @param aFormat - [in/out] Export format to determine default file extension and wildcard.
     *                  Will be updated to the selected format.
     * @param fullFileName - [out] Full path of selected file
     * @return true if filename was successfully obtained, false if cancelled
     */
    bool getExportFileName( EDA_3D_VIEWER_EXPORT_FORMAT& aFormat, wxString& fullFileName );

    /**
     * Capture screenshot using appropriate rendering method
     * @param aSize - Size of the screenshot
     * @return the image containing the screenshot
     */
    RASTER_IMAGE captureScreenshot( const wxSize& aSize );

    /**
     * Setup rendering configuration for screenshot capture
     * @param adapter - Board adapter to configure
     */
    void setupRenderingConfig( BOARD_ADAPTER& adapter );

    /**
     * Capture screenshot of the current view using the configured renderer.
     * This function handles both ray tracing and OpenGL rendering methods.
     * @return the image containing the screenshot of the current view.
     */
    RASTER_IMAGE captureCurrentViewScreenshot();

    /**
     * Capture screenshot using raytracing renderer
     * @param adapter - Configured board adapter
     * @param camera - Camera settings
     * @param aSize - Size of the screenshot
     * @return the image containing the screenshot
     */
    RASTER_IMAGE captureRaytracingScreenshot( BOARD_ADAPTER& adapter, TRACK_BALL& camera,
                                              const wxSize& aSize );

    /**
     * Convert RGBA buffer (rows from the bottom to the top) to an image
     * @param rgbaBuffer - Source RGBA buffer
     * @param realSize - Size of the buffer
     * @return image with RGB data and alpha channel
     */
    RASTER_IMAGE convertRGBAToImage( uint8_t* rgbaBuffer, const wxSize& realSize );

    /**
     * Capture screenshot using OpenGL renderer
     * @param adapter - Configured board adapter
     * @param camera - Camera settings
     * @param aSize - Size of the screenshot
     * @return the image containing the screenshot
     */
    RASTER_IMAGE captureOpenGLScreenshot( BOARD_ADAPTER& adapter, TRACK_BALL& camera,
                                          const wxSize& aSize );

    /**
     * Configure canvas settings for screenshot capture
     * @param canvas - Canvas to configure
     * @param cfg - Configuration settings (can be nullptr)
     */
    void configureCanvas( std::unique_ptr<EDA_3D_CANVAS>& canvas, EDA_3D_VIEWER_SETTINGS* cfg );

    /**
     * Save image to file or copy to clipboard based on format
     * @param screenshotImage - Image to save/copy
     * @param aFormat - Export format
     * @param fullFileName - Full path for file save (ignored for clipboard)
     */
    void saveOrCopyImage( const RASTER_IMAGE& screenshotImage, EDA_3D_VIEWER_EXPORT_FORMAT aFormat,
                          const wxString& fullFileName );

    /**
     * Copy image to system clipboard
     * @param screenshotImage - Image to copy
     */
    void copyImageToClipboard( const RASTER_IMAGE& screenshotImage );

    /**
     * Save image to file
     * @param screenshotImage - Image to save
     * @param aFormat - Export format (JPEG or PNG)
     * @param fullFileName - Full path of target file
     */
    void saveImageToFile( const RASTER_IMAGE& screenshotImage, EDA_3D_VIEWER_EXPORT_FORMAT aFormat,
                          const wxString& fullFileName );

private:
    wxFileName                     m_defaultSaveScreenshotFileName;

    EDA_3D_CANVAS*                 m_canvas;
    BOARD_ADAPTER                  m_boardAdapter;
    CAMERA&                        m_currentCamera;
    TRACK_BALL                     m_trackBallCamera;

    bool                           m_disable_ray_tracing;

#if defined(__linux__) || defined(__FreeBSD__)
    std::unique_ptr<SPNAV_VIEWER_PLUGIN> m_spaceMouse;
#else
    std::unique_ptr<NL_3D_VIEWER_PLUGIN> m_spaceMouse;
#endif

    /**
     *  Trace mask used to enable or disable the trace output of this class.
     *  The debug output can be turned on by setting the WXTRACE environment variable to
     *  "KI_TRACE_EDA_3D_VIEWER".  See the wxWidgets documentation on wxLogTrace for
     *  more information.
     */
    static const wxChar *m_logTrace;

};

#endif // EDA_3D_VIEWER_H
