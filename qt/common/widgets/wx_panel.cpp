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


#include <widgets/wx_panel.h>

#include <QPainter>
#include <QPalette>

#include <qt_ui.h>


WX_PANEL::WX_PANEL( wxWidgetArgCompat aParent, wxWindowID aId, const wxPoint& aPos,
                    const wxSize& aSize, long aStyle, const wxString& aName ) :
        wxPanel( aParent, aId, aPos, aSize, aStyle, aName )
{
}


void WX_PANEL::paintEvent( QPaintEvent* aEvent )
{
    QWidget::paintEvent( aEvent );

    if( !m_leftBorder && !m_rightBorder && !m_topBorder && !m_bottomBorder )
        return;

    QColor border = ToQColor( m_borderColor );

    if( m_borderColor == KIGFX::COLOR4D::UNSPECIFIED )
    {
        // The color of the borders of the frame, mixed with its background
        KIGFX::COLOR4D bg = FromQColor( palette().color( QPalette::Window ) );
        KIGFX::COLOR4D fg = FromQColor( palette().color( QPalette::Mid ) );
        border = ToQColor( fg.Mix( bg, 0.18 ) );
    }

    QPainter painter( this );
    painter.setPen( QPen( border, 1 ) );

    QRect rect = this->rect();

    if( m_leftBorder )
        painter.drawLine( rect.topLeft(), rect.bottomLeft() );

    if( m_rightBorder )
        painter.drawLine( rect.topRight(), rect.bottomRight() );

    if( m_topBorder )
        painter.drawLine( rect.topLeft(), rect.topRight() );

    if( m_bottomBorder )
        painter.drawLine( rect.bottomLeft(), rect.bottomRight() );
}
