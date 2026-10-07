/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright The KiCad Developers, see CHANGELOG.txt for contributors.
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
 * @file tool_dispatcher.cpp
 * TOOL_DISPATCHER of the Qt build: the state machine of common/tool/tool_dispatcher.cpp,
 * fed by the events of Qt.
 */

#include <tool/tool_dispatcher.h>

#include <bit>
#include <chrono>
#include <optional>

#include <QAbstractSpinBox>
#include <QApplication>
#include <QComboBox>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QStyleHints>
#include <QTextEdit>
#include <QWheelEvent>

#include <core/kicad_algo.h>
#include <trace_helpers.h>
#include <tool/tool_manager.h>
#include <tool/actions.h>
#include <tool/action_manager.h>
#include <view/view.h>
#include <view/view_controls.h>
#include <toolkit/gui_services.h>
#include <core/ki_log.h>
#include <core/ki_assert.h>

#include <toolkit/qt_keys.h>


/// The time, in milliseconds.
static int64_t localTimeMillis()
{
    using namespace std::chrono;
    return duration_cast<milliseconds>( steady_clock::now().time_since_epoch() ).count();
}


/// Store information about a mouse button state.
struct TOOL_DISPATCHER::BUTTON_STATE
{
    BUTTON_STATE( TOOL_MOUSE_BUTTONS aButton, Qt::MouseButton aQtButton ) :
        dragging( false ),
        pressed( false ),
        button( aButton ),
        qtButton( aQtButton ),
        downTimestamp( 0 )
    {};

    /// Flag indicating that dragging is active for the given button.
    bool dragging;

    /// Flag indicating that the given button is pressed.
    bool pressed;

    /// Point where dragging has started (in world coordinates).
    VECTOR2D dragOrigin;

    /// Point where dragging has started (in screen coordinates).
    VECTOR2D dragOriginScreen;

    /// Point where click event has occurred.
    VECTOR2D downPosition;

    /// Determines the mouse button for which information are stored.
    TOOL_MOUSE_BUTTONS button;

    /// The button of Qt.
    Qt::MouseButton qtButton;

    /// Time stamp for the last mouse button press event.
    int64_t downTimestamp;

    /// Restores initial state.
    void Reset()
    {
        dragging = false;
        pressed = false;
    }

    /// Checks the current state of the button.
    bool GetState() const
    {
        return QGuiApplication::mouseButtons() & qtButton;
    }

    /// The state of the button when an event of the mouse happened.
    bool GetState( const QEvent* aEvent ) const
    {
        if( aEvent->isSinglePointEvent() )
            return static_cast<const QSinglePointEvent*>( aEvent )->buttons() & qtButton;

        return GetState();
    }
};


TOOL_DISPATCHER::TOOL_DISPATCHER( TOOL_MANAGER* aToolMgr ) :
    m_lastKeyCode( 0 ),
    m_lastKeyTime( 0 ),
    m_toolMgr( aToolMgr )
{
    int dragDistance = QGuiApplication::styleHints()->startDragDistance();

    m_sysDragMinX = dragDistance > 0 ? dragDistance : DragDistanceThreshold;
    m_sysDragMinY = m_sysDragMinX;

    m_buttons.push_back( new BUTTON_STATE( BUT_LEFT, Qt::LeftButton ) );
    m_buttons.push_back( new BUTTON_STATE( BUT_RIGHT, Qt::RightButton ) );
    m_buttons.push_back( new BUTTON_STATE( BUT_MIDDLE, Qt::MiddleButton ) );
    m_buttons.push_back( new BUTTON_STATE( BUT_AUX1, Qt::BackButton ) );
    m_buttons.push_back( new BUTTON_STATE( BUT_AUX2, Qt::ForwardButton ) );

    // The keys pressed are known from now on
    TOOLKIT::TrackKeyState();

    ResetState();
}


TOOL_DISPATCHER::~TOOL_DISPATCHER()
{
    for( BUTTON_STATE* st : m_buttons )
        delete st;
}


void TOOL_DISPATCHER::ResetState()
{
    for( BUTTON_STATE* st : m_buttons )
        st->Reset();

    m_lastKeyCode = 0;
    m_lastKeyTime = 0;
}


KIGFX::VIEW* TOOL_DISPATCHER::getView()
{
    return m_toolMgr->GetView();
}


