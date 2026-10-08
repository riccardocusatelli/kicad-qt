/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#pragma once

#include <math/vector2d.h>


/**
 * The state of the mouse with an event of a window: the GUI toolkits give their mouse events
 * to the toolkit-independent code (the 3D canvas) this way.
 */
struct MOUSE_STATE
{
    VECTOR2I m_position;                ///< In the client coordinates of the window

    bool     m_leftDown = false;        ///< The buttons pressed
    bool     m_middleDown = false;
    bool     m_rightDown = false;

    bool     m_dragging = false;        ///< The mouse moves with a button pressed

    bool     m_shift = false;           ///< The modifier keys pressed
    bool     m_control = false;
    bool     m_alt = false;

    int      m_wheelRotation = 0;       ///< 120 by notch of the wheel (a wheel event)
    bool     m_wheelHorizontal = false; ///< The wheel turns horizontally

    float    m_magnification = 0.0f;    ///< The magnification of a pinch (a magnify event)
};
