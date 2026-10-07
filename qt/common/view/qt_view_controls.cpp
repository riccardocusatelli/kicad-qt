/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2013 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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
 * @file qt_view_controls.cpp
 * QT_VIEW_CONTROLS: the port of WX_VIEW_CONTROLS (common/view/wx_view_controls.cpp) to Qt.
 *
 * The positions on the canvas are in device pixels, as the screen of the GAL.
 */

#include <view/qt_view_controls.h>

#include <cmath>

#include <QCursor>
#include <QMouseEvent>
#include <QNativeGestureEvent>
#include <QWheelEvent>

#include <pgm_base.h>
#include <core/profile.h>
#include <view/view.h>
#include <view/zoom_controller.h>
#include <gal/graphics_abstraction_layer.h>
#include <trace_helpers.h>
#include <settings/common_settings.h>
#include <math/util.h> // for KiROUND
#include <geometry/geometry_utils.h>
#include <class_draw_panel_gal.h>
#include <eda_draw_frame.h>
#include <kiway.h>
#include <kiway_player.h>
#include <kiplatform/ui.h>
#include <toolkit/gui_services.h>
#include <tool/tool_event.h>          // MD_ modifiers
#include <core/ki_log.h>
#include <core/ki_assert.h>

using namespace KIGFX;


static std::unique_ptr<ZOOM_CONTROLLER> GetZoomControllerForPlatform( bool aAcceleration )
{
#ifdef __APPLE__
    // On Apple pointer devices, wheel events occur frequently and with
    // smaller rotation values.  For those devices, let's handle zoom
    // based on the rotation amount rather than the time difference.
    return std::make_unique<CONSTANT_ZOOM_CONTROLLER>( CONSTANT_ZOOM_CONTROLLER::MAC_SCALE );
#else
    if( aAcceleration )
        return std::make_unique<ACCELERATING_ZOOM_CONTROLLER>();
    else
        return std::make_unique<CONSTANT_ZOOM_CONTROLLER>( CONSTANT_ZOOM_CONTROLLER::MSW_SCALE );
#endif
}


QT_VIEW_CONTROLS::QT_VIEW_CONTROLS( VIEW* aView, EDA_DRAW_PANEL_GAL* aParentPanel ) :
        VIEW_CONTROLS( aView ),
        m_state( IDLE ),
        m_parentPanel( aParentPanel ),
        m_dragStartPoint( 0, 0 ),
        m_panDirection( 0, 0 ),
        m_panTimer( [this]() { onTimer(); } ),
        m_zoomStartPoint( 0, 0 ),
        m_cursorPos( 0, 0 ),
        m_updateCursor( true ),
        m_metaPanning( false ),
        m_metaPanStart( 0, 0 ),
        m_infinitePanWorks( false ),
        m_justWarped( false )
{
    LoadSettings();

    m_MotionEventCounter = std::make_unique<PROF_COUNTER>( "Mouse motion events" );

    m_cursorWarped = false;

    m_settings.m_lastKeyboardCursorPositionValid = false;
    m_settings.m_lastKeyboardCursorPosition = { 0.0, 0.0 };
    m_settings.m_lastKeyboardCursorCommand = 0;
}


QT_VIEW_CONTROLS::~QT_VIEW_CONTROLS()
{
}


