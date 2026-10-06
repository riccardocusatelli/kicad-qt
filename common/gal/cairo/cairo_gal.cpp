/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2017-2018 CERN
 *
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * CairoGal - Graphics Abstraction Layer for Cairo
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

#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/image.h>

#include <gal/cairo/cairo_gal.h>
#include <math/vector2wx.h>

using namespace KIGFX;


CAIRO_GAL::CAIRO_GAL( GAL_DISPLAY_OPTIONS& aDisplayOptions, wxWindow* aParent,
                      wxEvtHandler* aMouseListener, wxEvtHandler* aPaintListener,
                      const wxString& aName ) :
        CAIRO_RASTER_GAL( aDisplayOptions ),
        wxWindow( aParent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxEXPAND, aName )
{
#ifdef _WIN32
    // need to fix broken cairo rendering on Windows with wx 3.3
    SetDoubleBuffered( false );
#endif

    m_parentWindow = aParent;
    m_mouseListener = aMouseListener;
    m_paintListener = aPaintListener;

    // Connect the native cursor handler
    Connect( wxEVT_SET_CURSOR, wxSetCursorEventHandler( CAIRO_GAL::onSetNativeCursor ), nullptr,
             this );

    // Connecting the event handlers
    Connect( wxEVT_PAINT, wxPaintEventHandler( CAIRO_GAL::onPaint ) );

    // Mouse events are skipped to the parent
    Connect( wxEVT_MOTION, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_LEFT_UP, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_MIDDLE_DOWN, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_MIDDLE_UP, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_MIDDLE_DCLICK, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_RIGHT_UP, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_RIGHT_DCLICK, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_AUX1_DOWN, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_AUX1_UP, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_AUX1_DCLICK, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_AUX2_DOWN, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_AUX2_UP, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_AUX2_DCLICK, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
    Connect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );

#if defined _WIN32 || defined _WIN64
    Connect( wxEVT_ENTER_WINDOW, wxMouseEventHandler( CAIRO_GAL::skipMouseEvent ) );
#endif

    Bind( wxEVT_GESTURE_ZOOM, &CAIRO_GAL::skipGestureEvent, this );
    Bind( wxEVT_GESTURE_PAN, &CAIRO_GAL::skipGestureEvent, this );

    SetSize( aParent->GetClientSize() );
    m_screenSize = ToVECTOR2I( aParent->GetClientSize() );

    // Allocate memory for pixel storage
    allocateBitmaps();
}


void CAIRO_GAL::present()
{
    // Translate the raw context data from the format stored by cairo into a format understood
    // by wxImage.
    int height = m_screenSize.y;
    int stride = m_stride;

    m_wxOutput.resize( static_cast<size_t>( m_bufferWidth ) * 3 * height );

    unsigned char* srcRow = m_bitmapBuffer;
    unsigned char* dst = m_wxOutput.data();

    for( int y = 0; y < height; y++ )
    {
        for( int x = 0; x < stride; x += 4 )
        {
            const unsigned char* src = srcRow + x;

#if defined( __BYTE_ORDER__ ) && ( __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__ )
            // XRGB
            dst[0] = src[1];
            dst[1] = src[2];
            dst[2] = src[3];
#else
            // BGRX
            dst[0] = src[2];
            dst[1] = src[1];
            dst[2] = src[0];
#endif

            dst += 3;
        }

        srcRow += stride;
    }

    wxImage    img( m_bufferWidth, m_screenSize.y, m_wxOutput.data(), true );
    wxBitmap   bmp( img );
    wxMemoryDC mdc( bmp );
    wxClientDC clientDC( this );

    // Now it is the time to blit the mouse cursor
    blitCursor( mdc );
    clientDC.Blit( 0, 0, m_screenSize.x, m_screenSize.y, &mdc, 0, 0, wxCOPY );
}


void CAIRO_GAL::PostPaint( wxPaintEvent& aEvent )
{
    // posts an event to m_paint_listener to ask for redraw the canvas.
    if( m_paintListener )
        wxPostEvent( m_paintListener, aEvent );
}


