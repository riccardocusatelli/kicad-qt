/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * @file hidpi_gl_3D_canvas_qt.cpp
 * The Qt part of HIDPI_GL_3D_CANVAS: its events, given to its virtual functions (see
 * common/gal/hidpi_gl_3D_canvas_wx.cpp for the wxWidgets one).
 */

#include <gal/hidpi_gl_3D_canvas.h>

#include <cmath>
#include <functional>
#include <memory>

#include <QMouseEvent>
#include <QNativeGestureEvent>
#include <QWheelEvent>

#include <math/util.h>
#include <ui_hooks.h>


namespace
{

/// The state of the mouse and of the modifier keys of a Qt input event.
MOUSE_STATE mouseState( const QSinglePointEvent* aEvent )
{
    MOUSE_STATE state;

    QPointF position = aEvent->position();
    state.m_position = VECTOR2I( KiROUND( position.x() ), KiROUND( position.y() ) );

    Qt::MouseButtons buttons = aEvent->buttons();
    state.m_leftDown = buttons & Qt::LeftButton;
    state.m_middleDown = buttons & Qt::MiddleButton;
    state.m_rightDown = buttons & Qt::RightButton;
    state.m_dragging = aEvent->type() == QEvent::MouseMove && buttons != Qt::NoButton;

    Qt::KeyboardModifiers modifiers = aEvent->modifiers();
    state.m_shift = modifiers & Qt::ShiftModifier;
    state.m_control = modifiers & Qt::ControlModifier;
    state.m_alt = modifiers & Qt::AltModifier;

    return state;
}

} // namespace


HIDPI_GL_3D_CANVAS::HIDPI_GL_3D_CANVAS( const KIGFX::VC_SETTINGS& aVcSettings, CAMERA& aCamera,
                                        TOOLKIT_WINDOW* aParent,
                                        const KIGL_ATTRIBUTES& aGLAttribs ) :
        HIDPI_GL_CANVAS( aVcSettings, aParent, aGLAttribs ),
        m_mouse_is_moving( false ),
        m_mouse_was_moved( false ),
        m_camera_is_moving( false ),
        m_camera( aCamera )
{
    // The canvas gets the input events of its OpenGL window, and draws itself
    SetMouseListener( this );
    SetPaintListener( this );

    setFocusPolicy( Qt::StrongFocus );
    setMouseTracking( true );

    // The angle of the rotation gesture, as wxWidgets gives it (the total, in radians)
    auto rotation = std::make_shared<double>( 0.0 );

    auto handler =
            [this, rotation]( QEvent* aEvent ) -> bool
            {
                // The input events go to the tools first, then to the functions of the canvas
                auto toTools =
                        [&]() -> bool
                        {
                            if( onInputEvent( *aEvent ) )
                            {
                                aEvent->accept();
                                return true;
                            }

                            return false;
                        };

                switch( aEvent->type() )
                {
                case QEvent::Paint:
                    onPaint();
                    return true;

                case QEvent::Resize:
                    onResize();
                    return false;

                case QEvent::Close:
                    onClose();
                    return false;

                case QEvent::MouseButtonPress:
                case QEvent::MouseButtonRelease:
                {
                    if( toTools() )
                        return true;

                    QMouseEvent* event = static_cast<QMouseEvent*>( aEvent );
                    MOUSE_STATE  state = mouseState( event );
                    bool         press = aEvent->type() == QEvent::MouseButtonPress;

                    switch( event->button() )
                    {
                    case Qt::LeftButton:   press ? onLeftDown( state ) : onLeftUp( state );     break;
                    case Qt::MiddleButton: press ? onMiddleDown( state ) : onMiddleUp( state ); break;
                    case Qt::RightButton:  press ? onRightDown( state ) : onRightUp( state );   break;
                    default:                                                                    break;
                    }

                    return true;
                }

                case QEvent::MouseMove:
                    if( !toTools() )
                        onMouseMove( mouseState( static_cast<QMouseEvent*>( aEvent ) ) );

                    return true;

                case QEvent::Wheel:
                {
                    if( toTools() )
                        return true;

                    QWheelEvent* event = static_cast<QWheelEvent*>( aEvent );
                    MOUSE_STATE  state = mouseState( event );
                    QPoint       delta = event->angleDelta();

                    state.m_wheelHorizontal = delta.y() == 0 && delta.x() != 0;
                    state.m_wheelRotation = state.m_wheelHorizontal ? delta.x() : delta.y();

                    if( state.m_wheelRotation != 0 )
                        onMouseWheel( state );

                    return true;
                }

                case QEvent::NativeGesture:
                {
                    if( toTools() )
                        return true;

                    QNativeGestureEvent* event = static_cast<QNativeGestureEvent*>( aEvent );
                    MOUSE_STATE          state = mouseState( event );

                    switch( event->gestureType() )
                    {
                    case Qt::BeginNativeGesture:
                        *rotation = 0.0;
                        onRotateGesture( true, state.m_position, 0.0 );
                        break;

                    case Qt::ZoomNativeGesture:
                        state.m_magnification = static_cast<float>( event->value() );
                        onMagnify( state );
                        break;

                    case Qt::RotateNativeGesture:
                        // Qt gives the change of the angle, in degrees
                        *rotation += event->value() * M_PI / 180.0;
                        onRotateGesture( false, state.m_position, *rotation );
                        break;

                    default:
                        break;
                    }

                    return true;
                }

                case QEvent::MouseButtonDblClick:
                case QEvent::KeyPress:
                    // The other input events go to the tools only
                    return toTools();

                default:
                    return false;
                }
            };

    KIQT::FilterEvents( this, std::move( handler ) );
}
