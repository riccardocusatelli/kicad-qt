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

#include <gal/hidpi_gl_canvas.h>

#include <dpi_scaling.h>
#include <kicad_gl/gl_utils.h>


/// The attributes of the canvases of the GAL.
static wxGLAttributes galGLAttribs()
{
    wxGLAttributes attribs;
    attribs.RGBA().DoubleBuffer().Depth( 8 ).EndList();

    return attribs;
}


HIDPI_GL_CANVAS::HIDPI_GL_CANVAS( const KIGFX::VC_SETTINGS& aSettings, wxWindow* aParent,
                                  const wxGLAttributes& aGLAttribs, wxWindowID aId,
                                  const wxPoint& aPos, const wxSize& aSize, long aStyle,
                                  const wxString& aName, const wxPalette& aPalette ) :
        wxGLCanvas( aParent, aGLAttribs, aId, aPos, aSize, aStyle, aName, aPalette ),
        m_settings( aSettings )
{
}


HIDPI_GL_CANVAS::HIDPI_GL_CANVAS( const KIGFX::VC_SETTINGS& aSettings, TOOLKIT_WINDOW* aParent,
                                  const wxString& aName ) :
        wxGLCanvas( aParent, galGLAttribs(), wxID_ANY, wxDefaultPosition, wxDefaultSize,
                    wxEXPAND, aName ),
        m_settings( aSettings )
{
    // Connect the native cursor handler
    Connect( wxEVT_SET_CURSOR, wxSetCursorEventHandler( HIDPI_GL_CANVAS::onSetNativeCursor ),
             nullptr, this );

    // Connecting the event handlers
    Connect( wxEVT_PAINT, wxPaintEventHandler( HIDPI_GL_CANVAS::onPaint ) );

    // Mouse events are skipped to the parent
    Connect( wxEVT_MOTION, wxMouseEventHandler( HIDPI_GL_CANVAS::skipMouseEvent ) );
    Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( HIDPI_GL_CANVAS::skipMouseEvent ) );
    Connect( wxEVT_LEFT_UP, wxMouseEventHandler( HIDPI_GL_CANVAS::skipMouseEvent ) );
    Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( HIDPI_GL_CANVAS::skipMouseEvent ) );
    Connect( wxEVT_MIDDLE_DOWN, wxMouseEventHandler( HIDPI_GL_CANVAS::skipMouseEvent ) );
    Connect( wxEVT_MIDDLE_UP, wxMouseEventHandler( HIDPI_GL_CANVAS::skipMouseEvent ) );
    Connect( wxEVT_MIDDLE_DCLICK, wxMouseEventHandler( HIDPI_GL_CANVAS::skipMouseEvent ) );
    Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( HIDPI_GL_CANVAS::skipMouseEvent ) );
    Connect( wxEVT_RIGHT_UP, wxMouseEventHandler( HIDPI_GL_CANVAS::skipMouseEvent ) );
    Connect( wxEVT_RIGHT_DCLICK, wxMouseEventHandler( HIDPI_GL_CANVAS::skipMouseEvent ) );
    Connect( wxEVT_AUX1_DOWN, wxMouseEventHandler( HIDPI_GL_CANVAS::skipMouseEvent ) );
    Connect( wxEVT_AUX1_UP, wxMouseEventHandler( HIDPI_GL_CANVAS::skipMouseEvent ) );
    Connect( wxEVT_AUX1_DCLICK, wxMouseEventHandler( HIDPI_GL_CANVAS::skipMouseEvent ) );
    Connect( wxEVT_AUX2_DOWN, wxMouseEventHandler( HIDPI_GL_CANVAS::skipMouseEvent ) );
    Connect( wxEVT_AUX2_UP, wxMouseEventHandler( HIDPI_GL_CANVAS::skipMouseEvent ) );
    Connect( wxEVT_AUX2_DCLICK, wxMouseEventHandler( HIDPI_GL_CANVAS::skipMouseEvent ) );
    Connect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( HIDPI_GL_CANVAS::skipMouseEvent ) );
    Connect( wxEVT_MAGNIFY, wxMouseEventHandler( HIDPI_GL_CANVAS::skipMouseEvent ) );

