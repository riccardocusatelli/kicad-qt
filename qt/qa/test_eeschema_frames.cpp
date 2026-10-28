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
 * @file test_eeschema_frames.cpp
 * The test of the frames of eeschema on Qt: the KIFACE with the GUI (eeschema/eeschema.cpp and
 * eeschema_gui_qt) makes the original frames (the schematic editor, the symbol editor, the symbol
 * viewer), which open a schematic and run their tools and dialogs.
 */

#include <kicad_gl/kiglad.h> // Must be included first (OpenGL GAL)
#include <cstdio>
#include <string>
#include <kiqt.h>     // Qt, without its METHOD macro
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QLabel>
#include <QTemporaryDir>
#include <QTimer>
#include <QComboBox>
#include <QRadioButton>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <wx/init.h>

#include <kiqt_app.h>

#include "qa_qt_frames.h"

#include <class_draw_panel_gal.h>
#include <kiface_base.h>
#include <lib_symbol.h>
#include <sch_edit_frame.h>
#include <sch_screen.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <sim/simulator_frame.h>
#include <sim/sim_plot_tab.h>
#include <widgets/mathplot.h>
#include <settings/kicad_settings.h>
#include <libraries/library_manager.h>
#include <symbol_editor/symbol_edit_frame.h>
#include <symbol_viewer_frame.h>
#include <tool/actions.h>
#include <tools/sch_actions.h>
#include <tools/sch_selection_tool.h>


/// The KIFACE linked in the test is the one of eeschema in its KIWAY, as in the program of
/// single_top (the friend of KIWAY which registers it).
struct PGM_SINGLE_TOP
{
    static void RegisterKiface( KIWAY& aKiway )
    {
        aKiway.set_kiface( KIWAY::KifaceType( FRAME_SCH ), &Kiface() );
    }
};


