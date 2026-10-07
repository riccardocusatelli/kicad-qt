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
 * @file msgpanel.cpp
 * EDA_MSG_PANEL of the Qt build.
 */

#include <widgets/msgpanel.h>

#include <QPainter>
#include <QPaintEvent>

#include <qt_strings.h>


EDA_MSG_PANEL::EDA_MSG_PANEL( QWidget* aParent ) :
        QWidget( aParent )
{
    setAutoFillBackground( true );
    setBackgroundRole( QPalette::Button );
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
}


EDA_MSG_PANEL::~EDA_MSG_PANEL()
{
}


QSize EDA_MSG_PANEL::sizeHint() const
{
    // Two lines of text
    return QSize( 100, 2 * fontMetrics().height() + 4 );
}


void EDA_MSG_PANEL::AppendMessage( const wxString& aUpperText, const wxString& aLowerText,
                                   int aPadding )
{
    m_Items.emplace_back( aUpperText, aLowerText, aPadding );
    update();
}


void EDA_MSG_PANEL::EraseMsgBox()
{
    m_Items.clear();
    update();
}


void EDA_MSG_PANEL::paintEvent( QPaintEvent* aEvent )
{
    QPainter     painter( this );
    QFontMetrics metrics = fontMetrics();
    int          charWidth = metrics.horizontalAdvance( QLatin1Char( 'W' ) );
    int          lineHeight = metrics.height();

    // Change the text to a disabled color when the window isn't active
    painter.setPen( palette().color( window()->isActiveWindow() ? QPalette::Active
                                                                : QPalette::Inactive,
                                     QPalette::WindowText ) );

    // Don't put the first message a window client position 0.  Offset by
    // one 'W' character width.
    int x = charWidth;
    int upperY = height() / 2 - lineHeight + metrics.ascent();
    int lowerY = height() - lineHeight + metrics.ascent();

    for( const MSG_PANEL_ITEM& item : m_Items )
    {
        QString upper = ToQString( item.GetUpperText() );
        QString lower = ToQString( item.GetLowerText() );

        if( !upper.isEmpty() )
            painter.drawText( x, upperY, upper );

        if( !lower.isEmpty() )
            painter.drawText( x, lowerY, lower );

        QString text = upper.size() > lower.size() ? upper : lower;
        text += QString( item.GetPadding(), QLatin1Char( ' ' ) );

        // Add an extra space between texts for a better look:
        x += metrics.horizontalAdvance( text ) + charWidth;
    }
}
