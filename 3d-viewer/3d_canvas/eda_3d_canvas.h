/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
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

#ifndef EDA_3D_CANVAS_H
#define EDA_3D_CANVAS_H

#include <atomic>
#include "../common_ogl/ogl_attr_list.h"  // first: the GL loader comes before GL headers
#include "board_adapter.h"
#include "3d_rendering/raytracing/accelerators/accelerator_3d.h"
#include "3d_rendering/render_3d_base.h"
#include "3d_cache/3d_cache.h"
#include <gal/hidpi_gl_3D_canvas.h>
#include <toolkit/gui_timer.h>

#include <functional>
#include <memory>


class TOOL_DISPATCHER;
class WX_INFOBAR;
class BOARD;
class RASTER_IMAGE;
class RENDER_3D_RAYTRACE_GL;
class RENDER_3D_OPENGL;


/// The fields of the status bar of the 3D viewer.
enum EDA_3D_VIEWER_STATUSBAR
{
    ACTIVITY = 0,
    HOVERED_ITEM,
    X_POS,
    Y_POS,
    ZOOM_LEVEL,
};


/**
 *  Implement the 3D canvas of the board, on the OpenGL canvas of the GUI toolkit.
 */
class EDA_3D_CANVAS : public HIDPI_GL_3D_CANVAS
{
public:
    /**
     *  Create a new 3D Canvas with an attribute list.
     *
     *  @param aParent the parent creator of this canvas.
     *  @param aGLAttribs openGL attributes created by #OGL_ATT_LIST::GetAttributesList.
     *  @param aBoard The board.
     *  @param aSettings the settings options to be used by this canvas.
     */
    EDA_3D_CANVAS( TOOLKIT_WINDOW* aParent, const KIGL_ATTRIBUTES& aGLAttribs,
                   BOARD_ADAPTER& aSettings, CAMERA& aCamera, S3D_CACHE* a3DCachePointer );

    ~EDA_3D_CANVAS() override;

    /**
     * Set a dispatcher that processes events and forwards them to tools.
     *
     * #DRAW_PANEL_GAL does not take over the ownership. Passing NULL disconnects all event
     * handlers from the DRAW_PANEL_GAL and parent frame.
     *
     * @param aEventDispatcher is the object that will be used for dispatching events.
     */
    void SetEventDispatcher( TOOL_DISPATCHER* aEventDispatcher );

    /// Show a text in a field (EDA_3D_VIEWER_STATUSBAR) of the status bar of the parent.
    using STATUS_TEXT_HANDLER = std::function<void( const wxString& aText, int aField )>;

    /**
     * Set the function showing the position, the activity and the hovered item in the status
     * bar of the parent (no status bar if none).
     */
    void SetStatusTextHandler( STATUS_TEXT_HANDLER aHandler )
    {
        m_statusTextHandler = std::move( aHandler );
    }

    /**
     * A reporter showing its messages in a field of the status bar of the parent (with the
     * status text handler), or nullptr if the parent has no status bar.
     */
    std::unique_ptr<REPORTER> MakeStatusReporter( EDA_3D_VIEWER_STATUSBAR aField ) const;

    /**
     * Set the function called when the OpenGL version does not support the ray tracing (the
     * viewer disables it).
     */
    void SetRaytracingUnsupportedHandler( std::function<void()> aHandler )
    {
        m_raytracingUnsupportedHandler = std::move( aHandler );
    }

    void SetInfoBar( WX_INFOBAR* aInfoBar )
    {
        m_parentInfoBar = aInfoBar;
    }

    void ReloadRequest( BOARD* aBoard = nullptr, S3D_CACHE* aCachePointer = nullptr );

    /**
     * Query if there is a pending reload request.
     *
     * @return true if it wants to reload, false if there is no reload pending.
     */
    bool IsReloadRequestPending() const
    {
        if( m_3d_render )
            return m_3d_render->IsReloadRequestPending();
        else
            return false;
    }

    /**
     * @return the current render ( a RENDER_3D_RAYTRACE_GL* or a RENDER_3D_OPENGL* render )
     */
    RENDER_3D_BASE* GetCurrentRender() const { return m_3d_render; }

    /**
     * Request to render the current view in Raytracing mode.
     */
    void RenderRaytracingRequest();