bool TOOL_DISPATCHER::handleMouseButton( QEvent* aEvent, int aIndex, bool aMotion, int aMods )
{
    BUTTON_STATE* st = m_buttons[aIndex];
    std::optional<TOOL_EVENT> evt;
    bool isClick = false;
    bool up = false, down = false;
    bool dblClick = false;
    bool state = st->GetState( aEvent );

    QMouseEvent* me = ( aEvent->type() == QEvent::MouseButtonPress
                        || aEvent->type() == QEvent::MouseButtonRelease
                        || aEvent->type() == QEvent::MouseButtonDblClick )
                              ? static_cast<QMouseEvent*>( aEvent )
                              : nullptr;

    bool thisButton = me && me->button() == st->qtButton;

    dblClick = thisButton && aEvent->type() == QEvent::MouseButtonDblClick;

    if( !dblClick )
    {
        // Sometimes the dispatcher does not receive mouse button up event, so it stays
        // in the dragging mode even if the mouse button is not held anymore
        if( st->pressed && !state )
            up = true;
        // Don't apply same logic to down events as it kills touchpad tapping
        else if( !st->pressed && thisButton && aEvent->type() == QEvent::MouseButtonPress )
            down = true;
    }

    int args = st->button | aMods;

    if( down )      // Handle mouse button press
    {
        st->downTimestamp = localTimeMillis();

        if( !st->pressed )      // save the drag origin on the first click only
        {
            st->dragOrigin = m_lastMousePos;
            st->dragOriginScreen = m_lastMousePosScreen;
        }

        st->downPosition = m_lastMousePos;
        st->pressed = true;
        evt = TOOL_EVENT( TC_MOUSE, TA_MOUSE_DOWN, args );
    }
    else if( up )   // Handle mouse button release
    {
        st->pressed = false;

        if( st->dragging )
            evt = TOOL_EVENT( TC_MOUSE, TA_MOUSE_UP, args );
        else
            isClick = true;

        if( isClick )
            evt = TOOL_EVENT( TC_MOUSE, TA_MOUSE_CLICK, args );

        st->dragging = false;
    }
    else if( dblClick )
    {
        evt = TOOL_EVENT( TC_MOUSE, TA_MOUSE_DBLCLICK, args );
    }

    if( st->pressed && aMotion )
    {
        if( !st->dragging )
        {
#ifdef __APPLE__
            if( localTimeMillis() - st->downTimestamp > DragTimeThreshold )
                st->dragging = true;
#endif
            VECTOR2D offset = m_lastMousePosScreen - st->dragOriginScreen;

            if( abs( offset.x ) > m_sysDragMinX || abs( offset.y ) > m_sysDragMinY )
                st->dragging = true;
        }

        if( st->dragging )
        {
            evt = TOOL_EVENT( TC_MOUSE, TA_MOUSE_DRAG, args );
            evt->setMouseDragOrigin( st->dragOrigin );
            evt->setMouseDelta( m_lastMousePos - st->dragOrigin );
        }
    }

    if( evt )
    {
        evt->SetMousePosition( isClick ? st->downPosition : m_lastMousePos );
        m_toolMgr->ProcessEvent( *evt );

        return true;
    }

    return false;
}


/**
 * Helper to know if a special key ( see key list ) should be captured.
 *
 * These keys have a predefined action in the widgets (scrolling), which must not run even if
 * KiCad does not use the key.
 */
static bool isKeySpecialCode( int aKeyCode )
{
    // These keys have predefined actions (like move thumbtrack cursor),
    // and we do not want these actions executed
    const std::vector<int> special_keys =
    {
        WXK_PAGEUP, WXK_PAGEDOWN,
        WXK_NUMPAD_PAGEUP, WXK_NUMPAD_PAGEDOWN
    };

    return alg::contains( special_keys, aKeyCode );
}


/**
 * @return true if the widget that has the focus edits text: the tools must not get the keys
 *         typed in it.
 */
static bool isTextInputFocused( bool* aEditable )
{
    QWidget* focus = QApplication::focusWidget();

    if( QLineEdit* edit = qobject_cast<QLineEdit*>( focus ) )
    {
        *aEditable = !edit->isReadOnly();
        return true;
    }

    if( QTextEdit* edit = qobject_cast<QTextEdit*>( focus ) )
    {
        *aEditable = !edit->isReadOnly();
        return true;
    }

    if( QPlainTextEdit* edit = qobject_cast<QPlainTextEdit*>( focus ) )
    {
        *aEditable = !edit->isReadOnly();
        return true;
    }

    if( QAbstractSpinBox* spin = qobject_cast<QAbstractSpinBox*>( focus ) )
    {
        *aEditable = !spin->isReadOnly();
        return true;
    }

    if( QComboBox* combo = qobject_cast<QComboBox*>( focus ) )
    {
        *aEditable = combo->isEditable();
        return combo->isEditable();
    }

    return false;
}


