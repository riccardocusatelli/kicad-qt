/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2018 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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
 * @file class_draw_panel_gal.h
 * EDA_DRAW_PANEL_GAL of the Qt build: the canvas of the editors, on a QWidget.
 */

#ifndef  PANELGAL_WXSTRUCT_H
#define  PANELGAL_WXSTRUCT_H

#include <chrono>
#include <memory>
#include <mutex>

#include <kiqt.h>
#include <QImage>
#include <QWidget>

#include <wx/kiwx_window.h>

#include <gal/gal_backend.h>
#include <math/box2.h>
#include <math/vector2d.h>
#include <widgets/msgpanel_item.h>
#include <gal/kicursor.h>
#include <toolkit/gui_timer.h>
#include <wx/gdicmn.h>
#include <core/ki_assert.h>

class BOARD;
class EDA_DRAW_FRAME;
class TOOL_DISPATCHER;
class PROF_COUNTER;

namespace KIGFX
{
class GAL;
class VIEW;
class QT_CAIRO_GAL;
class QT_VIEW_CONTROLS;
class VIEW_CONTROLS;
class PAINTER;
class GAL_DISPLAY_OPTIONS;
class VIEW_OVERLAY;
struct VC_SETTINGS;
}


/**
 * The canvas of the editors: shows a VIEW drawn by a GAL, and gives the mouse and key events
 * to its view controls (zoom, pan) and to the tools (through the TOOL_DISPATCHER).
 *
 * The GAL draws in device pixels: the positions of the view controls and the tools on the
 * screen are in device pixels.
 */
class EDA_DRAW_PANEL_GAL : public wxWindowCompat<QWidget>
{
public:
    typedef KIGFX::GAL_BACKEND_TYPE GAL_TYPE;
    using enum KIGFX::GAL_BACKEND_TYPE;

    static constexpr GAL_TYPE GAL_FALLBACK = GAL_TYPE_CAIRO;

    static constexpr bool GAL_FALLBACK_AVAILABLE = GAL_FALLBACK != GAL_TYPE_OPENGL;

    /**
     * Create a drawing panel that is contained inside \p aParentWindow.
     *
     * If \p aParentWindow is not an EDA frame, a search through all the parents
     * of the parent window will be done to find the frame.
     *
     * @param aParentWindow is the window immediately containing this panel
     * @param aOptions are the display options of the GAL
     * @param aGalType is the type of GAL to use (the Qt build draws with Cairo)
     */
    EDA_DRAW_PANEL_GAL( QWidget* aParentWindow, KIGFX::GAL_DISPLAY_OPTIONS& aOptions,
                        GAL_TYPE aGalType = GAL_TYPE_OPENGL );

    /**
     * Create a drawing panel as the canvases of the editors do (the constructor of the wxWidgets
     * build).
     *
     * @param aWindowId is the id of the panel.
     * @param aPosition is ignored: the panel is laid out by its parent.
     * @param aSize is the initial size of the panel (wxDefaultSize for none).
     */
    EDA_DRAW_PANEL_GAL( QWidget* aParentWindow, wxWindowID aWindowId, const wxPoint& aPosition,
                        const wxSize& aSize, KIGFX::GAL_DISPLAY_OPTIONS& aOptions,
                        GAL_TYPE aGalType = GAL_TYPE_OPENGL );
    ~EDA_DRAW_PANEL_GAL();

    /// Give the focus to the canvas.
    void SetFocus();

    bool StatusPopupHasFocus();

    void SetStatusPopup( QWidget* aPopup )
    {
        m_statusPopup = aPopup;
    }

    /**
     * Switch method of rendering graphics.
     *
     * @param aGalType is a type of rendering engine that you want to use.
     */
    virtual bool SwitchBackend( GAL_TYPE aGalType );

    /**
     * Return the type of backend currently used by GAL canvas.
     */
    inline GAL_TYPE GetBackend() const { return m_backend; }

    /**
     * Return a pointer to the GAL instance used in the panel.
     *
     * @return The instance of GAL.
     */
    KIGFX::GAL* GetGAL() const { return m_gal; }

    /**
     * Return a pointer to the #VIEW instance used in the panel.
     *
     * @return The instance of #VIEW.
     */
    virtual KIGFX::VIEW* GetView() const { return m_view; }