void QT_VIEW_CONTROLS::LoadSettings()
{
    COMMON_SETTINGS* cfg = Pgm().GetCommonSettings();

    m_settings.m_warpCursor            = cfg->m_Input.center_on_zoom;
    m_settings.m_focusFollowSchPcb     = cfg->m_Input.focus_follow_sch_pcb;
    m_settings.m_autoPanSettingEnabled = cfg->m_Input.auto_pan;
    m_settings.m_autoPanAcceleration   = cfg->m_Input.auto_pan_acceleration;
    m_settings.m_horizontalPan         = cfg->m_Input.horizontal_pan;
    m_settings.m_zoomAcceleration      = cfg->m_Input.zoom_acceleration;
    m_settings.m_zoomSpeed             = cfg->m_Input.zoom_speed;
    m_settings.m_zoomSpeedAuto         = cfg->m_Input.zoom_speed_auto;
    m_settings.m_scrollModifierZoom    = cfg->m_Input.scroll_modifier_zoom;
    m_settings.m_scrollModifierPanH    = cfg->m_Input.scroll_modifier_pan_h;
    m_settings.m_scrollModifierPanV    = cfg->m_Input.scroll_modifier_pan_v;
    m_settings.m_dragLeft              = cfg->m_Input.drag_left;
    m_settings.m_dragMiddle            = cfg->m_Input.drag_middle;
    m_settings.m_dragRight             = cfg->m_Input.drag_right;
    m_settings.m_scrollReverseZoom     = cfg->m_Input.reverse_scroll_zoom;
    m_settings.m_scrollReversePanH     = cfg->m_Input.reverse_scroll_pan_h;
    m_settings.m_motionPanModifier     = cfg->m_Input.motion_pan_modifier;

    m_zoomController.reset();

    if( cfg->m_Input.zoom_speed_auto )
    {
        m_zoomController = GetZoomControllerForPlatform( cfg->m_Input.zoom_acceleration );
    }
    else
    {
        if( cfg->m_Input.zoom_acceleration )
        {
            m_zoomController =
                    std::make_unique<ACCELERATING_ZOOM_CONTROLLER>( cfg->m_Input.zoom_speed );
        }
        else
        {
            double scale = CONSTANT_ZOOM_CONTROLLER::MANUAL_SCALE_FACTOR * cfg->m_Input.zoom_speed;

            m_zoomController = std::make_unique<CONSTANT_ZOOM_CONTROLLER>( scale );
        }
    }
}


bool QT_VIEW_CONTROLS::onMotion( QMouseEvent* aEvent )
{
    ( *m_MotionEventCounter )++;

    VECTOR2I mouseRel = getMouseScreenPosition();

    bool     isAutoPanning = false;
    int      x = mouseRel.x;
    int      y = mouseRel.y;
    VECTOR2D mousePos( x, y );

    // Clear keyboard cursor position flag when actual mouse motion is detected
    // (i.e., not from cursor warping and position has changed)
    if( !m_cursorWarped && m_settings.m_lastKeyboardCursorPositionValid )
    {
        VECTOR2I screenPos( x, y );
        VECTOR2I keyboardScreenPos = m_view->ToScreen( m_settings.m_lastKeyboardCursorPosition );

        // If mouse has moved to a different position than the keyboard cursor position,
        // clear the keyboard position flag to allow mouse control
        if( screenPos != keyboardScreenPos )
        {
            m_settings.m_lastKeyboardCursorPositionValid = false;
            m_settings.m_lastKeyboardCursorPosition = { 0.0, 0.0 };
        }
    }

    // Automatic focus switching between SCH and PCB windows on canvas mouse motion
    if( m_settings.m_focusFollowSchPcb )
    {
        if( EDA_DRAW_FRAME* frame = m_parentPanel->GetParentEDAFrame() )
        {
            KIWAY_PLAYER* otherFrame = nullptr;

            if( frame->IsType( FRAME_PCB_EDITOR ) )
                otherFrame = frame->Kiway().Player( FRAME_SCH, false );
            else if( frame->IsType( FRAME_SCH ) )
                otherFrame = frame->Kiway().Player( FRAME_PCB_EDITOR, false );

            if( otherFrame && KIPLATFORM::UI::IsWindowActive( otherFrame )
                && !KIPLATFORM::UI::IsWindowActive( frame ) )
            {
                frame->raise();
            }
        }
    }

    if( m_settings.m_motionPanModifier != WXK_NONE
        && TOOLKIT::IsKeyDown( m_settings.m_motionPanModifier ) )
    {
        if( !m_metaPanning )
        {
            m_metaPanning = true;
            m_metaPanStart = mousePos;
        }
        else
        {
            VECTOR2D d = m_metaPanStart - mousePos;
            m_metaPanStart = mousePos;
            VECTOR2D delta = m_view->ToWorld( d, false );
            m_view->SetCenter( m_view->GetCenter() + delta );
        }

        if( m_updateCursor )
            m_cursorPos = GetClampedCoords( m_view->ToWorld( mousePos ) );
        else
            m_updateCursor = true;

        return true;
    }
    else
    {
        m_metaPanning = false;
    }

    if( m_state != DRAG_PANNING && m_state != DRAG_ZOOMING )
        handleCursorCapture( x, y );

    if( m_settings.m_autoPanEnabled && m_settings.m_autoPanSettingEnabled )
        isAutoPanning = handleAutoPanning( VECTOR2I( x, y ) );

    bool dragging = aEvent->buttons() & ( Qt::LeftButton | Qt::MiddleButton | Qt::RightButton );

    if( !isAutoPanning && dragging )
    {
        const VECTOR2I parentSize = m_parentPanel->GetClientSizeDevice();

        if( m_state == DRAG_PANNING )
        {
            int warpX = 0;
            int warpY = 0;

            if( x < 0 )
                warpX = parentSize.x;
            else if( x >= parentSize.x )
                warpX = -parentSize.x;

            if( y < 0 )
                warpY = parentSize.y;
            else if( y >= parentSize.y )
                warpY = -parentSize.y;

            if( !m_justWarped )
            {
                VECTOR2D d = m_dragStartPoint - mousePos;
                m_dragStartPoint = mousePos;
                VECTOR2D delta = m_view->ToWorld( d, false );
                m_view->SetCenter( m_view->GetCenter() + delta );
            }

            if( warpX || warpY )
            {
                if( !m_justWarped )
                {
                    if( m_infinitePanWorks
                        && m_parentPanel->WarpPointer( VECTOR2D( x + warpX, y + warpY ) ) )
                    {
                        m_dragStartPoint += VECTOR2D( warpX, warpY );
                        m_justWarped = true;
                    }
                }
                else
                {
                    m_justWarped = false;
                }
            }
            else
            {
                m_justWarped = false;
            }
        }
        else if( m_state == DRAG_ZOOMING )
        {
            int warpY = 0;

            if( y < 0 )
                warpY = parentSize.y;
            else if( y >= parentSize.y )
                warpY = -parentSize.y;

            if( !m_justWarped )
            {
                VECTOR2D d = m_dragStartPoint - mousePos;
                m_dragStartPoint = mousePos;

                double scale = exp( d.y * m_settings.m_zoomSpeed * 0.001 );

                KI_LOG_TRACE( traceZoomScroll, wxString::Format( "dy: %f  scale: %f", d.y, scale ) );

                m_view->SetScale( m_view->GetScale() * scale, m_view->ToWorld( m_zoomStartPoint ) );
            }

            if( warpY )
            {
                if( !m_justWarped )
                {
                    m_parentPanel->WarpPointer( VECTOR2D( x, y + warpY ) );
                    m_dragStartPoint += VECTOR2D( 0, warpY );
                    m_justWarped = true;
                }
                else
                {
                    m_justWarped = false;
                }
            }
            else
            {
                m_justWarped = false;
            }
        }
    }

    if( m_updateCursor )        // do not update the cursor position if it was explicitly set
        m_cursorPos = GetClampedCoords( m_view->ToWorld( mousePos ) );
    else
        m_updateCursor = true;

    return true;
}


