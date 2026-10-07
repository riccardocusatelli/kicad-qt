/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2017 Bernhard Stegmaier <stegmaier@sw-systems.de>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Base class for HiDPI aware wxGLCanvas implementations.
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

#ifndef HIDPI_GL_CANVAS_H
#define HIDPI_GL_CANVAS_H

#include <kicad_gl/kiglad.h> // Must be included first
#include <wx/glcanvas.h>

#include <view/view_controls.h>
#include <gal/gal.h>
#include <gal/cursors.h>
#include <toolkit/toolkit_types.h>

class wxGestureEvent;
class wxSetCursorEvent;


/**
 * wxGLCanvas wrapper for HiDPI/Retina support.
 *
 * This is a small wrapper class to enable HiDPI/Retina support for wxGLCanvas.  For the
 * OpenGL GAL, it is also the window: it gives the mouse events to the panel of the GAL, asks
 * it to draw, and shows the cursor.  The Qt build has its own HIDPI_GL_CANVAS.
 */
class GAL_API HIDPI_GL_CANVAS : public wxGLCanvas
{
public:
    // wxGLCanvas constructor
    HIDPI_GL_CANVAS( const KIGFX::VC_SETTINGS& aSettings, wxWindow* aParent,
                     const wxGLAttributes& aGLAttribs, wxWindowID aId = wxID_ANY,
                     const wxPoint& aPos = wxDefaultPosition, const wxSize& aSize = wxDefaultSize,
                     long aStyle = 0, const wxString& aName = wxGLCanvasName,
                     const wxPalette& aPalette = wxNullPalette );

    /**
     * The canvas of a GAL: RGBA, double buffered, with a depth buffer, as large as its parent.
     *
     * Its mouse events go to the mouse listener and its paint events to the paint listener.
     */
    HIDPI_GL_CANVAS( const KIGFX::VC_SETTINGS& aSettings, TOOLKIT_WINDOW* aParent,
                     const wxString& aName );

    virtual wxSize GetNativePixelSize() const;

    /**
     * Convert the given point from client coordinates to native pixel coordinates.
     */
    wxPoint GetNativePosition( const wxPoint& aPoint ) const;

    /**
     * Get the current scale factor
     */
    double GetScaleFactor() const;

    void SetVcSettings( const KIGFX::VC_SETTINGS& aVcSettings ) { m_settings = aVcSettings; }

    /// The window that receives the mouse and gesture events of the canvas.
    void SetMouseListener( TOOLKIT_WINDOW* aMouseListener ) { m_mouseListener = aMouseListener; }

    /// The window asked to draw the canvas (by PostPaint()).
    void SetPaintListener( TOOLKIT_WINDOW* aPaintListener ) { m_paintListener = aPaintListener; }

    /// Ask the paint listener to draw the canvas.
    void PostPaint();

    /// Show a cursor of KiCad on the canvas.
    void SetCanvasCursor( KICURSOR aCursor, bool aHiDPI );

    /// Resize the canvas (in logical pixels).
    void ResizeCanvas( int aWidth, int aHeight );

    /// Show or hide the canvas; a shown canvas is raised.
    bool ShowCanvas( bool aShow );

    /// The canvas is shown on the screen, with a size.
    bool IsCanvasVisible() const;

    /**
     * Use the adaptive vertical synchronization if available.
     *
     * @return the swap interval set (-1 adaptive, 0 none, 1 vertical synchronization).
     */
    int SetupSwapInterval();

protected:
    ///< Current VIEW_CONTROLS settings.
    KIGFX::VC_SETTINGS m_settings;

private:
    void onPaint( wxPaintEvent& aEvent );

    /// Give a mouse event to the mouse listener.
    void skipMouseEvent( wxMouseEvent& aEvent );

    /// Give a gesture event to the mouse listener.
    void skipGestureEvent( wxGestureEvent& aEvent );

    void onSetNativeCursor( wxSetCursorEvent& aEvent );

    TOOLKIT_WINDOW* m_mouseListener = nullptr;
    TOOLKIT_WINDOW* m_paintListener = nullptr;

    /// wx cursor showing the current native cursor.
    WX_CURSOR_TYPE  m_currentwxCursor;
};

#endif // HIDPI_GL_CANVAS_H
