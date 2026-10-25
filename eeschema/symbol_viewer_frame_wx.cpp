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
 * @file symbol_viewer_frame_wx.cpp
 * The parts of SYMBOL_VIEWER_FRAME with wxWidgets: the layout of its panes.
 */

#include <symbol_viewer_frame.h>
#include <tool/action_toolbar.h>
#include <widgets/wx_infobar.h>


void SYMBOL_VIEWER_FRAME::setupLayout( wxPanel* libPanel, wxPanel* symbolPanel )
{
    m_auimgr.SetManagedWindow( this );

    CreateInfoBar();

    // Manage main toolbar
    m_auimgr.AddPane( m_tbTopMain, EDA_PANE().HToolbar().Name( "TopMainToolbar" ).Top().Layer(6) );
    m_auimgr.AddPane( m_messagePanel, EDA_PANE().Messages().Name( "MsgPanel" ) .Bottom().Layer(6) );

    m_auimgr.AddPane( libPanel, EDA_PANE().Palette().Name( "Libraries" ).Left().Layer(2)
                      .CaptionVisible( false ).MinSize( 100, -1 ).BestSize( 200, -1 ) );
    m_auimgr.AddPane( symbolPanel, EDA_PANE().Palette().Name( "Symbols" ).Left().Layer(1)
                      .CaptionVisible( false ).MinSize( 100, -1 ).BestSize( 300, -1 ) );

    m_auimgr.AddPane( GetCanvas(), EDA_PANE().Canvas().Name( "DrawFrame" ).Center() );

    RestoreAuiLayout();
    m_auimgr.Update();
}


void SYMBOL_VIEWER_FRAME::OnSize( wxSizeEvent& SizeEv )
{
    if( m_auimgr.GetManagedWindow() )
        m_auimgr.Update();

    SizeEv.Skip();
}
