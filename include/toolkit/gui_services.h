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
#include <toolkit/toolkit_types.h>

#include <memory>

class wxString;
class wxSize;

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

/**
 * Find a window by its name (the frame name of a frame).
 *
 * @param aParent is the window searched (with its children), or nullptr for all the windows.
 * @return the window or nullptr.
 */
APIVISIBLE TOOLKIT_WINDOW* FindWindowByName( const wxString& aName,
                                             TOOLKIT_WINDOW* aParent = nullptr );

/// The window with the keyboard focus, or nullptr.
APIVISIBLE TOOLKIT_WINDOW* FindFocus();

/// The parent of @a aWindow, or nullptr.
APIVISIBLE TOOLKIT_WINDOW* GetParent( const TOOLKIT_WINDOW* aWindow );

/// The top level window (frame, dialog) of @a aWindow, or nullptr.
APIVISIBLE TOOLKIT_WINDOW* GetTopLevelParent( TOOLKIT_WINDOW* aWindow );

/// The top level window of the focus, or else the main window of the application (or nullptr).
APIVISIBLE TOOLKIT_WINDOW* GetActiveTopWindow();

/// @a aWindow and its parents are shown.
APIVISIBLE bool IsShownOnScreen( const TOOLKIT_WINDOW* aWindow );

/// Close a window (a modeless dialog, a frame), even if it would ask to stay open.
APIVISIBLE void CloseWindow( TOOLKIT_WINDOW* aWindow );

/// Let @a aWindow process its events, or not (a canvas while its frame closes).
APIVISIBLE void SetEventsEnabled( TOOLKIT_WINDOW* aWindow, bool aEnabled );

/// Lay out the children of @a aWindow again (after a change of their sizes or of the fonts).
APIVISIBLE void Relayout( TOOLKIT_WINDOW* aWindow );

/// Show a window that needs the attention of the user (with the effect of the toolkit for a
/// window that appears).
APIVISIBLE void ShowWindow( TOOLKIT_WINDOW* aWindow );

/**
 * A hidden top level window of @a aSize, child of @a aParent: the parent of a window drawn
 * off screen (a canvas rendering an image).
 */
APIVISIBLE std::unique_ptr<TOOLKIT_WINDOW> CreateHiddenWindow( TOOLKIT_WINDOW* aParent,
                                                               const wxSize& aSize );

} // namespace TOOLKIT