bool QT_VIEW_CONTROLS::onWheel( QWheelEvent* aEvent )
{
    const double wheelPanSpeed = 0.001;

    // Qt gives the rotation in eighths of a degree: 120 for a notch, as wxWidgets
    const int verticalRotation = aEvent->angleDelta().y();
    const int horizontalRotation = aEvent->angleDelta().x();

    // Native horizontal wheel events (tilt wheels, side-button scroll combos, touchpads) pan the
    // view when "Pan left/right with horizontal movement" is enabled, otherwise they are ignored
    if( verticalRotation == 0 )
    {
        if( horizontalRotation == 0 || !m_settings.m_horizontalPan )
            return false;

        VECTOR2D scrollVec = m_view->ToWorld( m_view->GetScreenPixelSize(), false )
                             * ( (double) horizontalRotation * wheelPanSpeed );

        m_view->SetCenter( m_view->GetCenter() + VECTOR2D( -scrollVec.x, 0.0 ) );
        refreshMouse( true );
        return false;
    }

    // Pick the modifier, if any.  Shift beats control beats alt, we don't support more than one.
    Qt::KeyboardModifiers mods = aEvent->modifiers();
    int nMods = 0;
    int modifiers = 0;

    if( mods & Qt::ShiftModifier )
    {
        nMods += 1;
        modifiers = WXK_SHIFT;
    }

    if( mods & Qt::ControlModifier )
    {
        nMods += 1;
        modifiers = modifiers == 0 ? WXK_CONTROL : modifiers;
    }

    if( mods & Qt::AltModifier )
    {
        nMods += 1;
        modifiers = modifiers == 0 ? WXK_ALT : modifiers;
    }

    // Zero or one modifier is view control
    if( nMods <= 1 )
    {
        // Some platforms give the vertical rotation of the wheel with Alt as a horizontal one
        int rotation = verticalRotation;

        if( modifiers == m_settings.m_scrollModifierZoom )
        {
            rotation *= m_settings.m_scrollReverseZoom ? -1 : 1;
            const double zoomScale = m_zoomController->GetScaleForRotation( rotation );

            KI_TRACE( traceGalProfile, "Zoom: %.5f\n",  m_view->GetScale() * zoomScale );

            if( IsCursorWarpingEnabled() )
            {
                CenterOnCursor();
                m_view->SetScale( m_view->GetScale() * zoomScale );
            }
            else
            {
                const VECTOR2D anchor = m_view->ToWorld( m_parentPanel->ToDevice( aEvent->position() ) );
                m_view->SetScale( m_view->GetScale() * zoomScale, anchor );
            }

            // Refresh the zoom level and mouse position on message panel
            // (mouse position has not changed, only the zoom level has changed):
            refreshMouse( true );

            // The tools see the motion of the cursor in the world
            return true;
        }
        else
        {
            // Scrolling
            VECTOR2D scrollVec = m_view->ToWorld( m_view->GetScreenPixelSize(), false )
                                 * ( (double) rotation * wheelPanSpeed );
            double scrollX = 0.0;
            double scrollY = 0.0;
            bool   hReverse = m_settings.m_scrollReversePanH;

            if( modifiers == m_settings.m_scrollModifierPanH )
                scrollX = hReverse ? scrollVec.x : -scrollVec.x;
            else
                scrollY = -scrollVec.y;

            VECTOR2D delta( scrollX, scrollY );

            m_view->SetCenter( m_view->GetCenter() + delta );
            refreshMouse( true );
        }

        return false;
    }

    // When we have multiple mods, forward it for tool handling
    return true;
}


