/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2015 Miguel Angel Ajo Pelayo <miguelangel@nbee.es>
 * Copyright (C) 2012-2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
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
 * @file footprint_wizard_frame_wx.cpp
 * The parts of FOOTPRINT_WIZARD_FRAME with wxWidgets: the layout of its panes and its events.
 */

#include <wx/listbox.h>
#include <wx/panel.h>
#include <wx/textctrl.h>

#include <json_common.h>
#include <footprint_wizard_frame.h>
#include <pcbnew_id.h>
#include <pcbnew_settings.h>
#include <tool/action_toolbar.h>
#include <widgets/aui_json_serializer.h>
#include <widgets/wx_grid.h>


void FOOTPRINT_WIZARD_FRAME::setupLayout()
{
    SetSize( m_framePos.x, m_framePos.y, m_frameSize.x, m_frameSize.y );

    m_auimgr.SetManagedWindow( this );

    m_auimgr.AddPane( m_tbTopMain, EDA_PANE().HToolbar().Name( "TopMainToolbar" ).Top().Layer(6) );
    m_auimgr.AddPane( m_messagePanel, EDA_PANE().Messages().Name( "MsgPanel" ).Bottom().Layer(6)
                      .BestSize( -1, m_msgFrameHeight ) );

    m_auimgr.AddPane( m_parametersPanel, EDA_PANE().Palette().Name( "Params" ).Left().Position(0)
                      .Caption( _( "Parameters" ) ).MinSize( 360, 180 ) );
    m_auimgr.AddPane( m_buildMessageBox, EDA_PANE().Palette().Name( "Output" ).Left().Position(1)
                      .CaptionVisible( false ).MinSize( 360, -1 ) );

    m_auimgr.AddPane( GetCanvas(), wxAuiPaneInfo().Name( "DrawFrame" ).CentrePane() );
}


void FOOTPRINT_WIZARD_FRAME::setupToolkitEvents()
{
    // Window events
    Bind( wxEVT_SIZE, &FOOTPRINT_WIZARD_FRAME::OnSize, this );

    // Toolbar events
    Bind( wxEVT_TOOL, &FOOTPRINT_WIZARD_FRAME::SelectCurrentWizard, this,
          ID_FOOTPRINT_WIZARD_SELECT_WIZARD );
    Bind( wxEVT_TOOL, &FOOTPRINT_WIZARD_FRAME::ExportSelectedFootprint, this,
          ID_FOOTPRINT_WIZARD_DONE );

    // listbox events
    Bind( wxEVT_LISTBOX, &FOOTPRINT_WIZARD_FRAME::ClickOnPageList, this,
          ID_FOOTPRINT_WIZARD_PAGE_LIST );
    Bind( wxEVT_GRID_CELL_CHANGED, &FOOTPRINT_WIZARD_FRAME::ParametersUpdated, this,
          ID_FOOTPRINT_WIZARD_PARAMETER_LIST );
}


void FOOTPRINT_WIZARD_FRAME::saveLayout( PCBNEW_SETTINGS* cfg )
{
#if wxCHECK_VERSION( 3, 3, 0 )
    {
        WX_AUI_JSON_SERIALIZER serializer( m_auimgr );
        nlohmann::json state = serializer.Serialize();

        if( state.is_null() || state.empty() )
            cfg->m_FootprintViewer.aui_state = nlohmann::json();
        else
            cfg->m_FootprintViewer.aui_state = state;

        cfg->m_FootprintViewer.perspective.clear();
    }
#else
    cfg->m_FootprintViewer.perspective = m_auimgr.SavePerspective().ToStdString();
    cfg->m_FootprintViewer.aui_state = nlohmann::json();
#endif
}


void FOOTPRINT_WIZARD_FRAME::OnSize( wxSizeEvent& SizeEv )
{
    if( m_auimgr.GetManagedWindow() )
        m_auimgr.Update();

    SizeEv.Skip();
}
