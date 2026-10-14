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
 * @file ui.cpp
 * KIPLATFORM::UI on Qt (which handles the differences between the platforms itself).
 */

#include <kiplatform/ui.h>

#include <QAbstractScrollArea>
#include <QApplication>
#include <QCursor>
#include <QGuiApplication>
#include <QPalette>
#include <QScreen>
#include <QStyleHints>
#include <QWidget>
#include <QWindow>

#ifdef _WIN32
#include <windows.h>
#endif


namespace
{

/// The cursor was warped from this position to that one (where it may not be yet).
QPoint s_warpedFrom;
QPoint s_warpedTo;
bool   s_warped = false;

} // namespace


bool KIPLATFORM::UI::IsDarkTheme()
{
#if QT_VERSION >= QT_VERSION_CHECK( 6, 5, 0 )
    Qt::ColorScheme scheme = QGuiApplication::styleHints()->colorScheme();

    if( scheme != Qt::ColorScheme::Unknown )
        return scheme == Qt::ColorScheme::Dark;
#endif

    QColor bg = QGuiApplication::palette().color( QPalette::Window );

    // Weighted W3C formula
    double brightness = bg.redF() * 0.299 + bg.greenF() * 0.587 + bg.blueF() * 0.117;

    return brightness < 0.5;
}


void KIPLATFORM::UI::ForceFocus( QWidget* aWindow )
{
    if( !aWindow )
        return;

    aWindow->activateWindow();
    aWindow->setFocus( Qt::OtherFocusReason );
}


void KIPLATFORM::UI::GetInfoBarColours( QColor& aFGColour, QColor& aBGColour )
{
    QPalette palette = QApplication::palette();

    aBGColour = palette.color( QPalette::ToolTipBase );
    aFGColour = palette.color( QPalette::ToolTipText );
}


bool KIPLATFORM::UI::IsWindowActive( QWidget* aWindow )
{
    return aWindow && aWindow->isActiveWindow();
}


void KIPLATFORM::UI::ReparentWindow( QWidget* aWindow, QWidget* aParent )
{
    if( aWindow && aParent && aWindow->windowHandle() && aParent->window()->windowHandle() )
        aWindow->windowHandle()->setTransientParent( aParent->window()->windowHandle() );
}


double KIPLATFORM::UI::GetPixelScaleFactor( const QWidget* aWindow )
{
    return aWindow ? aWindow->devicePixelRatioF() : qApp->devicePixelRatio();
}


double KIPLATFORM::UI::GetContentScaleFactor( const QWidget* aWindow )
{
    return GetPixelScaleFactor( aWindow );
}


bool KIPLATFORM::UI::AllowIconsInMenus()
{
    return !QCoreApplication::testAttribute( Qt::AA_DontShowIconsInMenus );
}


wxPoint KIPLATFORM::UI::GetMousePosition()
{
    QPoint pos = QCursor::pos();

    // The platforms that can't move the cursor at once (Wayland) still give the position before
    // the warp: give the position where it was warped to
    if( s_warped )
    {
        if( pos == s_warpedFrom )
            pos = s_warpedTo;
        else
            s_warped = false;
    }

    return wxPoint( pos.x(), pos.y() );
}


bool KIPLATFORM::UI::WarpPointer( QWidget* aWindow, int aX, int aY )
{
    if( !aWindow )
        return false;

    // Wayland does not let the applications move the cursor
    if( QGuiApplication::platformName().startsWith( QLatin1String( "wayland" ) ) )
        return false;

    QPoint from = QCursor::pos();
    QPoint to = aWindow->mapToGlobal( QPoint( aX, aY ) );

    QCursor::setPos( aWindow->screen(), to );

    s_warpedFrom = from;
    s_warpedTo = to;
    s_warped = true;

    return true;
}


bool KIPLATFORM::UI::InfiniteDragPrepareWindow( QWidget* aWindow )
{
    // The cursor is warped back into the window when it leaves it, where it can be warped
    return !QGuiApplication::platformName().startsWith( QLatin1String( "wayland" ) );
}


void KIPLATFORM::UI::InfiniteDragReleaseWindow()
{
}


void KIPLATFORM::UI::EnsureVisible( QWidget* aWindow )
{
    if( !aWindow )
        return;

    QScreen* screen = aWindow->screen();

    if( !screen )
        return;

    QRect available = screen->availableGeometry();
    QRect frame = aWindow->frameGeometry();

    if( !available.intersects( frame ) )
        aWindow->move( available.topLeft() );
}


void KIPLATFORM::UI::SetFloatLevel( QWidget* aWindow )
{
    if( aWindow )
        aWindow->setWindowFlag( Qt::WindowStaysOnTopHint, true );
}


void KIPLATFORM::UI::CancelPendingScroll( QTreeView* aCtrl )
{
}


void KIPLATFORM::UI::RemoveShutdownBlockReason( QWidget* aWindow )
{
#ifdef _WIN32
    // Destroys any block reason that may have existed
    ShutdownBlockReasonDestroy( reinterpret_cast<HWND>( aWindow->winId() ) );
#endif
}


void KIPLATFORM::UI::SetShutdownBlockReason( QWidget* aWindow, const wxString& aReason )
{
#ifdef _WIN32
    // The message on the shutdown page on why it's being "blocked" (with the handling of
    // WM_QUERYENDSESSION, the closing of the frame); ShutdownBlockReasonCreate doesn't block by
    // itself
    HWND hwnd = reinterpret_cast<HWND>( aWindow->winId() );

    ShutdownBlockReasonDestroy( hwnd );
    ShutdownBlockReasonCreate( hwnd, aReason.wc_str() );
#endif
}


QColor KIPLATFORM::UI::GetDialogBGColour()
{
    return QApplication::palette().color( QPalette::Window );
}


wxSize KIPLATFORM::UI::GetUnobscuredSize( const QWidget* aWindow )
{
    if( !aWindow )
        return wxSize();

    if( const QAbstractScrollArea* area = qobject_cast<const QAbstractScrollArea*>( aWindow ) )
        return wxSize( area->viewport()->width(), area->viewport()->height() );

    return wxSize( aWindow->width(), aWindow->height() );
}