bool QT_VIEW_CONTROLS::onGesture( QNativeGestureEvent* aEvent )
{
    VECTOR2D pos = m_parentPanel->ToDevice( aEvent->position() );

    switch( aEvent->gestureType() )
    {
    case Qt::ZoomNativeGesture:
        // Scale based on the magnification of the gesture
        m_view->SetScale( m_view->GetScale() * ( aEvent->value() + 1.0 ), m_view->ToWorld( pos ) );
        refreshMouse( true );
        return true;

    case Qt::PanNativeGesture:
    {
        VECTOR2D screenDelta( aEvent->delta().x(), aEvent->delta().y() );
        m_view->SetCenter( m_view->GetCenter() - m_view->ToWorld( screenDelta, false ) );
        refreshMouse( true );
        return true;
    }

    default:
        return false;
    }
}


void QT_VIEW_CONTROLS::setState( STATE aNewState )
{
    m_state = aNewState;
}


bool QT_VIEW_CONTROLS::onButton( QMouseEvent* aEvent )
{
    const bool pressed = aEvent->type() == QEvent::MouseButtonPress;
    const bool released = aEvent->type() == QEvent::MouseButtonRelease;
    const Qt::MouseButton button = aEvent->button();

    switch( m_state )
    {
    case IDLE:
    case AUTO_PANNING:
        if( pressed
            && ( ( button == Qt::MiddleButton && m_settings.m_dragMiddle == MOUSE_DRAG_ACTION::PAN )
                 || ( button == Qt::RightButton && m_settings.m_dragRight == MOUSE_DRAG_ACTION::PAN ) ) )
        {
            m_dragStartPoint = m_parentPanel->ToDevice( aEvent->position() );
            setState( DRAG_PANNING );
            m_infinitePanWorks = KIPLATFORM::UI::InfiniteDragPrepareWindow( m_parentPanel );
        }
        else if( pressed
                 && ( ( button == Qt::MiddleButton && m_settings.m_dragMiddle == MOUSE_DRAG_ACTION::ZOOM )
                      || ( button == Qt::RightButton && m_settings.m_dragRight == MOUSE_DRAG_ACTION::ZOOM ) ) )
        {
            m_dragStartPoint = m_parentPanel->ToDevice( aEvent->position() );
            m_zoomStartPoint = m_dragStartPoint;
            setState( DRAG_ZOOMING );
        }

        if( released && button == Qt::LeftButton )
            setState( IDLE ); // Stop autopanning when user release left mouse button

        break;

    case DRAG_ZOOMING:
    case DRAG_PANNING:
        if( released && ( button == Qt::MiddleButton || button == Qt::LeftButton
                          || button == Qt::RightButton ) )
        {
            setState( IDLE );
            KIPLATFORM::UI::InfiniteDragReleaseWindow();
        }

        break;
    }

    return true;
}


