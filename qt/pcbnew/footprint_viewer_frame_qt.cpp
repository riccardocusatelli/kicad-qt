/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2016 Wayne Stambaugh <stambaughw@gmail.com>
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
 * @file footprint_viewer_frame_qt.cpp
 * The parts of FOOTPRINT_VIEWER_FRAME with Qt: the layout of its panes and its events (see
 * pcbnew/footprint_viewer_frame_wx.cpp).  The filters and the lists give their events to the
 * frame through the handlers bound on them.
 */

#include <QDockWidget>

#include <footprint_viewer_frame.h>
#include <widgets/wx_listbox.h>
#include <wx/srchctrl.h>


void FOOTPRINT_VIEWER_FRAME::setupLayout( wxPanel* aLibPanel, wxPanel* aFpPanel )
{
    // The canvas, the info bar and the message panel
    setupCentralArea();

    // The libraries and the footprints on the left side, side by side
    QDockWidget* libraries = addPane( aLibPanel, wxS( "Libraries" ), wxEmptyString,
                                      PANE_DOCK::LEFT );
    QDockWidget* footprints = addPane( aFpPanel, wxS( "Footprints" ), wxEmptyString,
                                       PANE_DOCK::LEFT );

    splitDockWidget( libraries, footprints, Qt::Horizontal );

    // No caption, as the wxWidgets build
    for( QDockWidget* dock : { libraries, footprints } )
        dock->setTitleBarWidget( new QWidget( dock ) );

    aLibPanel->setMinimumWidth( 100 );
    aFpPanel->setMinimumWidth( 100 );
    resizeDocks( { libraries, footprints }, { 200, 300 }, Qt::Horizontal );
}


void FOOTPRINT_VIEWER_FRAME::setupToolkitEvents()
{
    BindMenuCommand( wxID_EXIT, [this]() { OnExitKiCad(); } );
    BindMenuCommand( wxID_CLOSE, [this]() { CloseFootprintViewer(); } );

    m_libFilter->Bind( wxEVT_TEXT, &FOOTPRINT_VIEWER_FRAME::OnLibFilter, this );
    m_fpFilter->Bind( wxEVT_TEXT, &FOOTPRINT_VIEWER_FRAME::OnFPFilter, this );

    m_libList->Bind( wxEVT_LISTBOX, &FOOTPRINT_VIEWER_FRAME::ClickOnLibList, this );
    m_fpList->Bind( wxEVT_LISTBOX, &FOOTPRINT_VIEWER_FRAME::ClickOnFootprintList, this );
}


void FOOTPRINT_VIEWER_FRAME::OnSize( wxSizeEvent& SizeEv )
{
    // The dock widgets of Qt follow the size of the frame
    SizeEv.Skip();
}
