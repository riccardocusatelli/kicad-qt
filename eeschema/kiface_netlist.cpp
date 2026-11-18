/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
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

#include <kiface_netlist.h>

#include <connection_graph.h>
#include <netlist_exporters/netlist_exporter_kicad.h>
#include <pgm_base.h>
#include <io/io_base.h>
#include <richio.h>
#include <sch_io/sch_io.h>
#include <sch_io/sch_io_mgr.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <wildcards_and_files_ext.h>

#include <wx/filename.h>


static std::unique_ptr<SCHEMATIC> readSchematicFromFile( const std::string& aFilename )
{
    IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );
    std::unique_ptr<SCHEMATIC> schematic = std::make_unique<SCHEMATIC>( nullptr );

    SETTINGS_MANAGER& manager = Pgm().GetSettingsManager();

    wxFileName pro( aFilename );
    pro.SetExt( FILEEXT::ProjectFileExtension );
    pro.MakeAbsolute();
    wxString projectPath = pro.GetFullPath();

    PROJECT* project = manager.GetProject( projectPath );

    if( !project )
    {
        manager.LoadProject( projectPath, true );
        project = manager.GetProject( projectPath );
    }

    schematic->Reset();
    schematic->SetProject( project );
    SCH_SHEET* rootSheet = pi->LoadSchematicFile( aFilename, schematic.get() );

    if( !rootSheet )
        return nullptr;

    std::vector<SCH_SHEET*> topLevelSheets = schematic->GetTopLevelSheets();
    bool rootIsTopLevel = std::find( topLevelSheets.begin(), topLevelSheets.end(), rootSheet )
                          != topLevelSheets.end();
    bool rootIsVirtualRoot = rootSheet == &schematic->Root() || rootSheet->IsVirtualRootSheet();

    if( !rootIsTopLevel && !rootIsVirtualRoot )
        schematic->SetTopLevelSheets( { rootSheet } );

    SCH_SCREENS screens( schematic->Root() );

    for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
        screen->UpdateLocalLibSymbolLinks();

    SCH_SHEET_LIST sheets = schematic->Hierarchy();

    // Restore all of the loaded symbol instances from the root sheet screen.
    sheets.UpdateSymbolInstanceData( schematic->RootScreen()->GetSymbolInstances() );

    if( schematic->RootScreen()->GetFileFormatVersionAtLoad() < 20230221 )
    {
        for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
            screen->FixLegacyPowerSymbolMismatches();
    }

    for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
        screen->MigrateSimModels();

    // NOTE: This is required for multi-unit symbols to be correct
    for( SCH_SHEET_PATH& sheet : sheets )
        sheet.UpdateAllScreenReferences();

    // TODO: this must handle SchematicCleanup somehow.  The original version didn't because
    // it knew that QA test cases were saved in a clean state.

    // TODO: does this need to handle PruneOrphanedSymbolInstances() and
    // PruneOrphanedSheetInstances()?

    schematic->ConnectionGraph()->Recalculate( sheets, true );

    return schematic;
}


bool GenerateSchematicNetlist( const wxString& aFilename, std::string& aNetlist, KIWAY* aKiway )
{
    std::unique_ptr<SCHEMATIC> schematic = readSchematicFromFile( aFilename.ToStdString() );

    if( !schematic )
        return false;

    NETLIST_EXPORTER_KICAD exporter( schematic.get(), aKiway );
    STRING_FORMATTER       formatter;

    exporter.Format( &formatter, GNL_ALL | GNL_OPT_KICAD );
    aNetlist = formatter.GetString();

    return true;
}
