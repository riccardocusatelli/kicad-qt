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


/**
 * @file ui.h
 * KIPLATFORM::UI of the Qt build: the interface of libs/kiplatform/include/kiplatform/ui.h for
 * the windows of Qt (QWidget).
 */

#ifndef KIPLATFORM_UI_H_
#define KIPLATFORM_UI_H_

#include <wx/gdicmn.h>      // wxPoint, wxSize (kiwx)
#include <wx/string.h>      // wxString (kiwx)

#include <kiqt.h>

class QColor;
class QTreeView;

class QWidget;

namespace KIPLATFORM
{
    // Implemented in libkiqt
    namespace KIQT_API UI
    {
        /**
         * Determine if the desktop interface is currently using a dark theme or a light theme.
         *
         * @return true if a dark theme is being used.
         */
        bool IsDarkTheme();

        /**
         * Pass the current focus to the window, and activate its top level window.
         *
         * @param aWindow is the window to pass focus to
         */
        void ForceFocus( QWidget* aWindow );

        /**
         * Return the background and foreground colors for info bars and the popups of
         * information in the current UI theme (the info colors of the desktop).
         */
        void GetInfoBarColours( QColor& aFGColour, QColor& aBGColour );

        /**
         * Check to see if the given window is the currently active window (e.g. the window
         * in the foreground the user is interacting with).
         *
         * @param aWindow is the window to check
         */
        bool IsWindowActive( QWidget* aWindow );

        /**
         * Make a window stay above its parent window (the windows of KiCad that float over a
         * frame, as the quasi-modal dialogs).
         */
        void ReparentWindow( QWidget* aWindow, QWidget* aParent );

        /**
         * Tries to determine the pixel scaling factor currently in use for the window.
         * @param aWindow pointer to the window to check
         * @return Pixel scale factor in use
         */
        double GetPixelScaleFactor( const QWidget* aWindow );

        /**
         * Tries to determine the content scaling factor currently in use for the window.
         * The content scaling factor is typically settable by the user and may differ from the
         * pixel scaling factor.
         */
        double GetContentScaleFactor( const QWidget* aWindow );

        /**
         * If the user has disabled icons system-wide, we check that here
         */
        bool AllowIconsInMenus();

        /**
         * Returns the mouse position in screen coordinates.
         * If we've just warped the cursor, returns the new coordinates.
         */
        wxPoint GetMousePosition();

        /**
         * Move the mouse cursor to a specific position relative to the window
         * @param aWindow Window in which to position to mouse cursor
         * @param aX destination x position
         * @param aY destination y position
         * @return true if the warp was successful
         */
        bool WarpPointer( QWidget* aWindow, int aX, int aY );

        /**
         * Prepare a window for the panning by dragging the mouse beyond its edges (the cursor
         * is warped back into the window).
         *
         * @param aWindow Window in which to position to mouse cursor
         * @return true if infinite panning is supported
         */
        bool InfiniteDragPrepareWindow( QWidget* aWindow );

        /**
         * Allows the cursor to freely move again after a drag (see `InfiniteDragPrepareWindow`).
         */
        void InfiniteDragReleaseWindow();

        /**
         * Ensure that a window is visible on the screen.
         *
         * @param aWindow window to make visible
         */
        void EnsureVisible( QWidget* aWindow );

        /**
         * Intended to set the floating window level in macOS on a window
         */
        void SetFloatLevel( QWidget* aWindow );

        /**
         * Drop a scroll of a tree view pending in the toolkit, before its rows are freed (a
         * GtkTreeView of wxWidgets on GTK).  Nothing to do with Qt: the views of Qt scroll to
         * their indexes, invalidated with the model.
         */
        void CancelPendingScroll( QTreeView* aCtrl );

        /**
         * Sets the block reason why the window/application is preventing OS shutdown.
         * This should be set far ahead of any close event.
         *
         * This is mainly intended for Windows platforms where this is a native feature
         * (see KIPLATFORM::APP::SupportsShutdownBlockReason()).
         *
         * @param aWindow that will have a shutdown blocker message
         * @param aReason to display why the shutdown block is occurring
         */
        void SetShutdownBlockReason( QWidget* aWindow, const wxString& aReason );

        /**
         * Removes any shutdown block reason set
         *
         * @param aWindow that has a shutdown block reason set
         */
        void RemoveShutdownBlockReason( QWidget* aWindow );
    }
}

#endif // KIPLATFORM_UI_H_
