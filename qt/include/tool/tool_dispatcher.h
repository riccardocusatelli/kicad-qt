/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * @file tool_dispatcher.h
 * TOOL_DISPATCHER of the Qt build: the interface of include/tool/tool_dispatcher.h for the
 * events of Qt.
 */

#ifndef TOOL_DISPATCHER_H
#define TOOL_DISPATCHER_H

#include <cstdint>
#include <optional>
#include <vector>

#include <tool/tool_event.h>   // Needed for MD_ constants

class QEvent;
class QKeyEvent;
class TOOL_MANAGER;
class ACTIONS;
class ACTION_MENU;

namespace KIGFX
{
class VIEW;
}

/**
 * - takes events of the GUI toolkit,
 * - fixes all its silly quirks,
 * - converts them to TOOL_EVENTs,
 * - forwards the TOOL_EVENTs to the tools through the TOOL_MANAGER.
 *
 * In the Qt build, the events are the mouse, wheel and key events of Qt that the canvas
 * (and the frame, for the hotkeys) receives.
 */
class TOOL_DISPATCHER
{
public:
    /**
     * @param aToolMgr: tool manager instance the events will be sent to.
     */
    TOOL_DISPATCHER( TOOL_MANAGER* aToolMgr );

    virtual ~TOOL_DISPATCHER();

    /**
     * Bring the dispatcher to its initial state.
     */
    virtual void ResetState();

    /**
     * Process events of Qt (mouse, wheel and key events), translate them to TOOL_EVENTs, and
     * make tools handle those.
     *
     * @param aEvent is the Qt event to be processed.
     * @return true if the event was used by the tools (it must not go to the GUI).
     */
    virtual bool DispatchQtEvent( QEvent* aEvent );

    /**
     * Tell the tools that the position of the cursor in the world changed while the mouse did
     * not move (the view was zoomed or panned).
     *
     * @param aModifiers are the modifiers (MD_SHIFT...) of the motion event.
     */
    void RefreshMouse( int aModifiers );

    /**
     * Map a key event of Qt to a TOOL_EVENT.
     */
    std::optional<TOOL_EVENT> GetToolEvent( QKeyEvent* aKeyEvent, bool* aSpecialKeyFlag );

    /// The maximum gap (ms) between two events of the same key for them to count as one OS
    /// auto-repeat burst rather than two deliberate key presses.
    static const int AutoRepeatWindowMs = 250;

    /**
     * Pure decision used to discard backlogged OS key auto-repeat events.
     *
     * A repeat event is stale (and must be dropped) only when it belongs to the same burst as
     * the previously processed event (same key with no key-up, and less than AutoRepeatWindowMs
     * ago) and the key is no longer physically held. The leading edge of every press is kept,
     * so a quick tap whose key has already bounced up still runs once.
     *
     * @param aKeyCode    key code of the current event.
     * @param aNowMs      current time in milliseconds.
     * @param aKeyIsDown  whether the key is currently physically pressed.
     * @param aLastKey    in/out key code of the previously processed event; updated to aKeyCode.
     * @param aLastTimeMs in/out time of the previously processed event; updated to aNowMs.
     * @return true when the event is a stale auto-repeat that should be dropped.
     */
    static bool ShouldDropAutoRepeat( int aKeyCode, int64_t aNowMs, bool aKeyIsDown,
                                      int& aLastKey, int64_t& aLastTimeMs );

private:
    /// Handles mouse related events (click, motion, dragging).
    bool handleMouseButton( QEvent* aEvent, int aIndex, bool aMotion, int aMods );

    /// Processes any pending mouse clicks that have been physically completed but not yet
    /// dispatched. This ensures clicks are processed before cancel events when both happen
    /// in quick succession.
    void flushPendingClicks();

    /// Decide whether a keyboard event must be dropped because it is a backlogged OS key
    /// auto-repeat event delivered after the key was physically released.
    bool isStaleAutoRepeat( int aKeyCode );

    /// The mouse events: the buttons, the motion (and the refresh of the cursor position).
    bool dispatchMouseEvent( QEvent* aEvent, int aMods );

    /// Returns the instance of VIEW, used by the application.
    KIGFX::VIEW* getView();

private:
    /// The time threshold for a mouse button press that distinguishes between a single mouse
    /// click and a beginning of drag event (expressed in milliseconds).
    static const int DragTimeThreshold = 300;

    /// The distance threshold for mouse cursor that distinguishes between a single mouse click
    /// and a beginning of drag event (expressed in screen pixels).
    /// System drag preferences take precedence if available
    static const int DragDistanceThreshold = 8;

    int      m_sysDragMinX;          ///< Minimum distance before drag is activated in the X axis
    int      m_sysDragMinY;          ///< Maximum distance before drag is activated in the Y axis

    VECTOR2D m_lastMousePos;         ///< The last mouse cursor position (in world coordinates).
    VECTOR2D m_lastMousePosScreen;   ///< The last mouse cursor position (in screen coordinates).

    /// State of mouse buttons.
    struct BUTTON_STATE;
    std::vector<BUTTON_STATE*> m_buttons;

    /// Key code of the key that was last processed, or 0 when no key is armed. Used together
    /// with m_lastKeyTime to drop stale auto-repeat events that arrive after the key was
    /// released while still letting the leading edge of every fresh press through.
    int       m_lastKeyCode;

    /// Local time (ms) at which m_lastKeyCode was last processed.
    int64_t   m_lastKeyTime;

    /// Instance of tool manager that cooperates with the dispatcher.
    TOOL_MANAGER* m_toolMgr;
};

#endif  // TOOL_DISPATCHER_H
