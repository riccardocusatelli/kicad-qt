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

/**
 * @file hidpi_gl_canvas.h
 * HIDPI_GL_CANVAS of the Qt build: the interface of include/gal/hidpi_gl_canvas.h, on Qt.
 */

#ifndef HIDPI_GL_CANVAS_H
#define HIDPI_GL_CANVAS_H

#include <kicad_gl/kiglad.h> // Must be included first

#include <kiqt.h>
#include <QPointer>
#include <QWidget>

#include <view/view_controls.h>
#include <gal/gal.h>
#include <gal/kicursor.h>
#include <toolkit/toolkit_types.h>
#include <wx/gdicmn.h>

class QSurfaceFormat;
class QWindow;


/**
 * The OpenGL canvas of the Qt build, aware of the HiDPI scale.
 *
 * It holds an OpenGL window (QWindow, a native surface as wxGLCanvas): the contexts of the
 * GL_CONTEXT_MANAGER draw in its default framebuffer.  The mouse and keyboard events of the
 * window go to the mouse listener (the panel of the GAL), and the requests to draw to the
 * paint listener.
 */
class KIQT_API HIDPI_GL_CANVAS : public QWidget
{
public:
    /**
     * The canvas of a GAL, as large as its parent.
     *
     * @param aName is the name of the canvas (TOOLKIT::FindWindowByName()).
     */
    HIDPI_GL_CANVAS( const KIGFX::VC_SETTINGS& aSettings, TOOLKIT_WINDOW* aParent,
                     const wxString& aName );

    ~HIDPI_GL_CANVAS() override;

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

    /// The window that receives the mouse and keyboard events of the canvas.
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
     * The swap interval of the canvas (set in the format of its contexts).
     *
     * @return the swap interval (0 none, 1 vertical synchronization).
     */
    int SetupSwapInterval();

    /// Show what was drawn in the back buffer of the current context.
    void SwapBuffers();

    /// Draw the canvas again (from the event loop).
    void Refresh();

    /// The OpenGL window of the canvas: the surface of its contexts.
    QWindow* GetSurface() const { return m_window; }

    /// The format of the canvases and of their contexts (RGBA, double buffered, depth).
    static QSurfaceFormat SurfaceFormat();

protected:
    /// The events of the OpenGL window go to the listeners.
    bool eventFilter( QObject* aWatched, QEvent* aEvent ) override;

    ///< Current VIEW_CONTROLS settings.
    KIGFX::VC_SETTINGS m_settings;

private:
    QWindow*          m_window;
    QWidget*          m_container;
    QPointer<QWidget> m_mouseListener;
    QPointer<QWidget> m_paintListener;
};

#endif // HIDPI_GL_CANVAS_H