#if defined _WIN32 || defined _WIN64
    Connect( wxEVT_ENTER_WINDOW, wxMouseEventHandler( HIDPI_GL_CANVAS::skipMouseEvent ) );
#endif

    Bind( wxEVT_GESTURE_ZOOM, &HIDPI_GL_CANVAS::skipGestureEvent, this );
    Bind( wxEVT_GESTURE_PAN, &HIDPI_GL_CANVAS::skipGestureEvent, this );

    SetSize( aParent->GetClientSize() );
}


wxSize HIDPI_GL_CANVAS::GetNativePixelSize() const
{
    wxSize size = wxGLCanvas::GetClientSize();

    const double scaleFactor = GetScaleFactor();
    size.x *= scaleFactor;
    size.y *= scaleFactor;

    return size;
}


wxPoint HIDPI_GL_CANVAS::GetNativePosition( const wxPoint& aPoint ) const
{
    wxPoint nativePoint = aPoint;

    const double scaleFactor = GetScaleFactor();
    nativePoint.x *= scaleFactor;
    nativePoint.y *= scaleFactor;

    return nativePoint;
}


double HIDPI_GL_CANVAS::GetScaleFactor() const
{
    return GetContentScaleFactor();
}

void HIDPI_GL_CANVAS::PostPaint()
{
    // The paint listener gets a paint event, and draws the canvas
    if( m_paintListener )
        m_paintListener->Refresh();
}


void HIDPI_GL_CANVAS::onPaint( wxPaintEvent& aEvent )
{
    // posts an event to m_paint_listener to ask for redraw the canvas.
    if( m_paintListener )
        wxPostEvent( m_paintListener, aEvent );
}


void HIDPI_GL_CANVAS::skipMouseEvent( wxMouseEvent& aEvent )
{
    // Post the mouse event to the event listener registered in constructor, if any
    if( m_mouseListener )
        wxPostEvent( m_mouseListener, aEvent );
}


void HIDPI_GL_CANVAS::skipGestureEvent( wxGestureEvent& aEvent )
{
    // Post the gesture event to the event listener registered in constructor, if any
    if( m_mouseListener )
        wxPostEvent( m_mouseListener, aEvent );
}


void HIDPI_GL_CANVAS::SetCanvasCursor( KICURSOR aCursor, bool aHiDPI )
{
    m_currentwxCursor = CURSOR_STORE::GetCursor( aCursor, aHiDPI );

#if wxCHECK_VERSION( 3, 3, 0 )
    wxWindow::SetCursorBundle( m_currentwxCursor );
#else
    wxWindow::SetCursor( m_currentwxCursor );
#endif
}


void HIDPI_GL_CANVAS::onSetNativeCursor( wxSetCursorEvent& aEvent )
{
#if wxCHECK_VERSION( 3, 3, 0 )
    aEvent.SetCursor( m_currentwxCursor.GetCursorFor( this ) );
#else
    aEvent.SetCursor( m_currentwxCursor );
#endif
}


void HIDPI_GL_CANVAS::ResizeCanvas( int aWidth, int aHeight )
{
    wxGLCanvas::SetSize( aWidth, aHeight );
}


bool HIDPI_GL_CANVAS::ShowCanvas( bool aShow )
{
    bool s = wxGLCanvas::Show( aShow );

    if( aShow )
        wxGLCanvas::Raise();

    return s;
}


bool HIDPI_GL_CANVAS::IsCanvasVisible() const
{
    return IsShownOnScreen() && !GetClientRect().IsEmpty();
}


int HIDPI_GL_CANVAS::SetupSwapInterval()
{
#if wxCHECK_VERSION( 3, 3, 3 )
    wxGLCanvas::SetSwapInterval( -1 );
    return wxGLCanvas::GetSwapInterval();
#else
    return GL_UTILS::SetSwapInterval( this, -1 );
#endif
}
