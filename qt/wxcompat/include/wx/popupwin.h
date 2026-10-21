/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file popupwin.h
 * The popup windows of the wxWidgets compatibility layer (wxPopupWindow,
 * wxPopupTransientWindow): windows without frame over their parent, shown without taking the
 * focus; a transient one closes when the user clicks outside it.
 */

#pragma once

#include <wx/kiwx_gui.h>

#include <QHideEvent>


class wxPopupWindow : public wxWindowCompat<QWidget>
{
public:
    explicit wxPopupWindow( wxWidgetArgCompat aParent = nullptr, int aFlags = 0 ) :
            wxWindowCompat<QWidget>( aParent )
    {
        setWindowFlags( Qt::ToolTip | Qt::FramelessWindowHint );
        setAttribute( Qt::WA_ShowWithoutActivating );
    }

    /// Place the popup next to a window at @a aOrigin of size @a aSize: under it.
    virtual void Position( const wxPoint& aOrigin, const wxSize& aSize )
    {
        move( aOrigin.x, aOrigin.y + aSize.y );
    }
};


class wxPopupTransientWindow : public wxPopupWindow
{
public:
    explicit wxPopupTransientWindow( wxWidgetArgCompat aParent = nullptr, int aFlags = 0 ) :
            wxPopupWindow( aParent, aFlags )
    {
        setWindowFlags( Qt::Popup | Qt::FramelessWindowHint );
    }

    /// Show the popup (it gives the focus to @a aFocus).
    virtual void Popup( wxWindow* aFocus = nullptr )
    {
        show();
        raise();

        if( aFocus )
            aFocus->SetFocus();
    }

    /// Hide the popup.
    virtual void Dismiss() { hide(); }

protected:
    /// The popup was closed (by Dismiss() or by a click outside it).
    virtual void OnDismiss() {}

    void hideEvent( QHideEvent* aEvent ) override
    {
        wxPopupWindow::hideEvent( aEvent );
        OnDismiss();
    }
};