void CAIRO_GAL::ResizeScreen( int aWidth, int aHeight )
{
    CAIRO_RASTER_GAL::ResizeScreen( aWidth, aHeight );

    SetSize( wxSize( aWidth, aHeight ) );
}


bool CAIRO_GAL::Show( bool aShow )
{
    bool s = wxWindow::Show( aShow );

    if( aShow )
        wxWindow::Raise();

    return s;
}


void CAIRO_GAL::onPaint( wxPaintEvent& aEvent )
{
    // A wxPaintDC must be created in wxEVT_PAINT handlers. Without this, the system keeps
    // sending paint events because it thinks the window still needs to be painted, causing
    // high CPU usage in fallback mode (Cairo).
    wxPaintDC dc( this );
    PostPaint( aEvent );
}


void CAIRO_GAL::skipMouseEvent( wxMouseEvent& aEvent )
{
    // Post the mouse event to the event listener registered in constructor, if any
    if( m_mouseListener )
        wxPostEvent( m_mouseListener, aEvent );
}


void CAIRO_GAL::skipGestureEvent( wxGestureEvent& aEvent )
{
    // Post the gesture event to the event listener registered in constructor, if any
    if( m_mouseListener )
        wxPostEvent( m_mouseListener, aEvent );
}


bool CAIRO_GAL::updatedGalDisplayOptions( const GAL_DISPLAY_OPTIONS& aOptions )
{
    if( !CAIRO_RASTER_GAL::updatedGalDisplayOptions( aOptions ) )
        return false;

    Refresh();
    return true;
}


bool CAIRO_GAL::SetNativeCursorStyle( KICURSOR aCursor, bool aHiDPI )
{
    // Store the current cursor type and get the wx cursor for it
    if( !GAL::SetNativeCursorStyle( aCursor, aHiDPI ) )
        return false;

    m_currentwxCursor = CURSOR_STORE::GetCursor( m_currentNativeCursor, aHiDPI );

#if wxCHECK_VERSION( 3, 3, 0 )
    wxWindow::SetCursorBundle( m_currentwxCursor );
#else
    wxWindow::SetCursor( m_currentwxCursor );
#endif

    return true;
}


void CAIRO_GAL::onSetNativeCursor( wxSetCursorEvent& aEvent )
{
#if wxCHECK_VERSION( 3, 3, 0 )
    aEvent.SetCursor( m_currentwxCursor.GetCursorFor( this ) );
#else
    aEvent.SetCursor( m_currentwxCursor );
#endif
}


void CAIRO_GAL::blitCursor( wxMemoryDC& clientDC )
{
    if( !IsCursorEnabled() )
        return;

    VECTOR2D      p = ToScreen( m_cursorPosition );
    const COLOR4D cColor = getCursorColor();

    wxColour color( cColor.r * cColor.a * 255, cColor.g * cColor.a * 255, cColor.b * cColor.a * 255,
                    255 );
    clientDC.SetPen( wxPen( color ) );

    if( m_crossHairMode == CROSS_HAIR_MODE::FULLSCREEN_CROSS )
    {
        clientDC.DrawLine( 0, p.y, m_screenSize.x, p.y );
        clientDC.DrawLine( p.x, 0, p.x, m_screenSize.y );
    }
    else if( m_crossHairMode == CROSS_HAIR_MODE::FULLSCREEN_DIAGONAL )
    {
        // Oversized but that's ok
        int diagonalSize = m_screenSize.x + m_screenSize.y;
        clientDC.DrawLine( p.x - diagonalSize, p.y - diagonalSize,
                           p.x + diagonalSize, p.y + diagonalSize );
        clientDC.DrawLine( p.x - diagonalSize, p.y + diagonalSize,
                           p.x + diagonalSize, p.y - diagonalSize );
    }
    else
    {
        const int cursorSize = 80;
        clientDC.DrawLine( p.x - cursorSize / 2, p.y, p.x + cursorSize / 2, p.y );
        clientDC.DrawLine( p.x, p.y - cursorSize / 2, p.x, p.y + cursorSize / 2 );
    }
}
