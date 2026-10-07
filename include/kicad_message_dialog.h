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
 * @file kicad_message_dialog.h
 * The message dialogs of wxWidgets that KiCad uses.
 */

#include <wx/msgdlg.h>
#include <wx/richmsgdlg.h>

// On Windows, to have the best dark mode experience we need to sub in the generic dialogs
// as wx can't retheme the native dialogs
#if defined( _WIN32 ) && wxCHECK_VERSION( 3, 3, 0 )
#define KICAD_MESSAGE_DIALOG_BASE wxGenericMessageDialog
#define KICAD_RICH_MESSAGE_DIALOG_BASE wxGenericRichMessageDialog
// incase we in the future declare our own message dialog class
#define KICAD_MESSAGE_DIALOG KICAD_MESSAGE_DIALOG_BASE
#else
#define KICAD_MESSAGE_DIALOG_BASE wxMessageDialog
#define KICAD_RICH_MESSAGE_DIALOG_BASE wxRichMessageDialog
#define KICAD_MESSAGE_DIALOG KICAD_MESSAGE_DIALOG_BASE
#endif