void QT_VIEW_CONTROLS::onEnter()
{
}


void QT_VIEW_CONTROLS::onLeave()
{
    // Autopanning goes on while the cursor is outside the canvas (Qt grabs the mouse while a
    // button is pressed); without a button, the motion stops it
}


void QT_VIEW_CONTROLS::onTimer()
{
    switch( m_state )
    {
    case AUTO_PANNING:
    {
        if( !m_settings.m_autoPanEnabled )
        {
            setState( IDLE );
            return;
        }

        if( !m_parentPanel->hasFocus() && !m_parentPanel->StatusPopupHasFocus() )
        {
            setState( IDLE );
            return;
        }

        double borderSize = std::min( m_settings.m_autoPanMargin * m_view->GetScreenPixelSize().x,
                                      m_settings.m_autoPanMargin * m_view->GetScreenPixelSize().y );

        // When the mouse cursor is outside the area with no pan,
        // m_panDirection is the dist to this area limit ( in pixels )
        // It will be used also as pan value (the pan speed depends on this dist).
        VECTOR2D dir( m_panDirection );

        // When the mouse cursor is outside the area with no pan, the pan value
        // is accelerated depending on the dist between the area and the cursor
        float accel = 0.5f + ( m_settings.m_autoPanAcceleration / 5.0f );

        // For a small mouse cursor dist to area, just use the distance.
        // But for a dist > borderSize / 2, use an accelerated pan value

        if( dir.EuclideanNorm() >= borderSize ) // far from area limits
            dir = dir.Resize( borderSize * accel );
        else if( dir.EuclideanNorm() > borderSize / 2 ) // Near from area limits
            dir = dir.Resize( borderSize );

        dir = m_view->ToWorld( dir, false );
        m_view->SetCenter( m_view->GetCenter() + dir );

        refreshMouse( true );

        m_panTimer.StartOnce( (int) ( 250.0 / 60.0 ) );
    }
    break;

    case IDLE: // Just remove unnecessary warnings
    case DRAG_PANNING:
    case DRAG_ZOOMING: break;
    }
}


void QT_VIEW_CONTROLS::CaptureCursor( bool aEnabled )
{
    // Qt grabs the mouse for the widget while a button is pressed: the cursor is kept in the
    // canvas by handleCursorCapture()
    VIEW_CONTROLS::CaptureCursor( aEnabled );
}


void QT_VIEW_CONTROLS::CancelDrag()
{
    if( m_state == DRAG_PANNING || m_state == DRAG_ZOOMING )
        setState( IDLE );

    m_metaPanning = false;
}


VECTOR2D QT_VIEW_CONTROLS::GetMousePosition( bool aWorldCoordinates ) const
{
    VECTOR2D screenPos( getMouseScreenPosition() );

    return aWorldCoordinates ? GetClampedCoords( m_view->ToWorld( screenPos ) ) : screenPos;
}


VECTOR2D QT_VIEW_CONTROLS::GetRawCursorPosition( bool aEnableSnapping ) const
{
    GAL* gal = m_view->GetGAL();

    if( aEnableSnapping && gal->GetGridSnapping() )
        return gal->GetGridPoint( m_cursorPos );
    else
        return m_cursorPos;
}


VECTOR2D QT_VIEW_CONTROLS::GetCursorPosition( bool aEnableSnapping ) const
{
    if( m_settings.m_forceCursorPosition )
        return m_settings.m_forcedPosition;
    else
        return GetClampedCoords( GetRawCursorPosition( aEnableSnapping ) );
}


