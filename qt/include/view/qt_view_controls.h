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
 * @file qt_view_controls.h
 * QT_VIEW_CONTROLS: the VIEW_CONTROLS of the canvas of the Qt build (the port of
 * WX_VIEW_CONTROLS).
 */

#ifndef __QT_VIEW_CONTROLS_H
#define __QT_VIEW_CONTROLS_H

#include <memory>

#include <view/view_controls.h>
#include <toolkit/gui_timer.h>

class QMouseEvent;
class QNativeGestureEvent;
class QWheelEvent;
class EDA_DRAW_PANEL_GAL;
class PROF_COUNTER;

namespace KIGFX
{
class ZOOM_CONTROLLER;


/**
 * An implementation of class #VIEW_CONTROLS for Qt.
 *
 * The canvas gives it its mouse events first: it zooms and pans the view (wheel, drag with
 * the middle or right button, autopanning), then the TOOL_DISPATCHER gets the events it lets
 * through.
 */
class QT_VIEW_CONTROLS : public VIEW_CONTROLS
{
public:
    QT_VIEW_CONTROLS( VIEW* aView, EDA_DRAW_PANEL_GAL* aParentPanel );
    virtual ~QT_VIEW_CONTROLS();

    /**
     * Handler functions.
     *
     * @return true if the event goes on to the tools (as a skipped event of wxWidgets).
     */
    bool onWheel( QWheelEvent* aEvent );
    bool onMotion( QMouseEvent* aEvent );
    bool onButton( QMouseEvent* aEvent );
    bool onGesture( QNativeGestureEvent* aEvent );
    void onEnter();
    void onLeave();

    void CaptureCursor( bool aEnabled ) override;

    void PinCursorInsideNonAutoscrollArea( bool aWarpMouseCursor ) override;

    /// @copydoc VIEW_CONTROLS::GetMousePosition()
    VECTOR2D GetMousePosition( bool aWorldCoordinates = true ) const override;

    using VIEW_CONTROLS::GetCursorPosition;

    /// @copydoc VIEW_CONTROLS::GetCursorPosition()
    VECTOR2D GetCursorPosition( bool aSnappingEnabled ) const override;

    /// @copydoc VIEW_CONTROLS::GetRawCursorPosition()
    VECTOR2D GetRawCursorPosition( bool aSnappingEnabled = true ) const override;

    void SetCursorPosition( const VECTOR2D& aPosition, bool warpView, bool aTriggeredByArrows,
                            long aArrowCommand ) override;

    /// @copydoc VIEW_CONTROLS::SetCrossHairCursorPosition()
    void SetCrossHairCursorPosition( const VECTOR2D& aPosition, bool aWarpView ) override;

    /// @copydoc VIEW_CONTROLS::CursorWarp()
    void WarpMouseCursor( const VECTOR2D& aPosition, bool aWorldCoordinates = false,
                          bool aWarpView = false ) override;

    /// @copydoc VIEW_CONTROLS::CenterOnCursor()
    void CenterOnCursor() override;

    /// End any mouse drag action still in progress.
    void CancelDrag();

    void ForceCursorPosition( bool aEnabled, const VECTOR2D& aPosition = VECTOR2D( 0, 0 ) ) override;

    /// Applies VIEW_CONTROLS settings from the program #COMMON_SETTINGS.
    void LoadSettings() override;

    std::unique_ptr<PROF_COUNTER> m_MotionEventCounter;

private:
    /// Possible states for QT_VIEW_CONTROLS.
    enum STATE
    {
        IDLE = 1,     ///< Nothing is happening.
        DRAG_PANNING, ///< Panning with mouse button pressed.
        AUTO_PANNING, ///< Panning on approaching borders of the frame.
        DRAG_ZOOMING, ///< Zooming with mouse button pressed.
    };

    /// Set the interaction state, simply a proxy for assignment.
    void setState( STATE aNewState );

    /**
     * Compute new viewport settings while in autopanning mode.
     *
     * @param aPosition is the position of the mouse in the canvas.
     * @return true if it is currently autopanning (ie. autopanning is active and mouse cursor
     *         is in the area that causes autopanning to happen).
     */
    bool handleAutoPanning( const VECTOR2I& aPosition );

    /// Panning of the autopanning, at each step of its timer.
    void onTimer();

    /**
     * Limit the cursor position to within the canvas by warping it
     *
     * @param x Mouse position
     * @param y Mouse position
     */
    void handleCursorCapture( int x, int y );

    /**
     * Send an event to refresh mouse position.
     *
     * It is mostly used for notifying the tools that the cursor position in the world
     * coordinates has changed.
     *
     * @param aSetModifiers If false, don't change the modifiers (they were set using the
     *                      keyboard motion).
     */
    void refreshMouse( bool aSetModifiers );

    /**
     * Get the cursor position in the screen coordinates.
     */
    VECTOR2I getMouseScreenPosition() const;

    /// Current state of VIEW_CONTROLS.
    STATE m_state;

    /// Panel that is affected by VIEW_CONTROLS.
    EDA_DRAW_PANEL_GAL* m_parentPanel;

    /// Store information about point where dragging has started.
    VECTOR2D m_dragStartPoint;

    /// Current direction of panning (only autopanning mode).
    VECTOR2D m_panDirection;

    /// Timer responsible for handling autopanning.
    GUI_TIMER m_panTimer;

    /// The mouse position when a drag zoom started.
    VECTOR2D m_zoomStartPoint;

    /// Current cursor position (world coordinates).
    VECTOR2D m_cursorPos;

    /// Flag deciding whether the cursor position should be calculated using the mouse position.
    bool m_updateCursor;

    /// True if we are panning via the meta key.
    bool m_metaPanning;

    /// Last mouse position when panning via the meta key.
    VECTOR2D m_metaPanStart;

    /// Flag to indicate if infinite panning works on this platform.
    bool m_infinitePanWorks;

    /// The last pan or zoom of a drag warped the cursor (the next motion comes from the warp).
    bool m_justWarped;

    /// A #ZOOM_CONTROLLER that determines zoom steps. This is platform-specific.
    std::unique_ptr<ZOOM_CONTROLLER> m_zoomController;
};

} // namespace KIGFX

#endif
