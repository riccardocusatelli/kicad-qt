/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
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

#include <gal/hidpi_gl_3D_canvas.h>
#include <settings/common_settings.h>
#include <wx/defs.h>        // WXK_ key codes

const float HIDPI_GL_3D_CANVAS::m_delta_move_step_factor = 0.7f;


void HIDPI_GL_3D_CANVAS::OnMouseMoveCamera( const MOUSE_STATE& aMouse )
{
    if( m_camera_is_moving )
        return;

    const wxSize&  nativeWinSize = GetNativePixelSize();
    const wxPoint& nativePosition = GetNativePosition( wxPoint( aMouse.m_position.x,
                                                                aMouse.m_position.y ) );

    m_camera.SetCurWindowSize( nativeWinSize );

    if( aMouse.m_dragging )
    {
        bool handled = false;

        if( aMouse.m_leftDown )
        {
            m_camera.Drag( nativePosition );
            handled = true;
        }
        else if( ( aMouse.m_middleDown && m_settings.m_dragMiddle == MOUSE_DRAG_ACTION::PAN )
                 || ( aMouse.m_rightDown && m_settings.m_dragRight == MOUSE_DRAG_ACTION::PAN ) )
        {
            m_camera.Pan( nativePosition );
            handled = true;
        }

        if( handled )
        {
            m_mouse_is_moving = true;
            m_mouse_was_moved = true;
        }
    }

    m_camera.SetCurMousePosition( nativePosition );
}

void HIDPI_GL_3D_CANVAS::OnMouseWheelCamera( const MOUSE_STATE& aMouse, bool aPan )
{
    bool mouseActivity = false;

    if( m_camera_is_moving )
        return;

    // Honor the "Pan left/right with horizontal movement" setting for native horizontal wheel
    if( aMouse.m_wheelHorizontal && !m_settings.m_horizontalPan )
        return;

    // Pick the modifier, if any.  Shift beats control beats alt, we don't support more than one.
    int modifiers =
            aMouse.m_shift ? WXK_SHIFT : ( aMouse.m_control ? WXK_CONTROL : ( aMouse.m_alt ? WXK_ALT : 0 ) );

    float delta_move = m_delta_move_step_factor * m_camera.GetZoom();
    float horizontalSign = m_settings.m_scrollReversePanH ? -1 : 1;
    float zoomSign = m_settings.m_scrollReverseZoom ? -1 : 1;

    if( aPan )
        delta_move *= 0.01f * aMouse.m_wheelRotation;
    else if( aMouse.m_wheelRotation < 0 )
        delta_move = -delta_move;

    // mousewheel_panning enabled:
    //      wheel           -> pan;
    //      wheel + shift   -> horizontal scrolling;
    //      wheel + ctrl    -> zooming;
    // mousewheel_panning disabled:
    //      wheel + shift   -> vertical scrolling;
    //      wheel + ctrl    -> horizontal scrolling;
    //      wheel           -> zooming.

    if( aPan && modifiers != m_settings.m_scrollModifierZoom )
    {
        if( aMouse.m_wheelHorizontal || modifiers == m_settings.m_scrollModifierPanH )
            m_camera.Pan( SFVEC3F( -delta_move, 0.0f, 0.0f ) );
        else
            m_camera.Pan( SFVEC3F( 0.0f, -delta_move, 0.0f ) );

        mouseActivity = true;
    }
    else if( modifiers == m_settings.m_scrollModifierPanV && !aPan )
    {
        m_camera.Pan( SFVEC3F( 0.0f, -delta_move, 0.0f ) );
        mouseActivity = true;
    }
    else if( modifiers == m_settings.m_scrollModifierPanH && !aPan )
    {
        m_camera.Pan( SFVEC3F( delta_move * horizontalSign, 0.0f, 0.0f ) );
        mouseActivity = true;
    }
    else
    {
        mouseActivity = m_camera.Zoom( ( aMouse.m_wheelRotation * zoomSign ) > 0 ? 1.1f : 1 / 1.1f );
    }

    // If it results on a camera movement
    if( mouseActivity )
    {
        m_mouse_is_moving = true;
        m_mouse_was_moved = true;
    }

    // Update the cursor current mouse position on the camera
    m_camera.SetCurMousePosition( GetNativePosition( wxPoint( aMouse.m_position.x,
                                                              aMouse.m_position.y ) ) );
}
