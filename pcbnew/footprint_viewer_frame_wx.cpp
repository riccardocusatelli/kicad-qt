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
 * @file footprint_viewer_frame_wx.cpp
 * The parts of FOOTPRINT_VIEWER_FRAME with wxWidgets: the layout of its panes and its events.
 */

#include <footprint_viewer_frame.h>
#include <id.h>
#include <pcbnew_id.h>
#include <tool/action_toolbar.h>
#include <widgets/wx_aui_utils.h>
#include <widgets/wx_listbox.h>
#include <wx/srchctrl.h>


void FOOTPRINT_VIEWER_FRAME::setupLayout( wxPanel* aLibPanel, wxPanel* aFpPanel )
{
    m_auimgr.SetManagedWindow( this );

    // Horizontal items; layers 4 - 6
    m_auimgr.AddPane( m_tbTopMain, EDA_PANE().VToolbar().Name( "TopMainToolbar" ).Top().Layer(6) );
    m_auimgr.AddPane( m_tbLeft, EDA_PANE().VToolbar().Name( "LeftToolbar" ).Left().Layer(3) );
    m_auimgr.AddPane( m_messagePanel, EDA_PANE().Messages().Name( "MsgPanel" ).Bottom().Layer(6) );

    // Vertical items; layers 1 - 3
    m_auimgr.AddPane( aLibPanel, EDA_PANE().Palette().Name( "Libraries" ).Left().Layer(2)
                      .CaptionVisible( false ).MinSize( 100, -1 ).BestSize( 200, -1 ) );
    m_auimgr.AddPane( aFpPanel, EDA_PANE().Palette().Name( "Footprints" ).Left().Layer(1)
                      .CaptionVisible( false ).MinSize( 100, -1 ).BestSize( 300, -1 ) );

    m_auimgr.AddPane( GetCanvas(), EDA_PANE().Canvas().Name( "DrawFrame" ).Center() );

    // after changing something to the aui manager call Update() to reflect the changes
    m_auimgr.Update();
}


void FOOTPRINT_VIEWER_FRAME::setupToolkitEvents()
{
    // Window events
    Bind( wxEVT_SIZE, &FOOTPRINT_VIEWER_FRAME::OnSize, this );

    Bind( wxEVT_MENU, [this]( wxCommandEvent& ) { OnExitKiCad(); }, wxID_EXIT );
    Bind( wxEVT_MENU, [this]( wxCommandEvent& ) { CloseFootprintViewer(); }, wxID_CLOSE );

    // Toolbar events
    Bind( wxEVT_CHOICE, &FOOTPRINT_VIEWER_FRAME::OnSelectZoom, this, ID_ON_ZOOM_SELECT );
    Bind( wxEVT_CHOICE, &FOOTPRINT_VIEWER_FRAME::OnSelectGrid, this, ID_ON_GRID_SELECT );

    Bind( wxEVT_TEXT, &FOOTPRINT_VIEWER_FRAME::OnLibFilter, this, ID_MODVIEW_LIB_FILTER );
    Bind( wxEVT_TEXT, &FOOTPRINT_VIEWER_FRAME::OnFPFilter, this, ID_MODVIEW_FOOTPRINT_FILTER );

    // listbox events
    Bind( wxEVT_LISTBOX, &FOOTPRINT_VIEWER_FRAME::ClickOnLibList, this, ID_MODVIEW_LIB_LIST );
    Bind( wxEVT_LISTBOX, &FOOTPRINT_VIEWER_FRAME::ClickOnFootprintList, this,
          ID_MODVIEW_FOOTPRINT_LIST );
}


void FOOTPRINT_VIEWER_FRAME::OnSize( wxSizeEvent& SizeEv )
{
    if( m_auimgr.GetManagedWindow() )
        m_auimgr.Update();

    SizeEv.Skip();
}
