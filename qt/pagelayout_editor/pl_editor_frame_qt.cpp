/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jean-Pierre Charras, jp.charras at wanadoo.fr
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
 * @file pl_editor_frame_qt.cpp
 * The parts of PL_EDITOR_FRAME with Qt: the fields of its status bar and the layout of its panes
 * (see pagelayout_editor/pl_editor_frame_wx.cpp).
 */

#include <iterator>

#include <QDockWidget>
#include <QFontMetrics>

#include <qt_strings.h>
#include <widgets/kistatusbar.h>

#include "pl_editor_frame.h"
#include "properties_frame.h"


void PL_EDITOR_FRAME::setupLayout()
{
    // The fields of the drawing sheet editor: the origin of the coordinates after the grid
    if( KISTATUSBAR* stsbar = GetStatusBar() )
    {
        QFontMetrics metrics( stsbar->font() );
        int          spacer = metrics.horizontalAdvance( QLatin1Char( 'M' ) ) * 2;

        auto textWidth =
                [&]( const wxString& aText )
                {
                    return metrics.horizontalAdvance( ToQString( aText ) ) + spacer;
                };

        int dims[] = {
            -1,                                                     // messages
            textWidth( wxT( "Z 762000" ) ),                         // zoom
            textWidth( wxT( "X 0234.567  Y 0234.567" ) ),           // cursor coords
            textWidth( wxT( "dx 0234.567  dx 0234.567" ) ),         // delta distances
            textWidth( wxT( "grid 0234.567" ) ),                    // grid size
            textWidth( _( "coord origin: Right Bottom page corner" ) ), // coord origin
            textWidth( _( "Inches" ) ),                             // units
            textWidth( _( "Constrain to H, V, 45" ) )               // constraint mode
        };

        SetStatusWidths( int( std::size( dims ) ), dims );
    }

    CreateInfoBar();
    m_propertiesPagelayout = new PROPERTIES_FRAME( this );

    // The canvas, the info bar and the message panel
    setupCentralArea();

    // The properties of the items on the right side
    QDockWidget* properties = addPane( m_propertiesPagelayout, wxS( "Props" ), _( "Properties" ),
                                       PANE_DOCK::RIGHT );

    m_propertiesPagelayout->setMinimumWidth( m_propertiesPagelayout->GetMinSize().x );
    resizeDocks( { properties }, { m_propertiesFrameWidth }, Qt::Horizontal );
}
