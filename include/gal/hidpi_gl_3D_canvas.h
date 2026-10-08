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

#ifndef HIDPI_GL_3D_CANVAS_H
#define HIDPI_GL_3D_CANVAS_H

#include <gal/gal.h>
#include <atomic>
#include <gal/3d/camera.h>
#include <gal/hidpi_gl_canvas.h>
#include <kicad_gl/gl_context_mgr.h>
#include <toolkit/mouse_state.h>
#include <toolkit/toolkit_types.h>

/**
 *  Provides basic 3D controls ( zoom, rotate, translate, ... )
 *
 *  The canvas gives its events to the virtual functions on*(), with the state of the mouse
 *  (MOUSE_STATE): the 3D canvases (EDA_3D_CANVAS) don't depend on the GUI toolkit.  The Qt build
 *  has its own HIDPI_GL_3D_CANVAS, with the same interface.
 */
class GAL_API HIDPI_GL_3D_CANVAS : public HIDPI_GL_CANVAS
{
public:
    /**
     * @param aGLAttribs are the attributes of the OpenGL surface (OGL_ATT_LIST).
     */
    HIDPI_GL_3D_CANVAS( const KIGFX::VC_SETTINGS& aVcSettings, CAMERA& aCamera,
                        TOOLKIT_WINDOW* aParent, const KIGL_ATTRIBUTES& aGLAttribs );

    bool m_mouse_is_moving; // Mouse activity is in progress
    bool m_mouse_was_moved;
    bool m_camera_is_moving; // Camera animation is ongoing

    CAMERA&            m_camera;
    static const float m_delta_move_step_factor; // Step factor to used with cursor on
                                                 // relation to the current zoom

    /**
     * Get the canvas camera.
     */
    CAMERA* GetCamera() { return &m_camera; }

    void OnMouseMoveCamera( const MOUSE_STATE& aMouse );
    void OnMouseWheelCamera( const MOUSE_STATE& aMouse, bool aPan );

protected:
    /// The canvas must be drawn.
    virtual void onPaint() {}

    /// The size of the canvas changed.
    virtual void onResize() {}

    /// The canvas is closed (with its window).
    virtual void onClose() {}

    /**
     * An input event (mouse, keyboard), before the functions of the mouse events: for the
     * tools.
     *
     * @return true if the event was used (it does not go to the functions of the mouse events).
     */
    virtual bool onInputEvent( TOOLKIT_EVENT& aEvent ) { return false; }

    virtual void onMouseMove( const MOUSE_STATE& aMouse ) {}
    virtual void onMouseWheel( const MOUSE_STATE& aMouse ) {}
    virtual void onMagnify( const MOUSE_STATE& aMouse ) {}
    virtual void onLeftDown( const MOUSE_STATE& aMouse ) {}
    virtual void onLeftUp( const MOUSE_STATE& aMouse ) {}
    virtual void onMiddleDown( const MOUSE_STATE& aMouse ) {}
    virtual void onMiddleUp( const MOUSE_STATE& aMouse ) {}
    virtual void onRightDown( const MOUSE_STATE& aMouse ) {}
    virtual void onRightUp( const MOUSE_STATE& aMouse ) {}

    /**
     * Touch gestures.
     *
     * @param aStart is true for the first event of the gesture.
     * @param aPosition is the position of the gesture, in client coordinates.
     */
    virtual void onZoomGesture( bool aStart, const VECTOR2I& aPosition, double aZoomFactor ) {}
    virtual void onPanGesture( bool aStart, const VECTOR2I& aPosition ) {}
    virtual void onRotateGesture( bool aStart, const VECTOR2I& aPosition, double aAngle ) {}
};


#endif // HIDPI_GL_3D_CANVAS_H
