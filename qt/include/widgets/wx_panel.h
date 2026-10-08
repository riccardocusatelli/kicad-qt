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
 * @file wx_panel.h
 * WX_PANEL of the Qt build: a panel with borders (the class of KiCad is kept, so that the code
 * generated from the wxFormBuilder files uses it).
 */

#ifndef WX_PANEL_H
#define WX_PANEL_H

#include <QWidget>

#include <kiqt.h>
#include <gal/color4d.h>


class KIQT_API WX_PANEL : public QWidget
{
public:
    WX_PANEL( QWidget* aParent );

    ~WX_PANEL() override = default;

    void SetBorders( bool aLeft, bool aRight, bool aTop, bool aBottom )
    {
        m_leftBorder = aLeft;
        m_rightBorder = aRight;
        m_topBorder = aTop;
        m_bottomBorder = aBottom;
        update();
    }

    void SetBorderColor( const KIGFX::COLOR4D& aColor )
    {
        m_borderColor = aColor;
        update();
    }

protected:
    void paintEvent( QPaintEvent* aEvent ) override;

private:
    bool           m_leftBorder = false;
    bool           m_rightBorder = false;
    bool           m_topBorder = false;
    bool           m_bottomBorder = false;

    KIGFX::COLOR4D m_borderColor = KIGFX::COLOR4D::UNSPECIFIED;
};


#endif //WX_PANEL_H
