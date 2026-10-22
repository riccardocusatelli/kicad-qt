/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Mikołaj Wielgus <wielgusmikolaj@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
 * @file html_window.cpp
 * HTML_WINDOW of the Qt build (see common/widgets/html_window.cpp).
 */

#include <widgets/html_window.h>

#include <QEvent>
#include <QScrollBar>
#include <QTextCursor>

#include <qt_strings.h>
#include <ui_hooks.h>


HTML_WINDOW::HTML_WINDOW( wxWidgetArgCompat aParent, wxWindowID aId, const wxPoint& aPos,
                          const wxSize& aSize, long aStyle, const wxString& aName ) :
        wxWindowCompat<QTextBrowser>( aParent.get() )
{
    if( aId != wxID_ANY )
        KIQT::SetWindowId( this, aId );

    if( aSize.x > 0 && aSize.y > 0 )
        resize( aSize.x, aSize.y );

    setObjectName( ToQString( aName ) );

    // The links are given to the owner (anchorClicked()), as the link events of wxHtmlWindow
    setOpenLinks( false );
    setOpenExternalLinks( false );
}


HTML_WINDOW::~HTML_WINDOW()
{
}


bool HTML_WINDOW::SetPage( const wxString& aSource )
{
    m_pageSource = aSource;

    QPalette pal = palette();
    QString  fgColor = pal.color( QPalette::Text ).name();
    QString  bgColor = pal.color( QPalette::Base ).name();
    QString  linkColor = pal.color( QPalette::Link ).name();

    QString html = QStringLiteral( "<html>\n<body text='%1' bgcolor='%2' link='%3'>\n" )
                           .arg( fgColor, bgColor, linkColor );
    html.append( ToQString( aSource ) );
    html.append( QStringLiteral( "\n</body>\n</html>" ) );

    setHtml( html );
    return true;
}


bool HTML_WINDOW::AppendToPage( const wxString& aSource )
{
    return SetPage( m_pageSource + aSource );
}


void HTML_WINDOW::ThemeChanged()
{
    SetPage( m_pageSource );
}


bool HTML_WINDOW::ScrollToAnchor( const wxString& aAnchor )
{
    // Check if we have content loaded
    if( document()->isEmpty() )
        return false;

    scrollToAnchor( ToQString( aAnchor ) );
    return true;
}


wxString HTML_WINDOW::SelectionToText() const
{
    return ToWxString( textCursor().selectedText().replace( QChar::ParagraphSeparator,
                                                            QLatin1Char( '\n' ) ) );
}


void HTML_WINDOW::changeEvent( QEvent* aEvent )
{
    if( aEvent->type() == QEvent::PaletteChange || aEvent->type() == QEvent::ThemeChange )
        ThemeChanged();

    QTextBrowser::changeEvent( aEvent );
}


void HTML_WINDOW::GetScrollPixelsPerUnit( int* aX, int* aY ) const
{
    if( aX )
        *aX = 1;

    if( aY )
        *aY = 1;
}


int HTML_WINDOW::GetScrollRange( int aOrientation ) const
{
    // The range covers the page: the maximum position and the visible part
    QScrollBar* bar = aOrientation == wxHORIZONTAL ? horizontalScrollBar() : verticalScrollBar();
    return bar->maximum() + bar->pageStep();
}


void HTML_WINDOW::Scroll( int aX, int aY )
{
    if( aX >= 0 )
        horizontalScrollBar()->setValue( aX );

    if( aY >= 0 )
        verticalScrollBar()->setValue( aY );
}


void HTML_WINDOW::GetVirtualSize( int* aWidth, int* aHeight ) const
{
    if( aWidth )
        *aWidth = horizontalScrollBar()->maximum() + horizontalScrollBar()->pageStep();

    if( aHeight )
        *aHeight = verticalScrollBar()->maximum() + verticalScrollBar()->pageStep();
}