std::optional<TOOL_EVENT> TOOL_DISPATCHER::GetToolEvent( QKeyEvent* aKeyEvent, bool* keyIsSpecial )
{
    std::optional<TOOL_EVENT> evt;

    // A key that is only a modifier: not useful in KiCad
    if( TOOLKIT::IsModifierKey( aKeyEvent->key() ) )
        return evt;

    int key = TOOLKIT::KeyCodeFromQt( *aKeyEvent );

    if( key == 0 )
        return evt;

    KI_LOG_TRACE( kicadTraceKeyEvent, wxS( "TOOL_DISPATCHER::GetToolEvent key %d" ), key );

    *keyIsSpecial = isKeySpecialCode( key );

    int mods = TOOLKIT::ModifiersFromQt( aKeyEvent->modifiers() );

    if( key == WXK_ESCAPE ) // ESC is the special key for canceling tools
        evt = TOOL_EVENT( TC_COMMAND, TA_CANCEL_TOOL, WXK_ESCAPE );
    else
        evt = TOOL_EVENT( TC_KEYBOARD, TA_KEY_PRESSED, key | mods );

    return evt;
}


void TOOL_DISPATCHER::flushPendingClicks()
{
    // When an escape key event arrives, keyboard events can be processed before mouse button
    // events. If a mouse button was pressed and has since been released (detected via polling),
    // we need to process that click before handling the escape to maintain proper event
    // ordering.
    for( BUTTON_STATE* st : m_buttons )
    {
        if( st->pressed && !st->GetState() )
        {
            st->pressed = false;

            TOOL_EVENT clickEvt( TC_MOUSE, TA_MOUSE_CLICK, st->button );
            clickEvt.SetMousePosition( st->downPosition );
            m_toolMgr->ProcessEvent( clickEvt );

            st->dragging = false;
        }
    }
}


bool TOOL_DISPATCHER::ShouldDropAutoRepeat( int aKeyCode, int64_t aNowMs, bool aKeyIsDown,
                                            int& aLastKey, int64_t& aLastTimeMs )
{
    bool sameBurst = aKeyCode == aLastKey && ( aNowMs - aLastTimeMs ) < AutoRepeatWindowMs;

    aLastKey = aKeyCode;
    aLastTimeMs = aNowMs;

    return sameBurst && !aKeyIsDown;
}


bool TOOL_DISPATCHER::isStaleAutoRepeat( int aKeyCode )
{
    bool keyIsDown = TOOLKIT::IsKeyDown( aKeyCode );

    return ShouldDropAutoRepeat( aKeyCode, localTimeMillis(), keyIsDown, m_lastKeyCode,
                                 m_lastKeyTime );
}


bool TOOL_DISPATCHER::dispatchMouseEvent( QEvent* aEvent, int aMods )
{
    std::optional<TOOL_EVENT> evt;
    bool                      motion = false;
    bool                      buttonEvents = false;
    VECTOR2D                  pos;

    if( m_toolMgr->GetViewControls() )
    {
        pos = m_toolMgr->GetViewControls()->GetMousePosition();
        m_lastMousePosScreen = m_toolMgr->GetViewControls()->GetMousePosition( false );

        if( pos != m_lastMousePos )
        {
            motion = true;
            m_lastMousePos = pos;
        }
    }

    for( unsigned int i = 0; i < m_buttons.size(); i++ )
        buttonEvents |= handleMouseButton( aEvent, i, motion, aMods );

    if( m_toolMgr->GetViewControls() )
    {
        if( !buttonEvents && motion )
        {
            evt = TOOL_EVENT( TC_MOUSE, TA_MOUSE_MOTION, aMods );
            evt->SetMousePosition( pos );
        }
    }

    // We only handle wheel events that aren't for the view control.
    // Events with zero or one modifier are reserved for view control.
    if( !evt && aEvent->type() == QEvent::Wheel )
    {
        QWheelEvent* we = static_cast<QWheelEvent*>( aEvent );
        int          rotation = we->angleDelta().y() != 0 ? we->angleDelta().y()
                                                          : we->angleDelta().x();

        const unsigned modBits = static_cast<unsigned>( aMods ) & MD_MODIFIER_MASK;
        const bool     shouldHandle = std::popcount( modBits ) > 1;

        if( rotation != 0 && shouldHandle )
        {
            evt = TOOL_EVENT( TC_MOUSE, TA_MOUSE_WHEEL, aMods );
            evt->SetParameter<int>( rotation );
        }
    }

    bool handled = buttonEvents;

    if( evt )
    {
        KI_LOG_TRACE( kicadTraceToolStack, wxS( "TOOL_DISPATCHER::DispatchQtEvent %s" ),
                      evt->Format() );

        handled |= m_toolMgr->ProcessEvent( *evt );
    }

    return handled;
}