    /**
     * Return a pointer to the #VIEW_CONTROLS instance used in the panel.
     *
     * @return The instance of #VIEW_CONTROLS.
     */
    KIGFX::VIEW_CONTROLS* GetViewControls() const;

    /// The view controls of the canvas, with their handlers of the events of Qt.
    KIGFX::QT_VIEW_CONTROLS* GetQtViewControls() const { return m_viewControls; }

    /// The image of the canvas.
    bool GetScreenshot( QImage& aDstImage );

    /**
     * Repaint the canvas, if it has changed.
     *
     * @param aEraseBackground and @param aRect are unused: the canvas is always drawn whole (the
     *                         parameters of wxWindow::Refresh()).
     */
    virtual void Refresh( bool aEraseBackground = true, const wxRect* aRect = nullptr ) override;

    /// The canvas doesn't scroll (its view pans): the origin of wxScrolledCanvas::GetViewStart().
    void GetViewStart( int* aX, int* aY ) const
    {
        if( aX )
            *aX = 0;

        if( aY )
            *aY = 0;
    }

    /**
     * Force a redraw.
     */
    void ForceRefresh();

    /**
     * Make sure a refresh gets done on the next idle event if it hasn't already.
     */
    void RequestRefresh();

    /**
     * Resize the GAL to the size of the canvas.
     */
    void ResizeGal( bool aForce = false );

    /**
     * Set a dispatcher that processes events and forwards them to tools.
     *
     * #DRAW_PANEL_GAL does not take over the ownership. Passing NULL disconnects all event
     * handlers from the #DRAW_PANEL_GAL and parent frame.
     *
     * @param aEventDispatcher is the object that will be used for dispatching events.
     */
    void SetEventDispatcher( TOOL_DISPATCHER* aEventDispatcher );

    /**
     * Begin drawing if it was stopped previously.
     */
    void StartDrawing();

    /**
     * Prevent the GAL canvas from further drawing until it is recreated or #StartDrawing()
     * is called.
     */
    void StopDrawing();

    /**
     * Take care of display settings for the given layer to be displayed in high contrast mode.
     */
    virtual void SetHighContrastLayer( int aLayer );

    /**
     * Move the selected layer to the top, so it is displayed above all others.
     */
    virtual void SetTopLayer( int aLayer );

    virtual void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
    {
        KI_ASSERT( false );
    }

    /**
     * Returns parent EDA_DRAW_FRAME, if available or NULL otherwise.
     */
    EDA_DRAW_FRAME* GetParentEDAFrame() const { return m_edaFrame; }

    bool IsDialogPreview() const;

    /**
     * Called when the window is shown for the first time.
     */
    virtual void OnShow() {}

    /**
     * Set whether focus is taken on certain events (mouseover, keys, etc).
     *
     * This should be true (and is by default) for any primary canvas, but can be false to make
     * well-behaved preview panes and the like.
     */
    void SetStealsFocus( bool aStealsFocus ) { m_stealsFocus = aStealsFocus; }

    /**
     * Set the current cursor shape for this panel.
     */
    void SetCurrentCursor( KICURSOR aCursor );

    /**
     * Return the bounding box of the view that should be used if model is not valid.
     * For example, the drawing sheet bounding box for an empty PCB
     *
     * @return the default bounding box for the panel.
     */
    virtual BOX2I GetDefaultViewBBox() const { return BOX2I(); }

    /**
     * Repaint the canvas, and fix scrollbar cursors
     *
     * Usually called by a OnPaint event.
     *
     * Because it does not use a wxPaintDC, it can be called outside a wxPaintEvent.
     *
     * @return true if the repaint attempt was successful.
     */
    bool DoRePaint( bool aAllowSkip = true );

    /**
     * Create an overlay for rendering debug graphics.
     */
    std::shared_ptr<KIGFX::VIEW_OVERLAY> DebugOverlay();

    /**
     * Clear the contents of the debug overlay and removes it from the VIEW.
     */
    void ClearDebugOverlay();

    /**
     * Gets a populated View Controls settings object dervived from our program settings
     */
    static KIGFX::VC_SETTINGS GetVcSettings();

