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
 * tools) names in its interfaces: the Qt version of include/toolkit/toolkit_types.h.
 */

class QWidget;
class QEvent;

/// A window (a widget) of the user interface.
using TOOLKIT_WINDOW = QWidget;

class wxWidgetArgCompat;

/// The parent of a window that the toolkit-independent code makes: a widget of Qt or a window
/// of the wxWidgets compatibility layer.
using TOOLKIT_WINDOW_ARG = wxWidgetArgCompat;

/// An event of the user interface (an input event, given to the tool dispatcher).
using TOOLKIT_EVENT = QEvent;

class QComboBox;

/// A choice box (a drop-down list of strings, as the ones of the toolbars).
using TOOLKIT_CHOICE = QComboBox;

class QPixmap;
class wxBitmapBundle;

/// An image of the user interface (an icon at one size, a color swatch).
using TOOLKIT_BITMAP = QPixmap;

/// An icon of the user interface, with its images at the different scales of the screens: the
/// wxBitmapBundle of the compatibility layer (a QIcon with the API of wxWidgets, wx/bmpbndl.h).
using TOOLKIT_BITMAP_BUNDLE = wxBitmapBundle;

class wxAppConsole;

/// The application object (the KIQT_APP of the program: the wxApp API of the code of KiCad).
using TOOLKIT_APP = wxAppConsole;
