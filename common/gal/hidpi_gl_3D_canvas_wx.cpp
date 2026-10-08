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

/**
 * @file hidpi_gl_3D_canvas_wx.cpp
 * The wxWidgets part of HIDPI_GL_3D_CANVAS: its events, given to its virtual functions.
 */

#include <gal/hidpi_gl_3D_canvas.h>

#include <wx/event.h>


/// The state of the mouse of a wxWidgets mouse event.
static MOUSE_STATE mouseState( const wxMouseEvent& aEvent )
{
    MOUSE_STATE state;

    state.m_position = VECTOR2I( aEvent.GetX(), aEvent.GetY() );
    state.m_leftDown = aEvent.LeftIsDown();
    state.m_middleDown = aEvent.MiddleIsDown();
    state.m_rightDown = aEvent.RightIsDown();
    state.m_dragging = aEvent.Dragging();
    state.m_shift = aEvent.ShiftDown();
    state.m_control = aEvent.ControlDown();
    state.m_alt = aEvent.AltDown();
    state.m_wheelRotation = aEvent.GetWheelRotation();
    state.m_wheelHorizontal = aEvent.GetWheelAxis() == wxMOUSE_WHEEL_HORIZONTAL;
    state.m_magnification = aEvent.GetMagnification();

    return state;
}


HIDPI_GL_3D_CANVAS::HIDPI_GL_3D_CANVAS( const KIGFX::VC_SETTINGS& aVcSettings, CAMERA& aCamera,
                                        TOOLKIT_WINDOW* aParent,
                                        const KIGL_ATTRIBUTES& aGLAttribs ) :
        HIDPI_GL_CANVAS( aVcSettings, aParent, aGLAttribs, wxID_ANY, wxDefaultPosition,
                         wxDefaultSize, wxFULL_REPAINT_ON_RESIZE ),
        m_mouse_is_moving( false ),
        m_mouse_was_moved( false ),
        m_camera_is_moving( false ),
        m_camera( aCamera )
{
    Bind( wxEVT_PAINT,
          [this]( wxPaintEvent& aEvent )
          {
              onPaint();
          } );

    // Do nothing, to avoid flashing
    Bind( wxEVT_ERASE_BACKGROUND, []( wxEraseEvent& aEvent ) {} );

    Bind( wxEVT_SIZE,
          [this]( wxSizeEvent& aEvent )
          {
              onResize();
          } );

    Bind( wxEVT_CLOSE_WINDOW,
          [this]( wxCloseEvent& aEvent )
          {
              onClose();
              aEvent.Skip();
          } );

    // The mouse events: the tools first, then the functions of the canvas
    auto bindMouse =
            [this]( const wxEventTypeTag<wxMouseEvent>& aType,
                    void ( HIDPI_GL_3D_CANVAS::*aHandler )( const MOUSE_STATE& ) )
            {
                Bind( aType,
                      [this, aHandler]( wxMouseEvent& aEvent )
                      {
                          if( onInputEvent( aEvent ) )
                              return;

                          ( this->*aHandler )( mouseState( aEvent ) );
                      } );
            };

    bindMouse( wxEVT_LEFT_DOWN, &HIDPI_GL_3D_CANVAS::onLeftDown );
    bindMouse( wxEVT_LEFT_UP, &HIDPI_GL_3D_CANVAS::onLeftUp );
    bindMouse( wxEVT_MIDDLE_DOWN, &HIDPI_GL_3D_CANVAS::onMiddleDown );
    bindMouse( wxEVT_MIDDLE_UP, &HIDPI_GL_3D_CANVAS::onMiddleUp );
    bindMouse( wxEVT_RIGHT_DOWN, &HIDPI_GL_3D_CANVAS::onRightDown );
    bindMouse( wxEVT_RIGHT_UP, &HIDPI_GL_3D_CANVAS::onRightUp );
    bindMouse( wxEVT_MOUSEWHEEL, &HIDPI_GL_3D_CANVAS::onMouseWheel );
    bindMouse( wxEVT_MOTION, &HIDPI_GL_3D_CANVAS::onMouseMove );
    bindMouse( wxEVT_MAGNIFY, &HIDPI_GL_3D_CANVAS::onMagnify );

    // The other input events go to the tools only.  Binding both EVT_CHAR and EVT_CHAR_HOOK
    // ensures that all key events, especially special key like arrow keys, are handled by the
    // GAL event dispatcher, and not sent to GUI without filtering, because they have a default
    // action (scroll) that must not be called.
    const wxEventType inputEvents[] =
    {
        wxEVT_LEFT_DCLICK, wxEVT_RIGHT_DCLICK, wxEVT_MIDDLE_DCLICK,
        wxEVT_CHAR, wxEVT_CHAR_HOOK,
        wxEVT_MENU_OPEN, wxEVT_MENU_CLOSE, wxEVT_MENU_HIGHLIGHT
    };

    for( wxEventType eventType : inputEvents )
    {
        Bind( wxEventTypeTag<wxEvent>( eventType ),
              [this]( wxEvent& aEvent )
              {
                  if( !onInputEvent( aEvent ) )
                      aEvent.Skip();
              } );
    }

    // touch gesture events
#if defined( __WXMSW__ )
    EnableTouchEvents( wxTOUCH_ZOOM_GESTURE | wxTOUCH_ROTATE_GESTURE | wxTOUCH_PAN_GESTURES );
#elif defined( __WXGTK__ )
    EnableTouchEvents( wxTOUCH_ZOOM_GESTURE | wxTOUCH_ROTATE_GESTURE );
#endif

    Bind( wxEVT_GESTURE_ZOOM,
          [this]( wxZoomGestureEvent& aEvent )
          {
              onZoomGesture( aEvent.IsGestureStart(),
                             VECTOR2I( aEvent.GetPosition().x, aEvent.GetPosition().y ),
                             aEvent.GetZoomFactor() );
          } );

    Bind( wxEVT_GESTURE_PAN,
          [this]( wxPanGestureEvent& aEvent )
          {
              onPanGesture( aEvent.IsGestureStart(),
                            VECTOR2I( aEvent.GetPosition().x, aEvent.GetPosition().y ) );
          } );

    Bind( wxEVT_GESTURE_ROTATE,
          [this]( wxRotateGestureEvent& aEvent )
          {
              onRotateGesture( aEvent.IsGestureStart(),
                               VECTOR2I( aEvent.GetPosition().x, aEvent.GetPosition().y ),
                               aEvent.GetRotationAngle() );
          } );
}