void QT_VIEW_CONTROLS::SetCursorPosition( const VECTOR2D& aPosition, bool aWarpView,
                                          bool aTriggeredByArrows, long aArrowCommand )
{
    m_updateCursor = false;

    VECTOR2D clampedPosition = GetClampedCoords( aPosition );

    if( aTriggeredByArrows )
    {
        m_settings.m_lastKeyboardCursorPositionValid = true;
        m_settings.m_lastKeyboardCursorPosition = clampedPosition;
        m_settings.m_lastKeyboardCursorCommand = aArrowCommand;
        m_cursorWarped = false;
    }
    else
    {
        m_settings.m_lastKeyboardCursorPositionValid = false;
        m_settings.m_lastKeyboardCursorPosition = { 0.0, 0.0 };
        m_settings.m_lastKeyboardCursorCommand = 0;
        m_cursorWarped = true;
    }

    WarpMouseCursor( clampedPosition, true, aWarpView );
    m_cursorPos = clampedPosition;
}


void QT_VIEW_CONTROLS::SetCrossHairCursorPosition( const VECTOR2D& aPosition, bool aWarpView = true )
{
    m_updateCursor = false;

    VECTOR2D clampedPosition = GetClampedCoords( aPosition );

    const VECTOR2I& screenSize = m_view->GetGAL()->GetScreenPixelSize();
    BOX2I           screen( VECTOR2I( 0, 0 ), screenSize );
    VECTOR2D        screenPos = m_view->ToScreen( clampedPosition );

    if( aWarpView && !screen.Contains( screenPos ) )
        m_view->SetCenter( clampedPosition );

    m_cursorPos = clampedPosition;
}


void QT_VIEW_CONTROLS::WarpMouseCursor( const VECTOR2D& aPosition, bool aWorldCoordinates,
                                        bool aWarpView )
{
    if( aWorldCoordinates )
    {
        const VECTOR2I& screenSize = m_view->GetGAL()->GetScreenPixelSize();
        BOX2I           screen( VECTOR2I( 0, 0 ), screenSize );
        VECTOR2D        clampedPosition = GetClampedCoords( aPosition );
        VECTOR2D        screenPos = m_view->ToScreen( clampedPosition );

        if( !screen.Contains( screenPos ) )
        {
            if( aWarpView )
            {
                m_view->SetCenter( clampedPosition );
                m_parentPanel->WarpPointer( VECTOR2D( screenSize.x / 2, screenSize.y / 2 ) );
            }
        }
        else
        {
            m_parentPanel->WarpPointer( screenPos );
        }
    }
    else
    {
        m_parentPanel->WarpPointer( aPosition );
    }

    // If we are not refreshing because of mouse movement, don't set the modifiers because we
    // are refreshing for keyboard movement, which uses the same modifiers for other actions
    refreshMouse( m_updateCursor );
}


void QT_VIEW_CONTROLS::CenterOnCursor()
{
    const VECTOR2I& screenSize = m_view->GetGAL()->GetScreenPixelSize();
    VECTOR2D        screenCenter( screenSize / 2 );

    if( GetMousePosition( false ) != screenCenter )
    {
        VECTOR2D newCenter = GetCursorPosition();

        if( m_parentPanel->WarpPointer( screenCenter ) )
        {
            m_view->SetCenter( newCenter );
            m_dragStartPoint = screenCenter;
        }
    }
}


void QT_VIEW_CONTROLS::PinCursorInsideNonAutoscrollArea( bool aWarpMouseCursor )
{
    int border = std::min( m_settings.m_autoPanMargin * m_view->GetScreenPixelSize().x,
                           m_settings.m_autoPanMargin * m_view->GetScreenPixelSize().y );
    border += 2;

    VECTOR2D topLeft( border, border );
    VECTOR2D botRight( m_view->GetScreenPixelSize().x - border,
                       m_view->GetScreenPixelSize().y - border );

    topLeft = m_view->ToWorld( topLeft );
    botRight = m_view->ToWorld( botRight );

    VECTOR2D pos = GetMousePosition( true );

    if( pos.x < topLeft.x )
        pos.x = topLeft.x;
    else if( pos.x > botRight.x )
        pos.x = botRight.x;

    if( pos.y < topLeft.y )
        pos.y = topLeft.y;
    else if( pos.y > botRight.y )
        pos.y = botRight.y;

    SetCursorPosition( pos, false, false, 0 );

    if( aWarpMouseCursor )
        WarpMouseCursor( pos, true );
}


