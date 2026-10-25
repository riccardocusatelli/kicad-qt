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
 * @file symbol_viewer_frame_qt.cpp
 * The parts of SYMBOL_VIEWER_FRAME with Qt: the layout of its panes (see
 * eeschema/symbol_viewer_frame_wx.cpp).
 */

#include <QDockWidget>

#include <symbol_viewer_frame.h>


void SYMBOL_VIEWER_FRAME::setupLayout( wxPanel* libPanel, wxPanel* symbolPanel )
{
    CreateInfoBar();

    // The canvas, the info bar and the message panel
    setupCentralArea();

    // The libraries and the symbols on the left side, side by side
    QDockWidget* libraries = addPane( libPanel, wxS( "Libraries" ), wxEmptyString,
                                      PANE_DOCK::LEFT );
    QDockWidget* symbols = addPane( symbolPanel, wxS( "Symbols" ), wxEmptyString,
                                    PANE_DOCK::LEFT );

    splitDockWidget( libraries, symbols, Qt::Horizontal );

    // No caption, as the wxWidgets build
    for( QDockWidget* dock : { libraries, symbols } )
        dock->setTitleBarWidget( new QWidget( dock ) );

    libPanel->setMinimumWidth( 100 );
    symbolPanel->setMinimumWidth( 100 );
    resizeDocks( { libraries, symbols }, { 200, 300 }, Qt::Horizontal );
}


void SYMBOL_VIEWER_FRAME::OnSize( wxSizeEvent& SizeEv )
{
    // The dock widgets of Qt follow the size of the frame
    SizeEv.Skip();
}
