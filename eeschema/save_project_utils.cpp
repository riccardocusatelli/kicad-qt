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

// The save-as relocation of the sheets, shared by the schematic editor (files-io.cpp) and the
// jobs (eeschema_jobs_handler.cpp)

#include <save_project_utils.h>

#include <core/ki_assert.h>
#include <core/ki_log.h>
#include <schematic.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <trace_helpers.h>
#include <wx/translation.h>


bool PrepareSaveAsFiles( SCHEMATIC& aSchematic, SCH_SCREENS& aScreens, const wxFileName& aOldRoot,
                         const wxFileName& aNewRoot, bool aSaveCopy, bool aCopySubsheets, bool aIncludeExternSheets,
                         std::unordered_map<SCH_SCREEN*, wxString>& aFilenameMap, wxString& aErrorMsg )
{
    SCH_SCREEN* screen;

    for( size_t i = 0; i < aScreens.GetCount(); i++ )
    {
        screen = aScreens.GetScreen( i );

        KI_CHECK2( screen, continue );

        if( screen == aSchematic.RootScreen() )
            continue;

        // The virtual root's screen only holds the top-level sheets and has no file of its own
        // A destination here would be a nameless path that callers turn into a stray ".kicad_sch"
        if( screen == aSchematic.Root().GetScreen() )
            continue;

        wxFileName src = screen->GetFileName();

        if( !src.IsAbsolute() )
            src.MakeAbsolute( aOldRoot.GetPath() );

        bool internalSheet = src.GetPath().StartsWith( aOldRoot.GetPath() );

        if( aCopySubsheets && ( internalSheet || aIncludeExternSheets ) )
        {
            wxFileName dest = src;

            if( internalSheet && dest.MakeRelativeTo( aOldRoot.GetPath() ) )
                dest.MakeAbsolute( aNewRoot.GetPath() );
            else
                dest.Assign( aNewRoot.GetPath(), dest.GetFullName() );

            KI_LOG_TRACE( tracePathsAndFiles, wxS( "Moving schematic from '%s' to '%s'." ),
                        screen->GetFileName(), dest.GetFullPath() );

            if( !dest.DirExists() && !dest.Mkdir() )
            {
                aErrorMsg.Printf( _( "Folder '%s' could not be created.\n\n"
                                     "Make sure you have write permissions and try again." ),
                                 dest.GetPath() );
                return false;
            }

            if( aSaveCopy )
                aFilenameMap[screen] = dest.GetFullPath();
            else
                screen->SetFileName( dest.GetFullPath() );
        }
        else
        {
            if( aSaveCopy )
                aFilenameMap[screen] = wxString();

            screen->SetFileName( src.GetFullPath() );
        }
    }

    for( SCH_SHEET_PATH& sheet : aSchematic.Hierarchy() )
    {
        if( !sheet.Last()->IsTopLevelSheet() )
            sheet.MakeFilePathRelativeToParentSheet();
    }

    return true;
}