    /// A point of the canvas (in logical pixels, as the events of Qt) in device pixels.
    VECTOR2D ToDevice( const QPointF& aPoint ) const;

    /// The size of the canvas in logical pixels.
    wxSize GetClientSize() const { return wxSize( width(), height() ); }

    /// The size of the canvas in device pixels.
    VECTOR2I GetClientSizeDevice() const;

    /// The size the scroll bars take in the canvas (the Qt canvas has none).
    VECTOR2I GetScrollbarSize() const { return VECTOR2I( 0, 0 ); }

    /**
     * Move the mouse cursor to a point of the canvas, in device pixels.
     *
     * @return true if the cursor was moved.
     */
    bool WarpPointer( const VECTOR2D& aDevicePoint );

    /**
     * Ask the tools to see the position of the cursor in the world again, from the event loop
     * (the view moved under the cursor).
     *
     * @param aModifiers are the modifiers (MD_SHIFT...) of the motion.
     */
    void PostRefreshMouse( int aModifiers );

    std::unique_ptr<PROF_COUNTER> m_PaintEventCounter;

protected:
    bool event( QEvent* aEvent ) override;
    void paintEvent( QPaintEvent* aEvent ) override;
    void resizeEvent( QResizeEvent* aEvent ) override;
    void showEvent( QShowEvent* aEvent ) override;
    void focusOutEvent( QFocusEvent* aEvent ) override;
    bool focusNextPrevChild( bool aNext ) override;

    /// Give an input event to the view controls, then to the tools.
    bool onInputEvent( QEvent* aEvent );

    void onEnter();
    void onRefreshTimer();

    bool recoverFromGalError( const std::exception& aErr );

    /// Create the view controls of the canvas, once its view is made.
    void createViewControls();

    /// Copy the raster of the GAL (at the end of each drawing).
    void present();

    QWidget*                 m_parent;           ///< Pointer to the parent window
    EDA_DRAW_FRAME*          m_edaFrame;         ///< Parent EDA_DRAW_FRAME (if available)

    std::chrono::steady_clock::time_point m_lastRepaintStart; ///< Timestamp of the last repaint start
    std::chrono::steady_clock::time_point m_lastRepaintEnd;   ///< Timestamp of the last repaint end
    GUI_TIMER                             m_refreshTimer;     ///< Timer to prevent too-frequent refreshing

    std::mutex               m_refreshMutex;     ///< Blocks multiple calls to the draw

    /// True if GAL is currently redrawing the view
    bool                     m_drawing;

    /// Flag that determines if VIEW may use GAL for redrawing the screen.
    bool                     m_drawingEnabled;

    /// Flag to indicate that a deferred refresh is needed.
    bool                     m_needIdleRefresh;

    /// Last cursor position, to avoid redrawing when the cursor did not move
    VECTOR2D                 m_lastCursorPosition;

    /// Interface for drawing objects on a 2D-surface (a QT_CAIRO_GAL)
    KIGFX::GAL*              m_gal;

    /// Stores view settings (scale, center, etc.) and items to be drawn
    KIGFX::VIEW*             m_view;

    /// Contains information about how to draw items using GAL
    std::unique_ptr<KIGFX::PAINTER> m_painter;

    /// Control for VIEW (moving, zooming, etc.)
    KIGFX::QT_VIEW_CONTROLS* m_viewControls;

    /// Currently used GAL
    GAL_TYPE                 m_backend;
    KIGFX::GAL_DISPLAY_OPTIONS& m_options;

    /// Processes and forwards events to tools
    TOOL_DISPATCHER*         m_eventDispatcher;

    /// Flag to indicate that focus should be regained on the next mouse event. It is a
    /// workaround for cases when the panel loses keyboard focus, so it does not react to
    /// hotkeys anymore.
    bool                     m_lostFocus;

    /// Flag to indicate whether the panel should take focus at certain times (when moused
    /// over, and on various mouse/key events)
    bool                     m_stealsFocus;

    QWidget*                 m_statusPopup;

    /// The last drawing of the GAL
    QImage                   m_image;

    /// Optional overlay for drawing transient debug objects
    std::shared_ptr<KIGFX::VIEW_OVERLAY> m_debugOverlay;
};

#endif