namespace
{

using namespace QA_QT_FRAMES;


/// The screenshot of a test, with a suffix (empty: no screenshot).
QString screenshotPath( const QString& aScreenshot, const char* aSuffix )
{
    if( aScreenshot.isEmpty() )
        return QString();

    QString path = aScreenshot;
    return path.replace( QStringLiteral( ".png" ), QStringLiteral( "-%1.png" ).arg( aSuffix ) );
}


/// The canvas of a frame draws something (not a uniform image).
bool canvasDraws( EDA_DRAW_PANEL_GAL* aCanvas, const QString& aScreenshot )
{
    QImage image;
    bool   varied = false;

    for( int attempt = 0; attempt < 20 && !varied; ++attempt )
    {
        aCanvas->ForceRefresh();
        settle( 100 );

        if( !aCanvas->GetScreenshot( image ) || image.width() < 100 )
            continue;

        for( int y = 0; y < image.height() && !varied; y += 5 )
        {
            for( int x = 0; x < image.width() && !varied; x += 5 )
                varied = image.pixel( x, y ) != image.pixel( 0, 0 );
        }
    }

    if( varied && !aScreenshot.isEmpty() )
        image.save( aScreenshot );

    return varied;
}


/// The editor of the simulation model (DIALOG_SIM_MODEL, with its property grid), opened by a
/// button of the symbol properties dialog: shown with the parameters of the model, then closed.
void testSimModelDialog( QDialog* aParent, const QString& aButton, const QString& aScreenshot )
{
    bool shown = false;
    int  params = 0;

    QTimer* poll = new QTimer();

    QObject::connect( poll, &QTimer::timeout, poll,
                      [&, poll]()
                      {
                          for( QWidget* widget : QApplication::topLevelWidgets() )
                          {
                              QDialog* dialog = qobject_cast<QDialog*>( widget );

                              if( !dialog || dialog == aParent || !dialog->isVisible() )
                                  continue;

                              poll->stop();
                              poll->deleteLater();
                              shown = true;
                              settle( 200 );

                              // The built-in model: its parameters in the property grid
                              for( QRadioButton* radio : dialog->findChildren<QRadioButton*>() )
                              {
                                  if( radio->text() == QStringLiteral( "Built-in SPICE model" ) )
                                      radio->click();
                              }

                              settle( 300 );

                              // A device of the built-in models: the resistor
                              for( QComboBox* combo : dialog->findChildren<QComboBox*>() )
                              {
                                  int index = combo->findText( QStringLiteral( "Resistor" ) );

                                  if( index >= 0 && combo->isEnabled() )
                                  {
                                      combo->setCurrentIndex( index );
                                      Q_EMIT combo->activated( index );
                                      break;
                                  }
                              }

                              settle( 300 );

                              for( QTreeWidget* tree : dialog->findChildren<QTreeWidget*>() )
                              {
                                  for( QTreeWidgetItemIterator it( tree ); *it; ++it )
                                  {
                                      if( !( *it )->isHidden() && ( *it )->childCount() == 0 )
                                          params++;
                                  }
                              }

                              if( !aScreenshot.isEmpty() )
                                  dialog->grab().save( aScreenshot );

                              if( !pressButton( dialog, QStringLiteral( "Cancel" ) ) )
                                  dialog->reject();

                              return;
                          }
                      } );

    poll->start( 20 );
    QPointer<QTimer> pollPtr( poll );

    check( pressButton( aParent, aButton ), "the simulation model button" );
    settle( 300 );

    if( pollPtr && !shown )
        delete pollPtr.data();

    check( shown, "the simulation model dialog is shown" );
    check( params > 0, "the simulation model dialog shows the parameters of a resistor" );
}


/// The symbol editor, with a symbol of the schematic: its dialogs, then closed.
void testSymbolEditor( KIWAY& aKiway, SCH_SYMBOL* aSymbol, const QString& aScreenshot )
{
    std::printf( "SYMBOL_EDIT_FRAME\n" );

    SYMBOL_EDIT_FRAME* frame = createFrame<SYMBOL_EDIT_FRAME>( aKiway, FRAME_SCH_SYMBOL_EDITOR );
    check( frame != nullptr, "the KIFACE makes the symbol editor" );

    if( !frame )
        return;

    frame->show();
    settle( 300 );
    check( frame->isVisible(), "the symbol editor is shown" );

    if( aSymbol )
    {
        frame->LoadSymbolFromSchematic( aSymbol );
        settle( 300 );
        check( frame->GetCurSymbol() != nullptr, "the symbol editor loads the symbol of the schematic" );
        check( canvasDraws( frame->GetCanvas(), screenshotPath( aScreenshot, "symbol-editor" ) ),
               "the canvas draws the symbol" );

        TOOL_MANAGER* toolMgr = frame->GetToolManager();
        testDialog( toolMgr, SCH_ACTIONS::symbolProperties, "library symbol properties",
                    aScreenshot, QStringLiteral( "Cancel" ) );
        testDialog( toolMgr, SCH_ACTIONS::pinTable, "pin table", aScreenshot,
                    QStringLiteral( "Cancel" ) );

        if( frame->GetScreen() )
            frame->GetScreen()->SetContentModified( false );
    }

    closeFrame( frame, "the symbol editor" );
}


/// The simulator of the schematic (SIMULATOR_FRAME, with its plots): shown, then closed.
void testSimulator( KIWAY& aKiway, TOOL_MANAGER* aToolMgr, const QString& aScreenshot )
{
    std::printf( "SIMULATOR_FRAME\n" );

    aToolMgr->RunAction( SCH_ACTIONS::showSimulator );
    settle( 500 );

    auto* frame = static_cast<SIMULATOR_FRAME*>( aKiway.Player( FRAME_SIMULATOR, false ) );
    check( frame != nullptr, "the schematic editor opens the simulator" );

    if( !frame )
        return;

    check( frame->isVisible(), "the simulator is shown" );

    if( !aScreenshot.isEmpty() )
        frame->grab().save( screenshotPath( aScreenshot, "simulator" ) );

    closeFrame( frame, "the simulator" );
}


/// A simulation of a schematic: its transient analysis run by ngspice, a voltage plotted.
void testSimulation( KIWAY& aKiway, SCH_EDIT_FRAME* aFrame, const wxString& aSchematic,
                     const QString& aScreenshot )
{
    std::printf( "SIMULATION\n" );

    check( aFrame->OpenProjectFiles( { aSchematic } ), "the schematic editor opens the circuit" );
    settle( 300 );

    // The errors of the netlist of the circuit (none expected): shown, then closed
    bool    netlistError = false;
    QTimer* poll = onNextDialog( [&]( QDialog* aDialog )
                                 {
                                     netlistError = true;

                                     for( QLabel* label : aDialog->findChildren<QLabel*>() )
                                         std::printf( "  dialog: %s\n", label->text().toUtf8().constData() );

                                     aDialog->reject();
                                 } );
    QPointer<QTimer> pollPtr( poll );

    aFrame->GetToolManager()->RunAction( SCH_ACTIONS::showSimulator );
    settle( 500 );

    if( pollPtr && !netlistError )
        delete pollPtr.data();

    check( !netlistError, "the netlist of the circuit has no errors" );

    auto* frame = static_cast<SIMULATOR_FRAME*>( aKiway.Player( FRAME_SIMULATOR, false ) );
    check( frame != nullptr, "the simulator of the circuit" );

    if( !frame )
        return;

    SIM_TAB* tab = frame->GetCurrentSimTab();

    if( !tab )
        tab = frame->NewSimTab( wxS( ".tran 1u 10m" ) );

    check( tab != nullptr, "the transient analysis" );

    if( !tab )
        return;

    frame->StartSimulation();

    for( int i = 0; i < 300 && !frame->SimFinished(); ++i )
        settle( 50 );

    check( frame->SimFinished(), "ngspice runs the simulation" );

    frame->AddVoltageTrace( wxS( "/out" ) );
    settle( 300 );

    auto* plotTab = dynamic_cast<SIM_PLOT_TAB*>( frame->GetCurrentSimTab() );
    check( plotTab && !plotTab->GetTraces().empty(), "the voltage is plotted" );

    if( plotTab && plotTab->GetPlotWin() )
    {
        QWidget* plot = plotTab->GetPlotWin()->GetHandle();
        QImage   image = plot->grab().toImage();
        bool     varied = false;

        for( int y = 0; y < image.height() && !varied; y += 3 )
        {
            for( int x = 0; x < image.width() && !varied; x += 3 )
                varied = image.pixel( x, y ) != image.pixel( 0, 0 );
        }

        check( image.width() > 100 && varied, "mathplot draws the plot" );
    }

    if( !aScreenshot.isEmpty() )
        frame->grab().save( screenshotPath( aScreenshot, "simulation" ) );

    // The workbook of the simulation is modified: its changes are discarded
    bool             asked = false;
    QPointer<QTimer> discard = onNextDialog( [&]( QDialog* aDialog )
                  {
                      asked = true;
                      std::printf( "  dialog %s: %s\n", aDialog->metaObject()->className(),
                                   aDialog->windowTitle().toUtf8().constData() );

                      if( !pressButton( aDialog, "Discard Changes" ) )
                          aDialog->reject();
                  } );

    closeFrame( frame, "the simulator of the circuit" );

    if( discard )
        delete discard.data();

    check( asked, "the simulator asks to save its workbook" );
}


/// The symbol viewer: shown, then closed.
void testSymbolViewer( KIWAY& aKiway )
{
    std::printf( "SYMBOL_VIEWER_FRAME\n" );

    SYMBOL_VIEWER_FRAME* frame = createFrame<SYMBOL_VIEWER_FRAME>( aKiway, FRAME_SCH_VIEWER );
    check( frame != nullptr, "the KIFACE makes the symbol viewer" );

    if( !frame )
        return;

    frame->show();
    settle( 300 );
    check( frame->isVisible(), "the symbol viewer is shown" );
    closeFrame( frame, "the symbol viewer" );
}


void testSchEditor( KIWAY& aKiway, const wxString& aSchematicFile, const wxString& aSimSchematic,
                    const QString& aScreenshot )
{
    std::printf( "SCH_EDIT_FRAME\n" );

    SCH_EDIT_FRAME* frame = createFrame<SCH_EDIT_FRAME>( aKiway, FRAME_SCH );
    check( frame != nullptr, "the KIFACE makes the schematic editor" );

    if( !frame )
        return;

    frame->show();
    settle( 300 );
    check( frame->isVisible(), "the schematic editor is shown" );
    check( frame->GetCanvas() != nullptr, "the schematic editor has a canvas" );
    check( frame->GetToolManager() != nullptr, "the schematic editor has its tools" );

    check( frame->OpenProjectFiles( { aSchematicFile } ), "the schematic editor opens the schematic" );
    settle( 300 );

    SCH_SCREEN* screen = frame->GetScreen();
    SCH_SYMBOL* symbol = nullptr;
    size_t      items = 0;

    if( screen )
    {
        for( SCH_ITEM* item : screen->Items() )
        {
            items++;

            if( !symbol && item->Type() == SCH_SYMBOL_T )
                symbol = static_cast<SCH_SYMBOL*>( item );
        }
    }

    check( items > 0, "the schematic has its items" );
    check( symbol != nullptr, "the schematic has its symbols" );

    TOOL_MANAGER* toolMgr = frame->GetToolManager();

    toolMgr->RunAction( ACTIONS::zoomFitScreen );
    settle( 100 );
    check( canvasDraws( frame->GetCanvas(), screenshotPath( aScreenshot, "canvas" ) ),
           "the canvas draws the schematic" );

    // The tools of the editor on the schematic
    toolMgr->RunAction( ACTIONS::selectAll );
    settle( 100 );

    SCH_SELECTION_TOOL* selTool = toolMgr->GetTool<SCH_SELECTION_TOOL>();
    check( selTool && selTool->GetSelection().Size() > 0, "select all" );

    toolMgr->RunAction( ACTIONS::doDelete );
    settle( 100 );
    check( screen->Items().empty(), "delete the selection" );

    toolMgr->RunAction( ACTIONS::undo );
    settle( 100 );

    size_t restored = 0;

    for( SCH_ITEM* item : screen->Items() )
    {
        restored++;
        (void) item;
    }

    check( restored == items, "undo the delete" );

    // The dialogs of the editor
    testDialog( toolMgr, SCH_ACTIONS::annotate, "annotate", aScreenshot,
                QStringLiteral( "Close" ) );
    testDialog( toolMgr, SCH_ACTIONS::schematicSetup, "schematic setup", aScreenshot,
                QStringLiteral( "Cancel" ) );

    // The symbol of the schematic, after the undo (the items are the restored ones)
    symbol = nullptr;

    for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
    {
        symbol = static_cast<SCH_SYMBOL*>( item );
        break;
    }

    if( symbol && selTool )
    {
        selTool->ClearSelection();
        selTool->AddItemToSel( symbol );
        testDialog( toolMgr, SCH_ACTIONS::properties, "symbol properties", aScreenshot,
                    QStringLiteral( "Cancel" ),
                    [&]( QDialog* aDialog )
                    {
                        testSimModelDialog( aDialog, QStringLiteral( "Simulation Model..." ),
                                            screenshotPath( aScreenshot, "sim-model" ) );
                    } );
        selTool->ClearSelection();
    }

    if( !aScreenshot.isEmpty() )
        check( frame->grab().save( aScreenshot ), "screenshot of the schematic editor" );

    testSimulator( aKiway, toolMgr, aScreenshot );
    testSymbolEditor( aKiway, symbol, aScreenshot );
    testSymbolViewer( aKiway );

    // The changes of the test are not saved (the editor would ask)
    screen->SetContentModified( false );

    if( !aSimSchematic.IsEmpty() )
    {
        testSimulation( aKiway, frame, aSimSchematic, aScreenshot );

        if( frame->GetScreen() )
            frame->GetScreen()->SetContentModified( false );
    }

    closeFrame( frame, "the schematic editor" );
}


} // namespace


