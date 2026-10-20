/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2020 Ian McInerney <ian.s.mcinerney at ieee dot org>
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
 * @file bitmap_button.h
 * BITMAP_BUTTON of the Qt build: a button with an icon, drawn as the buttons of the toolbars,
 * that can be a check or a radio button, a separator, and show a badge (see
 * include/widgets/bitmap_button.h).
 */

#ifndef BITMAP_BUTTON_H_
#define BITMAP_BUTTON_H_

#include <kiqt.h>     // Qt, without its METHOD macro
#include <QAbstractButton>
#include <QColor>
#include <QIcon>

#include <wx/defs.h>
#include <wx/gdicmn.h>
#include <wx/string.h>
#include <wx/kiwx_window.h>


class wxBitmap;


class KIQT_API BITMAP_BUTTON : public wxWindowCompat<QAbstractButton>
{
public:
    BITMAP_BUTTON( wxWidgetArgCompat aParent, wxWindowID aId = wxID_ANY,
                   const wxPoint& aPos = wxDefaultPosition, const wxSize& aSize = wxDefaultSize );

    /// The constructor of wxFormBuilder (the bitmap is set later).
    BITMAP_BUTTON( wxWidgetArgCompat aParent, wxWindowID aId, const wxBitmap& aDummyBitmap,
                   const wxPoint& aPos = wxDefaultPosition, const wxSize& aSize = wxDefaultSize,
                   int aStyle = 0 ) :
            BITMAP_BUTTON( aParent, aId, aPos, aSize )
    {
    }

    ~BITMAP_BUTTON() override;

    /**
     * Set the amount of padding present on each side of the bitmap.
     *
     * @param aPadding is the padding on each side inside the edges of the button.
     */
    void SetPadding( int aPadding );

    /// Set the bitmap shown when the button is enabled.
    void SetBitmap( const QIcon& aBmp );

    /// Set the bitmap shown when the button is disabled.
    void SetDisabledBitmap( const QIcon& aBmp );

    /// Enable the button.  @return true if the state changed.
    bool Enable( bool aEnable = true ) override;

    /// Setup the control as a two-state button (checked or unchecked).
    void SetIsCheckButton();

    /// Setup the control as a radio button (clicking it doesn't uncheck it).
    void SetIsRadioButton();

    /// Check the control (only for check and radio buttons).
    void Check( bool aCheck = true );

    /// @return true if the button is checked (only for check and radio buttons).
    bool IsChecked() const;

    /// Render as a toolbar separator (a vertical line).
    void SetIsSeparator();

    /// Accept a mouse-up as a click even if the mouse-down happened outside of the control.
    void AcceptDragInAsClick( bool aAcceptDragIn = true ) { m_acceptDraggedInClicks = aAcceptDragIn; }

    void SetShowBadge( bool aShowBadge ) { m_showBadge = aShowBadge; update(); }

    void SetBadgeText( const wxString& aText );

    void SetBadgeColors( const QColor& aBadgeColor, const QColor& aBadgeTextColor )
    {
        m_badgeColor = aBadgeColor;
        m_badgeTextColor = aBadgeTextColor;
        update();
    }

    /// Draw the bitmap centered in the control (instead of offset by the padding).
    void SetBitmapCentered( bool aCentered = true ) { m_centerBitmap = aCentered; update(); }

    void SetIsToolbarButton( bool aIsToolbar = true ) { m_isToolbarButton = aIsToolbar; }
    bool IsToolbarButton() const { return m_isToolbarButton; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    void paintEvent( QPaintEvent* aEvent ) override;
    void nextCheckState() override;
    void mouseReleaseEvent( QMouseEvent* aEvent ) override;

private:
    QIcon     m_normalBitmap;
    QIcon     m_disabledBitmap;
    QSize     m_bitmapSize;
    bool      m_isRadioButton;
    bool      m_isSeparator;
    bool      m_showBadge;
    QString   m_badgeText;
    QColor    m_badgeColor;
    QColor    m_badgeTextColor;
    int       m_padding;
    bool      m_isToolbarButton;
    bool      m_acceptDraggedInClicks;
    bool      m_centerBitmap;
};

#endif /*BITMAP_BUTTON_H_*/
