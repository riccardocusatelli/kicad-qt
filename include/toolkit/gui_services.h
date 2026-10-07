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

/**
 * @file gui_services.h
 * Services of the GUI toolkit used by the toolkit-independent code (the tool framework, the
 * tools): the event loop, the keyboard, the bell.
 *
 * Each GUI build has its own implementation (common/toolkit/gui_services_wx.cpp,
 * qt/common/toolkit/gui_services_qt.cpp).
 */
#include <import_export.h>

namespace TOOLKIT
{

/// Ring the bell of the system (an action that can't be done).
APIVISIBLE void Bell();

/**
 * @return true if a key is pressed.
 * @param aKeyCode is a key code of KiCad: a character, or a WXK_ code (WXK_SHIFT, WXK_F1...).
 */
APIVISIBLE bool IsKeyDown( int aKeyCode );

/// Handle the pending events of the event loop (the tools wait for an action this way).
APIVISIBLE void YieldEvents();

/// Handle the events that are queued for later processing.
APIVISIBLE void ProcessPendingEvents();

/// @return true while the event loop is handling events from YieldEvents() (no popup menu
///         may be opened then).
APIVISIBLE bool IsYielding();

} // namespace TOOLKIT
