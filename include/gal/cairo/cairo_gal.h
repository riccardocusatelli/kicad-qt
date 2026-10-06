/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2017-2018 CERN
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

#ifndef CAIROGAL_H_
#define CAIROGAL_H_

#include <vector>

#include <gal/cairo/cairo_raster_gal.h>
#include <gal/cursors.h>
#include <wx/dcbuffer.h>

namespace KIGFX
{

/// The Cairo GAL in a wxWidgets window.
class GAL_API CAIRO_GAL : public CAIRO_RASTER_GAL, public wxWindow
{
public:
    /**
     * @param aParent is the wxWidgets immediate wxWindow parent of this object.
     * @param aMouseListener is the wxEvtHandler that should receive the mouse events, this
     *                       can be can be any wxWindow, but is often a wxFrame container.
     * @param aPaintListener is the wxEvtHandler that should receive the paint event.  This
     *                       can be any wxWindow, but is often a derived instance of this
     *                       class or a containing wxFrame.  The "paint event" here is a
     *                       wxCommandEvent holding EVT_GAL_REDRAW, as sent by PostPaint().
     *
     * @param aName is the name of this window for use by wxWindow::FindWindowByName().
     */
    CAIRO_GAL( GAL_DISPLAY_OPTIONS& aDisplayOptions, wxWindow* aParent,
               wxEvtHandler* aMouseListener = nullptr, wxEvtHandler* aPaintListener = nullptr,
               const wxString& aName = wxT( "CairoCanvas" ) );

    ///< @copydoc GAL::IsVisible()
    bool IsVisible() const override
    {
        return IsShownOnScreen() && !GetClientRect().IsEmpty();
    }

    void ResizeScreen( int aWidth, int aHeight ) override;

    bool Show( bool aShow ) override;

    /**
     * Post an event to m_paint_listener.
     *
     * A post is used so that the actual drawing function can use a device context type that
     * is not specific to the wxEVT_PAINT event, just by changing the PostPaint code.
     */
    void PostPaint( wxPaintEvent& aEvent );

    void SetMouseListener( wxEvtHandler* aMouseListener )
    {
        m_mouseListener = aMouseListener;
    }

    void SetPaintListener( wxEvtHandler* aPaintListener )
    {
        m_paintListener = aPaintListener;
    }

    /// @copydoc GAL::SetNativeCursorStyle()
    bool SetNativeCursorStyle( KICURSOR aCursor, bool aHiDPI ) override;

protected:
    /// Blit the raster to the window, with the cursor.
    void present() override;

    /// Blit cursor into the current screen.
    void blitCursor( wxMemoryDC& clientDC );

    // Event handlers
    /**
     * Paint event handler.
     *
     * @param aEvent is the paint event.
     */
    void onPaint( wxPaintEvent& aEvent );

    /**
     * Mouse event handler, forwards the event to the child.
     *
     * @param aEvent is the mouse event to be forwarded.
     */
    void skipMouseEvent( wxMouseEvent& aEvent );

    /**
     * Skip the gesture event to the parent.
     *
     * @param aEvent is the gesture event.
     */
    void skipGestureEvent( wxGestureEvent& aEvent );

    /**
     * Give the correct cursor image when the native widget asks for it.
     *
     * @param aEvent is the cursor event to plac the cursor into.
     */
    void onSetNativeCursor( wxSetCursorEvent& aEvent );

    ///< Cairo-specific update handlers
    bool updatedGalDisplayOptions( const GAL_DISPLAY_OPTIONS& aOptions ) override;

    // Variables related to wxWidgets
    wxWindow*           m_parentWindow;        ///< Parent window
    wxEvtHandler*       m_mouseListener;       ///< Mouse listener
    wxEvtHandler*       m_paintListener;       ///< Paint listener
    std::vector<unsigned char> m_wxOutput;     ///< wxImage compatible buffer

    WX_CURSOR_TYPE      m_currentwxCursor;     ///< wx cursor showing the current native cursor
};

} // namespace KIGFX

#endif  // CAIROGAL_H_
