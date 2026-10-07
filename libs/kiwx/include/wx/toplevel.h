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
 * @file toplevel.h
 *
 * The styles of the top level windows of wxWidgets (wx/toplevel.h, wx/frame.h and wx/dialog.h):
 * the frames and dialogs of KiCad pass them to their base classes, which give them to the GUI
 * toolkit (the Qt frames map them to window flags).
 */

#include <wx/defs.h>

#define wxSTAY_ON_TOP           0x8000
#define wxICONIZE               0x4000
#define wxMINIMIZE              wxICONIZE
#define wxMAXIMIZE              0x2000
#define wxCLOSE_BOX             0x1000
#define wxSYSTEM_MENU           0x0800
#define wxMINIMIZE_BOX          0x0400
#define wxMAXIMIZE_BOX          0x0200
#define wxTINY_CAPTION          0x0080
#define wxRESIZE_BORDER         0x0040

#define wxDEFAULT_FRAME_STYLE \
    ( wxSYSTEM_MENU | wxRESIZE_BORDER | wxMINIMIZE_BOX | wxMAXIMIZE_BOX | wxCLOSE_BOX | \
      wxCAPTION | wxCLIP_CHILDREN )

#define wxFRAME_NO_TASKBAR      0x0002
#define wxFRAME_TOOL_WINDOW     0x0004
#define wxFRAME_FLOAT_ON_PARENT 0x0008
#define wxFRAME_SHAPED          0x0010

#define wxDIALOG_NO_PARENT      0x00000020
#define wxDEFAULT_DIALOG_STYLE  ( wxCAPTION | wxSYSTEM_MENU | wxCLOSE_BOX )

