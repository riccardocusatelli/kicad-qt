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
#include <wx/kiwx_window.h>      // wxWidgetArgCompat

class QColor;
class QTreeView;

class QWidget;

namespace KIPLATFORM
{
    // Implemented in libkiqt
    namespace UI
    {
        /**
         * Determine if the desktop interface is currently using a dark theme or a light theme.
         *
         * @return true if a dark theme is being used.
         */
        KIQT_API bool IsDarkTheme();

        /**
         * Pass the current focus to the window, and activate its top level window.
         *
         * @param aWindow is the window to pass focus to
         */
        KIQT_API void ForceFocus( QWidget* aWindow );

        /**
         * Let a file dialog show the network file systems (GTK): the dialogs of Qt show them.
         */
        inline void AllowNetworkFileSystems( const void* aDialog ) {}

        /**
         * Return the background and foreground colors for info bars and the popups of
         * information in the current UI theme (the info colors of the desktop).
         */
        KIQT_API void GetInfoBarColours( QColor& aFGColour, QColor& aBGColour );

        /**
         * Check to see if the given window is the currently active window (e.g. the window
         * in the foreground the user is interacting with).
         *
         * @param aWindow is the window to check
         */
        KIQT_API bool IsWindowActive( QWidget* aWindow );

        /**
         * Make a window stay above its parent window (the windows of KiCad that float over a
         * frame, as the quasi-modal dialogs).
         */
        KIQT_API void ReparentWindow( QWidget* aWindow, wxWidgetArgCompat aParent );

        /**
         * Tries to determine the pixel scaling factor currently in use for the window.
         * @param aWindow pointer to the window to check
         * @return Pixel scale factor in use
         */
        KIQT_API double GetPixelScaleFactor( const QWidget* aWindow );

        /**
         * Tries to determine the content scaling factor currently in use for the window.
         * The content scaling factor is typically settable by the user and may differ from the
         * pixel scaling factor.
         */
        KIQT_API double GetContentScaleFactor( const QWidget* aWindow );

        /**
         * If the user has disabled icons system-wide, we check that here
         */
        KIQT_API bool AllowIconsInMenus();

        /**
         * Returns the mouse position in screen coordinates.
         * If we've just warped the cursor, returns the new coordinates.
         */
        KIQT_API wxPoint GetMousePosition();

        /**
         * Move the mouse cursor to a specific position relative to the window
         * @param aWindow Window in which to position to mouse cursor
         * @param aX destination x position
         * @param aY destination y position
         * @return true if the warp was successful
         */
        KIQT_API bool WarpPointer( QWidget* aWindow, int aX, int aY );

        /**
         * Prepare a window for the panning by dragging the mouse beyond its edges (the cursor
         * is warped back into the window).
         *
         * @param aWindow Window in which to position to mouse cursor
         * @return true if infinite panning is supported
         */
        KIQT_API bool InfiniteDragPrepareWindow( QWidget* aWindow );

        /**
         * Allows the cursor to freely move again after a drag (see `InfiniteDragPrepareWindow`).
         */
        KIQT_API void InfiniteDragReleaseWindow();

        /**
         * Ensure that a window is visible on the screen.
         *
         * @param aWindow window to make visible
         */
        KIQT_API void EnsureVisible( QWidget* aWindow );

        /**
         * Intended to set the floating window level in macOS on a window
         */
        KIQT_API void SetFloatLevel( QWidget* aWindow );

        /**
         * Drop a scroll of a tree view pending in the toolkit, before its rows are freed (a
         * GtkTreeView of wxWidgets on GTK).  Nothing to do with Qt: the views of Qt scroll to
         * their indexes, invalidated with the model.
         */
        KIQT_API void CancelPendingScroll( QTreeView* aCtrl );

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
        KIQT_API void SetShutdownBlockReason( QWidget* aWindow, const wxString& aReason );

        /**
         * Removes any shutdown block reason set
         *
         * @param aWindow that has a shutdown block reason set
         */
        KIQT_API void RemoveShutdownBlockReason( QWidget* aWindow );

        /// The background color of the dialogs.
        KIQT_API QColor GetDialogBGColour();

        /// The size of the visible part of a window (the viewport of a scroll area).
        KIQT_API wxSize GetUnobscuredSize( const QWidget* aWindow );

        /*
         * The workarounds of the toolkits of wxWidgets (GTK, macOS, MSW): the widgets of Qt need
         * none of them.
         */
        inline void SetOverlayScrolling( const QWidget* aWindow, bool aOverlay ) {}
        inline void FixupCancelButtonCmdKeyCollision( QWidget* aWindow ) {}
        inline void LargeChoiceBoxHack( QWidget* aChoice ) {}
        inline void EllipsizeChoiceBox( QWidget* aChoice ) {}
        inline void ImmControl( QWidget* aWindow, bool aEnable ) {}
        inline void ImeNotifyCancelComposition( QWidget* aWindow ) {}
        inline void ReparentModal( QWidget* aWindow ) {}
        inline void ReleaseChildWindow( QWidget* aWindow ) {}
        inline bool IsStockCursorOk( int aCursor ) { return true; }
    }
}

#endif // KIPLATFORM_UI_H_
