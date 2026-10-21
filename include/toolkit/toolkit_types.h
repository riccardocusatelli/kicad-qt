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
 * @file toolkit_types.h
 * The types of the GUI toolkit that the toolkit-independent code (the tool framework, the
 * tools) names in its interfaces.
 *
 * This is the wxWidgets version; the Qt build has its own version of this header
 * (qt/include/toolkit/toolkit_types.h), earlier in its include path.
 */

class wxWindow;
class wxEvent;

/// A window (a widget) of the user interface.
using TOOLKIT_WINDOW = wxWindow;

/// An event of the user interface (an input event, given to the tool dispatcher).
using TOOLKIT_EVENT = wxEvent;

class wxChoice;

/// A choice box (a drop-down list of strings, as the ones of the toolbars).
using TOOLKIT_CHOICE = wxChoice;

class wxBitmap;
class wxBitmapBundle;

/// An image of the user interface (an icon at one size, a color swatch).
using TOOLKIT_BITMAP = wxBitmap;

/// An icon of the user interface, with its images at the different scales of the screens.
using TOOLKIT_BITMAP_BUNDLE = wxBitmapBundle;

#include <wx/defs.h>

#ifdef KIWX
// The wxBase of KiCad (the headless tools of the Qt build) has no GUI application: its wxApp is
// a typedef of wxAppConsole
class wxAppConsole;

/// The application object of the GUI toolkit.
using TOOLKIT_APP = wxAppConsole;
#else
class wxApp;

/// The application object of the GUI toolkit.
using TOOLKIT_APP = wxApp;
#endif
