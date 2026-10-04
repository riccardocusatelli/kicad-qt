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
 * @file headless_pcbnew.cpp
 *
 * Uses the board model of pcbnew directly, without the GUI toolkit: loads a board, fills the
 * zones, runs the DRC through a headless BOARD_COMMIT, plots the copper layers to Gerber, saves
 * the board and loads it again.
 *
 *   qa_headless_pcbnew <board.kicad_pcb> <output dir> [expected DRC violations]
 *
 * It is linked like the kicad-cli kiface (static archives, --no-undefined against wxBase):
 * building it proves that these parts of the model need no GUI.
 */

#include <cstdio>
#include <memory>

#include <wx/filename.h>
#include <wx/init.h>

#include <board.h>
#include <board_commit.h>
#include <board_design_settings.h>
#include <drawing_sheet/ds_proxy_view_item.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <footprint.h>
#include <kiface_base.h>
#include <kiway_base.h>
#include <pcb_marker.h>
#include <pcb_plot_params.h>
#include <pcb_plotter.h>
#include <pcbnew_scripting_helpers.h>
#include <pgm_base.h>
#include <reporter.h>
#include <settings/settings_manager.h>
#include <string_utils.h>
#include <zone.h>
#include <zone_filler.h>


namespace
{

int s_failures = 0;


void check( bool aCondition, const char* aWhat )
{
    std::printf( "%s  %s\n", aCondition ? "ok  " : "FAIL", aWhat );

    if( !aCondition )
        s_failures++;
}


struct BOARD_COUNTS
{
    size_t footprints = 0;
    size_t tracks = 0;
    size_t zones = 0;
    size_t drawings = 0;
    int    nets = 0;

    bool operator==( const BOARD_COUNTS& ) const = default;
};


BOARD_COUNTS countItems( const BOARD& aBoard )
{
    return { aBoard.Footprints().size(), aBoard.Tracks().size(), aBoard.Zones().size(),
             aBoard.Drawings().size(), aBoard.GetNetCount() };
}


/// Run the DRC as kicad-cli does; @return the number of violations (errors and warnings).
int runDrc( BOARD* aBoard )
{
    std::shared_ptr<DRC_ENGINE> engine = aBoard->GetDesignSettings().m_DRCEngine;

    DS_PROXY_VIEW_ITEM drawingSheet( pcbIUScale, &aBoard->GetPageSettings(),
                                     aBoard->GetProject(), &aBoard->GetTitleBlock(),
                                     &aBoard->GetProperties() );
    drawingSheet.SetIsFirstPage( true );
    drawingSheet.SetFileName( TO_UTF8( aBoard->GetFileName() ) );

    BOARD_COMMIT commit( aBoard );

    engine->SetDrawingSheet( &drawingSheet );
    engine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos, int aLayer,
                 const std::function<void( PCB_MARKER* )>& aPathGenerator )
            {
                PCB_MARKER* marker = new PCB_MARKER( aItem, aPos, aLayer );
                aPathGenerator( marker );
                commit.Add( marker );
            } );

    aBoard->DeleteMARKERs( true, true );
    engine->RunTests( EDA_UNITS::MM, false, false );
    engine->ClearViolationHandler();
    engine->SetDrawingSheet( nullptr );

    commit.Push( wxS( "DRC" ), SKIP_UNDO | SKIP_SET_DIRTY );

    DRC_ITEMS_PROVIDER markers( aBoard, MARKER_BASE::MARKER_DRC,
                                MARKER_BASE::MARKER_DRAWING_SHEET );
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
        std::fprintf( stderr, "usage: %s <board.kicad_pcb> <output dir> [expected DRC violations]\n",
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

    wxString   boardFile = inputFile.GetFullPath();
    wxFileName outDir = wxFileName::DirName( wxString::FromUTF8( argv[2] ) );
    outDir.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

    std::unique_ptr<BOARD> board( LoadBoard( boardFile, true ) );
    check( board != nullptr, "load the board" );

    if( !board )
        return 1;

    BOARD_COUNTS loaded = countItems( *board );
    std::printf( "      %zu footprints, %zu tracks, %zu zones, %zu drawings, %d nets\n",
                 loaded.footprints, loaded.tracks, loaded.zones, loaded.drawings, loaded.nets );

    check( FillAllBoardZones( board.get() ), "fill the zones" );

    int violations = runDrc( board.get() );
    std::printf( "      %d DRC violations\n", violations );

    if( argc > 3 )
        check( violations == std::atoi( argv[3] ), "DRC violations as expected" );

    PCB_PLOT_PARAMS plotOpts = board->GetPlotOptions();
    plotOpts.SetFormat( PLOT_FORMAT::GERBER );

    NULL_REPORTER reporter;
    PCB_PLOTTER   plotter( board.get(), &reporter, plotOpts );

    check( plotter.Plot( outDir.GetPath(), { F_Cu, B_Cu }, {}, false ), "plot the copper layers" );

    wxString savedFile = wxFileName( outDir.GetPath(), wxS( "saved.kicad_pcb" ) ).GetFullPath();
    check( SaveBoard( savedFile, board.get(), true ), "save the board" );

    std::unique_ptr<BOARD> reloaded( LoadBoard( savedFile, false ) );
    check( reloaded != nullptr, "load the saved board" );

    if( reloaded )
        check( countItems( *reloaded ) == countItems( *board ), "same items after save and load" );

    reloaded.reset();
    board.reset();

    Kiface().OnKifaceEnd();
    program.Destroy();

    std::printf( "%s\n", s_failures ? "FAILED" : "PASSED" );
    return s_failures ? 1 : 0;
}
