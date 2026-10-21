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
 * @file toolkit_window_arg.h
 * The window given to the functions of KiCad that show a dialog on it (confirm.h), and the
 * parent of the canvases made by the toolkit-independent code: a window of wxWidgets.  The Qt build has its own version of this header, where it is a QWidget* or a
 * wxWindow* of its compatibility layer.
 */

class wxWindow;

/// A window given as the parent of a dialog.
using TOOLKIT_WINDOW_ARG = wxWindow*;