    /**
     *  Request a screenshot of the current OpenGL image.
     *
     *  @return the image, with alpha.
     */
    RASTER_IMAGE GetScreenshot();

    /**
     * Select a specific 3D view or operation.
     *
     * @param aRequestedView the view to move to.
     * @return true if the view request was handled, false if no command found for this view.
     */
    bool SetView3D( VIEW3D_TYPE aRequestedView );

    /**
     * Enable or disable camera animation when switching to a pre-defined view.
     */
    void SetAnimationEnabled( bool aEnable ) { m_animation_enabled = aEnable; }
    bool GetAnimationEnabled() const { return m_animation_enabled; }

    /**
     * Set the camera animation moving speed multiplier option.
     *
     * @param aMultiplier one of the possible integer options: [1,2,3,4,5].
     */
    void SetMovingSpeedMultiplier( int aMultiplier ) { m_moving_speed_multiplier = aMultiplier; }
    int GetMovingSpeedMultiplier() const { return m_moving_speed_multiplier; }

    int GetProjectionMode() const { return (int) m_camera.GetProjection(); };
    void SetProjectionMode( int aMode ) { m_camera.SetProjection( (PROJECTION_TYPE) aMode ); }

    /**
     * Notify that the render engine was changed.
     */
    void RenderEngineChanged();

    /**
     * Update the status bar with the position information.
     */
    void DisplayStatus();

    /**
     * Schedule a refresh update of the canvas.
     *
     * @param aRedrawImmediately true will request a redraw, false will schedule a redraw
     *                           after a short timeout.
     */
    void Request_refresh( bool aRedrawImmediately = true );

    /**
     * Used to forward events to the canvas from popups, etc.
     */
    void OnEvent( TOOLKIT_EVENT& aEvent );

    /**
     * Get information used to display 3D board.
     */
    const BOARD_ADAPTER& GetBoardAdapter() const { return m_boardAdapter; }

    /**
     * Get a value indicating whether to render the pivot.
     */
    bool GetRenderPivot() { return m_render_pivot; }

    /**
     * Set aValue indicating whether to render the pivot.
     *
     * @param aValue true will cause the pivot to be rendered on the next redraw.
     */
    void SetRenderPivot( bool aValue ) { m_render_pivot = aValue; }

    /**
     * Get a value indicating whether to render the 3dmouse pivot.
     */
    bool GetRender3dmousePivot()
    {
        return m_render3dmousePivot;
    }


    /**
     * Set aValue indicating whether to render the 3dmouse pivot.
     *
     * @param aValue true will cause the pivot to be rendered on the next redraw.
     */
    void SetRender3dmousePivot( bool aValue )
    {
        m_render3dmousePivot = aValue;
    }


    /**
     *  Set the position of the the 3dmouse pivot.
     *
     *  @param aPos is the position of the 3dmouse rotation pivot
     */
    void Set3dmousePivotPos( const SFVEC3F& aPos )
    {
        m_3dmousePivotPos = aPos;
    }

    /**
     * The actual function to repaint the canvas.
     *
     * It is usually called by onPaint() but it can be called outside a paint event.
     */
    void DoRePaint();

    void RenderToFrameBuffer( unsigned char* aBuffer, int aWidth, int aHeight );

    /**
     * Free the renderers and the OpenGL context (before the canvas is destroyed with its
     * parent).
     */
    void ReleaseOpenGL();

protected:
    void onPaint() override;
    void onResize() override;
    void onClose() override;
    bool onInputEvent( TOOLKIT_EVENT& aEvent ) override;

    void onMouseWheel( const MOUSE_STATE& aMouse ) override;
    void onMagnify( const MOUSE_STATE& aMouse ) override;
    void onMouseMove( const MOUSE_STATE& aMouse ) override;
    void onLeftDown( const MOUSE_STATE& aMouse ) override;
    void onLeftUp( const MOUSE_STATE& aMouse ) override;
    void onMiddleUp( const MOUSE_STATE& aMouse ) override;
    void onMiddleDown( const MOUSE_STATE& aMouse ) override;
    void onRightUp( const MOUSE_STATE& aMouse ) override;
    void onRightDown( const MOUSE_STATE& aMouse ) override;