int main( int argc, char** argv )
{
    if( argc < 2 )
    {
        std::fprintf( stderr, "usage: %s <schematic.kicad_sch> [screenshot.png]\n", argv[0] );
        return 2;
    }

    // The results show up even if the test crashes
    std::setvbuf( stdout, nullptr, _IOLBF, 0 );

    // The settings of the test, not the ones of the user (the test changes them)
    QTemporaryDir configHome;

    if( !qEnvironmentVariableIsSet( "KICAD_CONFIG_HOME" ) )
        qputenv( "KICAD_CONFIG_HOME", configHome.path().toUtf8() );

    // The application object of a program with a user interface (Pgm().IsGUI())
    KIQT_APP      kiqtApp;
    wxInitializer wxBase( argc, argv );

    // The GUI of the program (PGM_GUI::InitPgm configures it)
    QApplication app( argc, argv );

    PGM_QT_TEST program;
    SetPgm( &program );

    if( !program.InitPgm( true, true ) )
    {
        std::fprintf( stderr, "cannot initialise the program\n" );
        return 2;
    }

    program.StartApiServer();

    KIWAY kiway( KFCTL_STANDALONE );
    program.GetSettingsManager().SetKiway( &kiway );

    // The settings of the project manager, as single_top (the start wizard uses them)
    program.GetSettingsManager().RegisterSettings( new KICAD_SETTINGS );
    Kiface().OnKifaceStart( &program, KFCTL_STANDALONE, &kiway );
    PGM_SINGLE_TOP::RegisterKiface( kiway );

    // The schematic is opened from a copy, with its project (the editor writes next to it)
    QTemporaryDir dir;
    QFileInfo     source( QString::fromUtf8( argv[1] ) );
    QString       copy = dir.filePath( source.fileName() );
    check( QFile::copy( source.filePath(), copy ), "copy the schematic" );

    QString project = source.path() + QLatin1Char( '/' ) + source.completeBaseName()
                      + QStringLiteral( ".kicad_pro" );

    if( QFile::exists( project ) )
        QFile::copy( project, dir.filePath( QFileInfo( project ).fileName() ) );

    // The circuit of the simulation: the rectifier of the QA data of eeschema (next to the
    // schematics of the test), copied in its own directory
    QFileInfo simSource( source.dir().filePath(
            QStringLiteral( "spice_netlists/rectifier/rectifier.kicad_sch" ) ) );
    QString   simCopy;

    if( simSource.exists() && dir.path().size() && QDir( dir.path() ).mkdir( QStringLiteral( "sim" ) ) )
    {
        // The files of the circuit: its schematic, its project and its model libraries
        QDir simDir( dir.filePath( QStringLiteral( "sim" ) ) );
        simCopy = simDir.filePath( simSource.fileName() );

        for( const QFileInfo& file : simSource.dir().entryInfoList( QDir::Files ) )
            QFile::copy( file.filePath(), simDir.filePath( file.fileName() ) );
    }

    testStartWizard( argc > 2 ? QString::fromUtf8( argv[2] ) : QString() );
    program.GetLibraryManager().LoadGlobalTables();

    testSchEditor( kiway, wxString::FromUTF8( copy.toUtf8().constData() ),
                   wxString::FromUTF8( simCopy.toUtf8().constData() ),
                   argc > 2 ? QString::fromUtf8( argv[2] ) : QString() );

    Kiface().OnKifaceEnd();
    program.Destroy();

    std::printf( "%s\n", s_failures ? "FAILED" : "PASSED" );
    return s_failures ? 1 : 0;
}