bool QT_VIEW_CONTROLS::handleAutoPanning( const VECTOR2I& aPosition )
{
    VECTOR2I p( aPosition );
    VECTOR2I pKey( m_view->ToScreen( m_settings.m_lastKeyboardCursorPosition ) );

    if( m_cursorWarped || ( m_settings.m_lastKeyboardCursorPositionValid && p == pKey ) )
    {
        // last cursor move event came from keyboard cursor control. If auto-panning is enabled
        // and the next position is inside the autopan zone, check if it really came from a mouse
        // event, otherwise disable autopan temporarily. Also temporarily disable autopan if the
        // cursor is in the autopan zone because the application warped the cursor.

        m_cursorWarped = false;
        return true;
    }

    m_cursorWarped = false;

    // Compute areas where autopanning is active
    int borderStart = std::min( m_settings.m_autoPanMargin * m_view->GetScreenPixelSize().x,
                                m_settings.m_autoPanMargin * m_view->GetScreenPixelSize().y );
    borderStart = std::max( borderStart, 2 );
    int borderEndX = m_view->GetScreenPixelSize().x - borderStart;
    int borderEndY = m_view->GetScreenPixelSize().y - borderStart;

    if( p.x < borderStart )
        m_panDirection.x = -( borderStart - p.x );
    else if( p.x > borderEndX )
        m_panDirection.x = ( p.x - borderEndX );
    else
        m_panDirection.x = 0;

    if( p.y < borderStart )
        m_panDirection.y = -( borderStart - p.y );
    else if( p.y > borderEndY )
        m_panDirection.y = ( p.y - borderEndY );
    else
        m_panDirection.y = 0;

    bool borderHit = ( m_panDirection.x != 0 || m_panDirection.y != 0 );

    switch( m_state )
    {
    case AUTO_PANNING:
        if( !borderHit )
        {
            m_panTimer.Stop();
            setState( IDLE );

            return false;
        }

        return true;

    case IDLE:
        if( borderHit )
        {
            setState( AUTO_PANNING );
            m_panTimer.StartOnce( (int) ( 250.0 / 60.0 ) );

            return true;
        }

        return false;

    case DRAG_PANNING:
    case DRAG_ZOOMING: return false;
    }

    KI_CHECK_MSG( false, false, wxT( "This line should never be reached" ) );

    return false;
}


void QT_VIEW_CONTROLS::handleCursorCapture( int x, int y )
{
    if( m_settings.m_cursorCaptured )
    {
        bool     warp = false;
        VECTOR2I parentSize = m_parentPanel->GetClientSizeDevice();

        if( x < 0 )
        {
            x = 0;
            warp = true;
        }
        else if( x >= parentSize.x )
        {
            x = parentSize.x - 1;
            warp = true;
        }

        if( y < 0 )
        {
            y = 0;
            warp = true;
        }
        else if( y >= parentSize.y )
        {
            y = parentSize.y - 1;
            warp = true;
        }

        if( warp )
            m_parentPanel->WarpPointer( VECTOR2D( x, y ) );
    }
}


void QT_VIEW_CONTROLS::refreshMouse( bool aSetModifiers )
{
    // Notify tools that the cursor position has changed in the world coordinates
    VECTOR2I msp = getMouseScreenPosition();
    int      mods = 0;

    if( aSetModifiers )
    {
        // Set the modifiers state
        mods |= TOOLKIT::IsKeyDown( WXK_CONTROL ) ? MD_CTRL : 0;
        mods |= TOOLKIT::IsKeyDown( WXK_SHIFT ) ? MD_SHIFT : 0;
        mods |= TOOLKIT::IsKeyDown( WXK_ALT ) ? MD_ALT : 0;
    }

    m_cursorPos = GetClampedCoords( m_view->ToWorld( VECTOR2D( msp.x, msp.y ) ) );
    m_parentPanel->PostRefreshMouse( mods );
}


VECTOR2I QT_VIEW_CONTROLS::getMouseScreenPosition() const
{
    wxPoint msp = KIPLATFORM::UI::GetMousePosition();
    return VECTOR2I( m_parentPanel->ToDevice(
            m_parentPanel->mapFromGlobal( QPointF( msp.x, msp.y ) ) ) );
}


void QT_VIEW_CONTROLS::ForceCursorPosition( bool aEnabled, const VECTOR2D& aPosition )
{
    VECTOR2D clampedPosition = GetClampedCoords( aPosition );

    m_settings.m_forceCursorPosition = aEnabled;
    m_settings.m_forcedPosition = clampedPosition;
}
