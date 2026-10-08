/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or (at your
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
 * @file wx_html_report_box.cpp
 * WX_HTML_REPORT_BOX of the Qt build (see common/widgets/wx_html_report_box.cpp).
 */

#include <widgets/wx_html_report_box.h>

#include <QFontMetrics>
#include <QScrollBar>

#include <math/util.h>


WX_HTML_REPORT_BOX::WX_HTML_REPORT_BOX( QWidget* parent, wxWindowID id, const wxPoint& pos,
                                        const wxSize& size, long style ) :
        HTML_WINDOW( parent, id, pos, size, style ),
        m_units( EDA_UNITS::MM ),
        m_immediateMode( false )
{
    // The theme changes rebuild the page (HTML_WINDOW::ThemeChanged())
    Flush();
}


WX_HTML_REPORT_BOX::~WX_HTML_REPORT_BOX()
{
}


REPORTER& WX_HTML_REPORT_BOX::Report( const wxString& aText, SEVERITY aSeverity )
{
    m_messages.push_back( aText );

    if( m_immediateMode )
    {
        Flush();
        verticalScrollBar()->setValue( verticalScrollBar()->maximum() );
    }

    return *this;
}


void WX_HTML_REPORT_BOX::Flush()
{
    wxString html;

    for( const wxString& line : m_messages )
        html += generateHtml( line );

    SetPage( html );
}


wxString WX_HTML_REPORT_BOX::generateHtml( const wxString& aLine )
{
    // As the wxWidgets build: a minimum line spacing, as the lines of the reporter are mostly
    // single lines (an invisible image of the height of the spacing)
    int additionalLineSpacing = KiROUND( QFontMetrics( font() ).height() * 0.6 );

    return wxString::Format( wxT( "<img align=texttop height=%d width=0 src=#>%s<br>" ),
                             additionalLineSpacing, aLine );
}


void WX_HTML_REPORT_BOX::Clear()
{
    REPORTER::Clear();
    m_messages.clear();
}