void TOOL_DISPATCHER::RefreshMouse( int aModifiers )
{
    // The cursor stays at the same place of the screen but moves in the world (autopanning,
    // zooming): the same as a motion of the mouse
    QMouseEvent motion( QEvent::MouseMove, QPointF(), QPointF(), Qt::NoButton,
                        QGuiApplication::mouseButtons(), Qt::NoModifier );

    dispatchMouseEvent( &motion, aModifiers );
}


bool TOOL_DISPATCHER::DispatchQtEvent( QEvent* aEvent )
{
    std::optional<TOOL_EVENT> evt;
    bool            keyIsEscape  = false;  // True if the keypress was the escape key
    bool            keyIsSpecial = false;  // True if the key is a special key code
    bool            droppedStaleAutoRepeat = false;  // True if a backlogged repeat was discarded

    if( m_lastKeyCode > 0 && !TOOLKIT::IsKeyDown( m_lastKeyCode ) )
        m_lastKeyCode = 0;

    switch( aEvent->type() )
    {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseMove:
    {
        QMouseEvent* me = static_cast<QMouseEvent*>( aEvent );
        return dispatchMouseEvent( aEvent, TOOLKIT::ModifiersFromQt( me->modifiers() ) );
    }

    case QEvent::Wheel:
    {
        QWheelEvent* we = static_cast<QWheelEvent*>( aEvent );
        return dispatchMouseEvent( aEvent, TOOLKIT::ModifiersFromQt( we->modifiers() ) );
    }

    case QEvent::KeyPress:
    {
        QKeyEvent* ke = static_cast<QKeyEvent*>( aEvent );

        keyIsEscape = ( ke->key() == Qt::Key_Escape );

        // When escape is pressed shortly after a mouse click, the keyboard event can be
        // processed before the mouse button release event. Flush any pending clicks first
        // to ensure proper event ordering.
        if( keyIsEscape )
            flushPendingClicks();

        bool editable = false;

        if( isTextInputFocused( &editable ) )
        {
            // Never process key events for tools when a text entry has focus
            if( editable )
                return false;
            // Even if not enabled, allow a copy out
            else if( ke->modifiers() == Qt::ControlModifier && ke->key() == Qt::Key_C )
                return false;
        }

        evt = GetToolEvent( ke, &keyIsSpecial );

        // Drop backlogged OS key auto-repeat events that arrive after the key was released so a
        // slow hotkey action (e.g. Rotate on a large selection) stops when the key is let go
        // instead of running the whole queued burst. Only ordinary hotkey presses are filtered;
        // cancel (escape) is always honoured.
        if( evt && !evt->IsCancel() && !keyIsEscape && ke->isAutoRepeat()
            && isStaleAutoRepeat( TOOLKIT::KeyCodeFromQt( *ke ) ) )
        {
            evt.reset();
            droppedStaleAutoRepeat = true;
        }
        else if( evt && !keyIsEscape )
        {
            m_lastKeyCode = TOOLKIT::KeyCodeFromQt( *ke );
            m_lastKeyTime = localTimeMillis();
        }

        break;
    }

    default:
        return false;
    }

    bool handled = false;

    if( evt )
    {
        KI_LOG_TRACE( kicadTraceToolStack, wxS( "TOOL_DISPATCHER::DispatchQtEvent %s" ),
                      evt->Format() );

        handled = m_toolMgr->ProcessEvent( *evt );

        KI_LOG_TRACE( kicadTraceToolStack,
                      wxS( "TOOL_DISPATCHER::DispatchQtEvent - Handled: %s  %s" ),
                      ( handled ? wxS( "true" ) : wxS( "false" ) ), evt->Format() );
    }

    // A dropped stale auto-repeat is fully swallowed.  The special keys (page up/down), which
    // scroll the widgets, and Escape, which cancels the tools, never go to the GUI.
    return handled || droppedStaleAutoRepeat || keyIsSpecial || keyIsEscape;
}
