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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file headless_eeschema.cpp
 *
 * Uses the schematic model of eeschema directly, without the GUI toolkit: loads a schematic,
 * runs the ERC, generates the netlist and plots the sheets to SVG.
 *
 *   qa_headless_eeschema <schematic.kicad_sch> <output dir> [expected ERC violations]
 *
 * It is linked like the kicad-cli kiface (static archives, --no-undefined against wxBase):
 * building it proves that these parts of the model need no GUI.
 */

#include <cstdio>
#include <memory>

#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/init.h>

#include <drawing_sheet/ds_proxy_view_item.h>
#include <eeschema_helpers.h>
#include <erc/erc.h>
#include <erc/erc_settings.h>
#include <kiface_base.h>
#include <kiface_netlist.h>
#include <kiway_base.h>
#include <libraries/symbol_library_adapter.h>
#include <pgm_base.h>
#include <project_sch.h>
#include <reporter.h>
#include <sch_plotter.h>
#include <sch_render_settings.h>
#include <sch_screen.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <string_utils.h>


namespace
{

int s_failures = 0;


void check( bool aCondition, const char* aWhat )
{
    std::printf( "%s  %s\n", aCondition ? "ok  " : "FAIL", aWhat );

    if( !aCondition )
        s_failures++;
}


/// Run the ERC as kicad-cli does; @return the number of violations (errors and warnings).
int runErc( SCHEMATIC* aSch )
{
    SYMBOL_LIBRARY_ADAPTER* adapter = PROJECT_SCH::SymbolLibAdapter( &aSch->Project() );
    adapter->AsyncLoad();
    adapter->BlockUntilLoaded();

    SCH_SCREEN* root = aSch->RootScreen();

    DS_PROXY_VIEW_ITEM drawingSheet( schIUScale, &root->GetPageSettings(), &aSch->Project(),
                                     &root->GetTitleBlock(), aSch->GetProperties() );
    drawingSheet.SetPageNumber( TO_UTF8( root->GetPageNumber() ) );
    drawingSheet.SetSheetCount( root->GetPageCount() );
    drawingSheet.SetFileName( TO_UTF8( root->GetFileName() ) );
    drawingSheet.SetIsFirstPage( root->GetVirtualPageNumber() == 1 );

    ERC_TESTER tester( aSch );
    tester.RunTests( &drawingSheet, {}, nullptr, &aSch->Project(), nullptr );

    SHEETLIST_ERC_ITEMS_PROVIDER markers( aSch );
    markers.SetSeverities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING );

    return markers.GetCount();
}


/// The program: settings and project management only, no GUI.
class PGM_HEADLESS_TEST : public PGM_BASE
{
public:
    ~PGM_HEADLESS_TEST() override { Destroy(); }
};

} // namespace


int main( int argc, char** argv )
{
    if( argc < 3 )
    {
        std::fprintf( stderr,
                      "usage: %s <schematic.kicad_sch> <output dir> [expected ERC violations]\n",
                      argv[0] );
        return 2;
    }

    wxInitializer wxBase( argc, argv );

    PGM_HEADLESS_TEST program;
    SetPgm( &program );

    if( !program.InitPgm( true, true ) )
    {
        std::fprintf( stderr, "cannot initialise the program\n" );
        return 2;
    }

    KIWAY_BASE kiway( KFCTL_CPP_PROJECT_SUITE | KFCTL_CLI );
    program.GetSettingsManager().SetKiway( &kiway );
    Kiface().OnKifaceStart( &program, KFCTL_CLI, &kiway );

    wxFileName inputFile( wxString::FromUTF8( argv[1] ) );
    inputFile.MakeAbsolute();

    wxString   schFile = inputFile.GetFullPath();
    wxFileName outDir = wxFileName::DirName( wxString::FromUTF8( argv[2] ) );
    outDir.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

    std::unique_ptr<SCHEMATIC> sch( EESCHEMA_HELPERS::LoadSchematic( schFile, true, false ) );
    check( sch != nullptr, "load the schematic" );

    if( !sch )
        return 1;

    SCH_SHEET_LIST sheets = sch->Hierarchy();
    size_t         symbols = 0;

    for( const SCH_SHEET_PATH& sheet : sheets )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            (void) item;
            symbols++;
        }
    }

    std::printf( "      %zu sheets, %zu symbols\n", sheets.size(), symbols );

    int violations = runErc( sch.get() );
    std::printf( "      %d ERC violations\n", violations );

    if( argc > 3 )
        check( violations == std::atoi( argv[3] ), "ERC violations as expected" );

    std::string netlist;
    check( GenerateSchematicNetlist( schFile, netlist ) && netlist.find( "(export" ) == 0,
           "generate the netlist" );

    SCH_RENDER_SETTINGS renderSettings;
    renderSettings.LoadColors( program.GetSettingsManager().GetColorSettings( wxEmptyString ) );
    renderSettings.SetDefaultPenWidth( sch->Settings().m_DefaultLineWidth );

    SCH_PLOT_OPTS plotOpts;
    plotOpts.m_outputDirectory = outDir.GetPath();

    NULL_REPORTER reporter;
    SCH_PLOTTER   plotter( sch.get() );
    plotter.Plot( PLOT_FORMAT::SVG, plotOpts, &renderSettings, &reporter );

    wxArrayString svgFiles;
    wxDir::GetAllFiles( outDir.GetPath(), &svgFiles, wxS( "*.svg" ), wxDIR_FILES );
    check( svgFiles.size() == sheets.size(), "one SVG file per sheet" );

    sch.reset();

    Kiface().OnKifaceEnd();
    program.Destroy();

    std::printf( "%s\n", s_failures ? "FAILED" : "PASSED" );
    return s_failures ? 1 : 0;
}
