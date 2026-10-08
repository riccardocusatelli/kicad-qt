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
 * @file eda_base_frame_help.cpp
 * The standard help menu of the frames of the Qt build: its actions are the ones of
 * COMMON_CONTROL (a file of its own, linked with the frames that use the tool).
 */

#include <eda_base_frame.h>

#include <tool/action_menu.h>
#include <tool/actions.h>
#include <tool/common_control.h>
#include <tool/tool_manager.h>
#include <widgets/wx_menubar.h>


void EDA_BASE_FRAME::AddStandardHelpMenu( WX_MENUBAR* aMenuBar )
{
    COMMON_CONTROL* commonControl = m_toolManager->GetTool<COMMON_CONTROL>();
    ACTION_MENU*    helpMenu = new ACTION_MENU( false, commonControl );

    helpMenu->Add( ACTIONS::help );
    helpMenu->Add( ACTIONS::gettingStarted );
    helpMenu->Add( ACTIONS::listHotKeys );
    helpMenu->Add( ACTIONS::getInvolved );
    helpMenu->Add( ACTIONS::donate );
    helpMenu->Add( ACTIONS::reportBug );

    helpMenu->AppendSeparator();
    helpMenu->Add( ACTIONS::about );

    aMenuBar->Append( helpMenu, _( "&Help" ) );
}