    void onZoomGesture( bool aStart, const VECTOR2I& aPosition, double aZoomFactor ) override;
    void onPanGesture( bool aStart, const VECTOR2I& aPosition ) override;
    void onRotateGesture( bool aStart, const VECTOR2I& aPosition, double aAngle ) override;

private:
    void onTimerTimeout_Editing();
    void onTimerTimeout_Redraw();

    /// Show a text in a field of the status bar of the parent.
    void setStatusText( const wxString& aText, int aField );

    /**
     *Stop the editing time so it will not timeout.
     */
    void stop_editingTimeOut_Timer();

    /**
     * Reset the editing timer.
     */
    void restart_editingTimeOut_Timer();

    /**
     * Start a camera movement.
     *
     * @param aMovingSpeed the time speed.
     * @param aRenderPivot if it should display pivot cursor while move.
     */
    void request_start_moving_camera( float aMovingSpeed = 2.0f, bool aRenderPivot = true );

    /**
     * This function hits a ray to the board and start a movement.
     */
    void move_pivot_based_on_cur_mouse_position();

    /**
     * Render the pivot cursor.
     *
     * @param t time between 0.0 and 1.0.
     * @param aScale scale to apply on the cursor.
     */
    void render_pivot( float t, float aScale );

    /**
     * Render the 3dmouse pivot cursor.
     *
     * @param aScale scale to apply on the cursor.
     */
    void render3dmousePivot( float aScale );

    /**
     * @return true if OpenGL initialization succeeded.
     */
    bool initializeOpenGL();

    RAY getRayAtCurrentMousePosition();

private:
    TOOL_DISPATCHER*       m_eventDispatcher = nullptr;
    STATUS_TEXT_HANDLER    m_statusTextHandler;                 // Parent statusbar to report progress
    std::function<void()>  m_raytracingUnsupportedHandler;
    WX_INFOBAR*            m_parentInfoBar = nullptr;

    KIGL_CONTEXT*          m_glRC = nullptr;                    // Current OpenGL context
    bool                   m_is_opengl_initialized = false;
    bool                   m_is_opengl_version_supported = true;

    GUI_TIMER              m_editing_timeout_timer;   // Expires after some time signaling that
                                                      // the mouse / keyboard movements are over
    GUI_TIMER              m_redraw_trigger_timer;    // Used to schedule a redraw event
    GUI_TIMER              m_refresh_request_timer;   // Redraws from the event loop
    std::atomic_flag       m_is_currently_painting = ATOMIC_FLAG_INIT;   // Avoid drawing twice at the same time

    bool                   m_render_pivot = false;            // Render the pivot while camera moving
    float                  m_camera_moving_speed = 1.0f;     // 1.0f will be 1:1
    int64_t                m_strtime_camera_movement = 0; // Ticktime of camera movement start
    bool                   m_animation_enabled = true;       // Camera animation enabled
    int                    m_moving_speed_multiplier = 3; // Camera animation speed multiplier option

    BOARD_ADAPTER&         m_boardAdapter;            // Pre-computed 3D info and settings
    RENDER_3D_BASE*        m_3d_render = nullptr;
    RENDER_3D_RAYTRACE_GL* m_3d_render_raytracing;
    RENDER_3D_OPENGL*      m_3d_render_opengl;

    bool                   m_opengl_supports_raytracing = true;
    bool                   m_render_raytracing_was_requested = false;

    ACCELERATOR_3D*        m_accelerator3DShapes = nullptr;    // used for mouse over searching

    BOARD_ITEM*            m_currentRollOverItem = nullptr;

    bool    m_render3dmousePivot = false; // Render the 3dmouse pivot
    SFVEC3F m_3dmousePivotPos;            // The position of the 3dmouse pivot

    /// Used to track gesture events.
    double   m_gestureLastZoomFactor = 1.0;
    double   m_gestureLastAngle = 0.0;

    /**
     *  Trace mask used to enable or disable the trace output of this class.
     *  The debug output can be turned on by setting the WXTRACE environment variable to
     *  "KI_TRACE_EDA_3D_CANVAS".  See the wxWidgets documentation on wxLogTrace for
     *  more information.
     */
    static const wxChar* m_logTrace;
};


#endif // EDA_3D_CANVAS_H
