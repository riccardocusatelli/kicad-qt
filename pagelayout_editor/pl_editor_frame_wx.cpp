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
 * @file pl_editor_frame_wx.cpp
 * The parts of PL_EDITOR_FRAME with wxWidgets: the fields of its status bar and the layout of its
 * panes.
 */

#include <wx/statusbr.h>

#include "pl_editor_frame.h"
#include "properties_frame.h"

#include <core/arraydim.h>
#include <tool/action_toolbar.h>
#include <widgets/msgpanel.h>
#include <widgets/ui_common.h>


void PL_EDITOR_FRAME::setupLayout()
{
    wxStatusBar* stsbar = GetStatusBar();
    int          spacer = KIUI::GetTextSize( wxT( "M" ), stsbar ).x * 2;

    int dims[] = {

        // balance of status bar on far left is set to a default or whatever is left over.
        -1,

        // When using GetTextSize() remember the width of '1' is not the same
        // as the width of '0' unless the font is fixed width, and it usually won't be.

        // zoom:
        KIUI::GetTextSize( wxT( "Z 762000" ), stsbar ).x + spacer,

        // cursor coords
        KIUI::GetTextSize( wxT( "X 0234.567  Y 0234.567" ), stsbar ).x + spacer,

        // delta distances
        KIUI::GetTextSize( wxT( "dx 0234.567  dx 0234.567" ), stsbar ).x + spacer,

        // grid size
        KIUI::GetTextSize( wxT( "grid 0234.567" ), stsbar ).x + spacer,

        // Coord origin (use the bigger message)
        KIUI::GetTextSize( _( "coord origin: Right Bottom page corner" ), stsbar ).x + spacer,

        // units display, Inches is bigger than mm
        KIUI::GetTextSize( _( "Inches" ), stsbar ).x + spacer,

        // constraint mode
        KIUI::GetTextSize( _( "Constrain to H, V, 45" ), stsbar ).x + spacer
    };

    if( stsbar )
        stsbar->SetFieldsCount( arrayDim( dims ), dims );

    m_auimgr.SetManagedWindow( this );

    CreateInfoBar();
    m_propertiesPagelayout = new PROPERTIES_FRAME( this );

    // Rows; layers 4 - 6
    m_auimgr.AddPane( m_tbTopMain, EDA_PANE().HToolbar().Name( "TopMainToolbar" )
                      .Top().Layer( 6 ) );
    m_auimgr.AddPane( m_tbLeft, EDA_PANE().VToolbar().Name( "LeftToolbar" )
                      .Left().Layer( 3 ) );
    m_auimgr.AddPane( m_messagePanel, EDA_PANE().Messages().Name( "MsgPanel" )
                      .Bottom().Layer( 6 ) );

    // Columns; layers 1 - 3
    m_auimgr.AddPane( m_tbRight, EDA_PANE().VToolbar().Name( "RightToolbar" )
                      .Right().Layer( 2 ) );

    m_auimgr.AddPane( m_propertiesPagelayout, EDA_PANE().Palette().Name( "Props" )
                      .Right().Layer( 3 )
                      .Caption( _( "Properties" ) )
                      .MinSize( m_propertiesPagelayout->GetMinSize() )
                      .BestSize( m_propertiesFrameWidth, -1 ) );

    // Center
    m_auimgr.AddPane( GetCanvas(), EDA_PANE().Canvas().Name( "DrawFrame" )
                      .Center() );

    FinishAUIInitialization();
}
