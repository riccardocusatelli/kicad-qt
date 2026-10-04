/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 NBEE Embedded Systems, Miguel Angel Ajo <miguelangel@nbee.es>
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
 * @file pcbnew_scripting_helpers_gui.cpp
 * @brief The scripting functions that work on the board editor.
 */

#include "pcbnew_scripting_helpers.h"

#include <action_plugin.h>
#include <board.h>
#include <pcb_edit_frame.h>
#include <tool/selection.h>

static PCB_EDIT_FRAME* s_PcbEditFrame = nullptr;


void ScriptingSetPcbEditFrame( PCB_EDIT_FRAME* aPcbEditFrame )
{
    s_PcbEditFrame = aPcbEditFrame;

    SCRIPTING_EDITOR_HOOKS hooks;

    if( aPcbEditFrame )
    {
        hooks.getBoard = [aPcbEditFrame]() { return aPcbEditFrame->GetBoard(); };
        hooks.getSettingsManager = [aPcbEditFrame]() { return aPcbEditFrame->GetSettingsManager(); };
    }

    ScriptingSetEditorHooks( hooks );
}


void ScriptingOnDestructPcbEditFrame( PCB_EDIT_FRAME* aPcbEditFrame )
{
    if( s_PcbEditFrame == aPcbEditFrame )
        ScriptingSetPcbEditFrame( nullptr );
}


bool ExportSpecctraDSN( wxString& aFullFilename )
{
    if( s_PcbEditFrame )
    {
        bool ok = s_PcbEditFrame->ExportSpecctraFile( aFullFilename );
        return ok;
    }
    else
    {
        return false;
    }
}


bool ExportVRML( const wxString& aFullFileName, double aMMtoWRMLunit, bool aIncludeUnspecified,
                 bool aIncludeDNP, bool aExport3DFiles,
                 bool aUseRelativePaths, const wxString& a3D_Subdir, double aXRef, double aYRef )
{
    if( s_PcbEditFrame )
    {
        bool ok = s_PcbEditFrame->ExportVRML_File( aFullFileName, aMMtoWRMLunit,
                                                   aIncludeUnspecified, aIncludeDNP,
                                                   aExport3DFiles, aUseRelativePaths,
                                                   a3D_Subdir, aXRef, aYRef );
        return ok;
    }
    else
    {
        return false;
    }
}


bool ImportSpecctraSES( wxString& aFullFilename )
{
    if( s_PcbEditFrame )
    {
        bool ok = s_PcbEditFrame->ImportSpecctraSession( aFullFilename );
        return ok;
    }
    else
    {
        return false;
    }
}


bool ExportFootprintsToLibrary( bool aStoreInNewLib, const wxString& aLibName, wxString* aLibPath )
{
    if( s_PcbEditFrame )
    {
        s_PcbEditFrame->ExportFootprintsToLibrary( aStoreInNewLib, aLibName, aLibPath );
        return true;
    }
    else
    {
        return false;
    }
}


void Refresh()
{
    if( s_PcbEditFrame )
    {
        s_PcbEditFrame->RebuildAndRefresh();
    }
}


void UpdateUserInterface()
{
    if( s_PcbEditFrame )
        s_PcbEditFrame->UpdateUserInterface();
}


int GetUserUnits()
{
    if( s_PcbEditFrame )
        return static_cast<int>( s_PcbEditFrame->GetUserUnits() );

    return -1;
}


std::deque<BOARD_ITEM*> GetCurrentSelection()
{
    std::deque<BOARD_ITEM*> items;

    if( s_PcbEditFrame )
    {
        SELECTION& selection = s_PcbEditFrame->GetCurrentSelection();

        std::for_each( selection.begin(), selection.end(),
                       [&items]( EDA_ITEM* item )
                       {
                           if( item->IsBOARD_ITEM() )
                               items.push_back( static_cast<BOARD_ITEM*>( item ) );
                       } );
    }

    return items;
}


void FocusOnItem( BOARD_ITEM* aItem, PCB_LAYER_ID aLayer )
{
    if( s_PcbEditFrame )
    {
        s_PcbEditFrame->FocusOnItem( aItem, aLayer );
    }
}


bool IsActionRunning()
{
    return ACTION_PLUGINS::IsActionRunning();
}
