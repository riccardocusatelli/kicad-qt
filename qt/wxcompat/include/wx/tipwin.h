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
 * @file tipwin.h
 * The tip window of the wxWidgets compatibility layer (wxTipWindow): a popup with a text under
 * the mouse, closed by a click outside it or by Close().
 */

#pragma once

#include <wx/popupwin.h>

#include <algorithm>

#include <QCursor>
#include <QLabel>
#include <QVBoxLayout>

#include <qt_strings.h>


class wxTipWindow : public wxPopupTransientWindow
{
public:
    wxTipWindow( wxWidgetArgCompat aParent, const wxString& aText, int aMaxLength = 100,
                 wxTipWindow** aWindowPtr = nullptr, wxRect* aRectBound = nullptr ) :
            wxPopupTransientWindow( aParent ),
            m_windowPtr( aWindowPtr )
    {
        setAttribute( Qt::WA_DeleteOnClose );

        QVBoxLayout* layout = new QVBoxLayout( this );
        QLabel*      label = new QLabel( ToQString( aText ), this );
        label->setWordWrap( true );
        label->setMaximumWidth( label->fontMetrics().averageCharWidth() * std::max( aMaxLength, 1 ) );
        layout->addWidget( label );

        adjustSize();
        move( QCursor::pos() + QPoint( 0, 16 ) );
        Popup();
    }

    ~wxTipWindow() override { clearWindowPtr(); }

    /// The pointer to clear when the tip closes.
    void SetTipWindowPtr( wxTipWindow** aWindowPtr ) { m_windowPtr = aWindowPtr; }

    /// Close the tip (it is deleted).
    void Close() { close(); }

protected:
    void OnDismiss() override
    {
        clearWindowPtr();
        deleteLater();
    }

private:
    void clearWindowPtr()
    {
        if( m_windowPtr )
        {
            *m_windowPtr = nullptr;
            m_windowPtr = nullptr;
        }
    }

    wxTipWindow** m_windowPtr;
};
