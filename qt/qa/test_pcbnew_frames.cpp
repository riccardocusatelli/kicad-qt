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
 * @file test_pcbnew_frames.cpp
 * The test of the frames of pcbnew on Qt: the KIFACE with the GUI (pcbnew/pcbnew.cpp and
 * pcbnew_gui_qt) makes the original frames (the PCB editor, the footprint editor, the footprint
 * viewer), which open a board and run their tools.
 */

#include <kicad_gl/kiglad.h> // Must be included first (OpenGL GAL)
#include <cstdio>
#include <functional>
#include <string>
#include <vector>
#include <kiqt.h>     // Qt, without its METHOD macro
#include <QApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QScreen>
#include <QTimer>
#include <QDialog>
#include <QPointer>
#include <QPushButton>
#include <QTemporaryDir>
#include <wx/init.h>

#include <kiqt_app.h>

#include <api/api_server.h>
#include <board.h>
#include <class_draw_panel_gal.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <footprint_edit_frame.h>
#include <frame_type.h>
#include <kiface_base.h>
#include <kiway.h>
#include <kiway_player.h>
#include <pcb_edit_frame.h>
#include <pgm_gui.h>
#include <widgets/paged_dialog.h>
#include <widgets/wx_treebook.h>
#include <settings/common_settings.h>
#include <settings/kicad_settings.h>
#include <settings/settings_manager.h>
#include <startwizard/startwizard.h>
#include <libraries/library_manager.h>
#include <wx/filename.h>
#include <wx/wizard.h>
#include <tool/actions.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <zone.h>
#include <tool/tool_manager.h>


/// The KIFACE linked in the test is the one of pcbnew in its KIWAY, as in the program of
/// single_top (the friend of KIWAY which registers it).
struct PGM_SINGLE_TOP
{
    static void RegisterKiface( KIWAY& aKiway )
    {
        aKiway.set_kiface( KIWAY::KifaceType( FRAME_PCB_EDITOR ), &Kiface() );
    }
};


namespace
{

int s_failures = 0;


void check( bool aOk, const char* aWhat )
{
    std::printf( "  %s  %s\n", aOk ? "ok  " : "FAIL", aWhat );

    if( !aOk )
        s_failures++;
}


/// The program of the frames: a program with a GUI (PgmGui()).
class PGM_QT_TEST : public PGM_GUI
{
public:
    ~PGM_QT_TEST() override { Destroy(); }

    void MacOpenFile( const wxString& aFileName ) override {}

    /// The server of the IPC API, as the one of single_top (the frames register their handlers)
    void StartApiServer()
    {
#ifdef KICAD_IPC_API
        m_api_server = std::make_unique<KICAD_API_SERVER>();
#endif
    }
};


/// Let the event loop run (the events posted by the frames, their timers).
void settle( int aMilliseconds = 50 )
{
    QElapsedTimer timer;
    timer.start();

    while( timer.elapsed() < aMilliseconds )
        QCoreApplication::processEvents( QEventLoop::AllEvents, 10 );
}


/// Run a function on the next dialog that opens (modal, or quasi-modal: a dialog with its own
/// event loop, which is not a modal widget).  @return the poll of the dialogs (deleted once the
/// dialog is found).
QTimer* onNextDialog( std::function<void( QDialog* )> aFunction )
{
    QTimer* poll = new QTimer();

    QObject::connect( poll, &QTimer::timeout, poll,
                      [poll, aFunction]()
                      {
                          for( QWidget* widget : QApplication::topLevelWidgets() )
                          {
                              QDialog* dialog = qobject_cast<QDialog*>( widget );

                              if( dialog && dialog->isVisible() )
                              {
                                  poll->stop();
                                  poll->deleteLater();
                                  aFunction( dialog );
                                  return;
                              }
                          }
                      } );

    poll->start( 20 );
    return poll;
}


/// Press a button of a dialog, by its label.
bool pressButton( QWidget* aDialog, const QString& aLabel )
{
    for( QPushButton* button : aDialog->findChildren<QPushButton*>() )
    {
        if( button->text().remove( QLatin1Char( '&' ) ) == aLabel )
        {
            button->click();
            return true;
        }
    }

    return false;
}


/**
 * A dialog of the editor, opened by an action: shown, with the size of its content (its pages
 * are not squeezed), then closed with one of its buttons.
 */
void testDialog( TOOL_MANAGER* aToolMgr, const TOOL_ACTION& aAction, const char* aName,
                 const QString& aScreenshot, const QString& aCloseButton,
                 std::function<void( QDialog* )> aWork = nullptr )
{
    bool shown = false;
    bool fits = false;
    bool closed = false;

    QTimer* poll = onNextDialog( [&]( QDialog* aDialog )
                  {
                      shown = true;
                      settle( 200 );

                      // What the test does in the dialog
                      if( aWork )
                          aWork( aDialog );

                      // The dialog has the size of its content
                      QSize content = aDialog->minimumSizeHint();
                      fits = aDialog->width() >= content.width()
                             && aDialog->height() >= content.height();

                      if( !aScreenshot.isEmpty() )
                      {
                          QString path = aScreenshot;
                          QString suffix = QString::fromUtf8( aName ).replace( ' ', '-' );
                          path.replace( QStringLiteral( ".png" ),
                                        QStringLiteral( "-%1.png" ).arg( suffix ) );
                          aDialog->grab().save( path );
                      }

                      closed = pressButton( aDialog, aCloseButton );

                      // Not closed: rejected, the test goes on
                      if( !closed )
                          aDialog->reject();
                  } );

    QPointer<QTimer> pollPtr( poll );

    aToolMgr->RunAction( aAction );
    settle( 300 );

    // No dialog: the poll does not wait for the next one
    if( pollPtr && !shown )
        delete pollPtr.data();

    std::string name( aName );
    check( shown, ( "the " + name + " dialog is shown" ).c_str() );
    check( fits, ( "the " + name + " dialog fits its content" ).c_str() );
    check( closed, ( "the " + name + " dialog is closed" ).c_str() );
}


/**
 * The start wizard of a new configuration (the settings of the test are new): its pages are
 * shown, the wizard is finished, and it makes the global library tables.
 */
void testStartWizard( const QString& aScreenshot )
{
    std::printf( "STARTWIZARD\n" );

    int  pages = 0;
    bool finished = false;

    // The pages, one after the other: Next, then Finish on the last one
    QTimer* poll = new QTimer();

    QObject::connect( poll, &QTimer::timeout, poll,
                      [&, poll]()
                      {
                          QDialog* wizard = nullptr;

                          for( QWidget* widget : QApplication::topLevelWidgets() )
                          {
                              if( QDialog* dialog = qobject_cast<QDialog*>( widget ) )
                              {
                                  if( dialog->isVisible() && dialog->inherits( "QDialog" )
                                          && dynamic_cast<wxWizard*>( dialog ) )
                                  {
                                      wizard = dialog;
                                  }
                              }
                          }

                          if( !wizard )
                              return;

                          pages++;

                          if( !aScreenshot.isEmpty() )
                          {
                              QString path = aScreenshot;
                              path.replace( QStringLiteral( ".png" ),
                                            QStringLiteral( "-start-wizard-%1.png" ).arg( pages ) );
                              wizard->grab().save( path );
                          }

                          if( pressButton( wizard, QStringLiteral( "Finish" ) ) )
                          {
                              finished = true;
                              poll->stop();
                              poll->deleteLater();
                          }
                          else if( !pressButton( wizard, QStringLiteral( "Next >" ) ) || pages > 10 )
                          {
                              poll->stop();
                              poll->deleteLater();
                          }
                      } );

    poll->start( 200 );

    STARTWIZARD startWizard;
    startWizard.CheckAndRun( nullptr );

    check( pages > 1, "the start wizard shows its pages" );
    check( finished, "the start wizard is finished" );

    wxFileName table( SETTINGS_MANAGER::GetUserSettingsPath(), wxS( "fp-lib-table" ) );
    check( table.FileExists(), "the start wizard makes the footprint library table" );
}


/// A frame of the KIFACE, made by the KIWAY (its player: the frames find each other).
template <typename FRAME>
FRAME* createFrame( KIWAY& aKiway, FRAME_T aFrameType )
{
    return dynamic_cast<FRAME*>( aKiway.Player( aFrameType, true ) );
}


/// Close a frame, then delete it (its deferred delete: the event loop of the program would).
void closeFrame( EDA_BASE_FRAME* aFrame, const char* aName )
{
    QPointer<QWidget> alive( aFrame );

    aFrame->Close( true );
    settle( 100 );
    QCoreApplication::sendPostedEvents( nullptr, QEvent::DeferredDelete );

    check( alive.isNull(), ( std::string( aName ) + " is closed and deleted" ).c_str() );
}


/**
 * The footprint editor: a footprint of the board is edited, with the dialogs of the editor (the
 * properties of the footprint and of a pad, the default pad, the checker, the cleanup of the
 * graphics, the enumeration of the pads).
 */
void testFootprintEditor( KIWAY& aKiway, FOOTPRINT* aFootprint, const QString& aScreenshot )
{
    std::printf( "FOOTPRINT_EDIT_FRAME\n" );

    FOOTPRINT_EDIT_FRAME* frame = createFrame<FOOTPRINT_EDIT_FRAME>( aKiway,
                                                                     FRAME_FOOTPRINT_EDITOR );
    check( frame != nullptr, "the KIFACE makes the footprint editor" );

    if( !frame )
        return;

    frame->show();
    settle( 300 );
    check( frame->isVisible(), "the footprint editor is shown" );
    check( frame->GetBoard() != nullptr, "the footprint editor has its board" );

    if( aFootprint && frame->LoadFootprintFromBoard( aFootprint ) )
    {
        settle( 300 );

        BOARD*        board = frame->GetBoard();
        TOOL_MANAGER* toolMgr = frame->GetToolManager();
        FOOTPRINT*    footprint = board->GetFirstFootprint();

        check( footprint && footprint->GetFPID() == aFootprint->GetFPID(),
               "the footprint editor edits the footprint of the board" );

        if( footprint )
        {
            PCB_SELECTION_TOOL* selTool = toolMgr->GetTool<PCB_SELECTION_TOOL>();

            testDialog( toolMgr, PCB_ACTIONS::footprintProperties, "footprint editor properties",
                        aScreenshot, "Cancel" );

            if( selTool && !footprint->Pads().empty() )
            {
                toolMgr->RunAction( ACTIONS::selectionClear );
                selTool->AddItemToSel( footprint->Pads().front() );
                settle( 50 );
                testDialog( toolMgr, PCB_ACTIONS::properties, "footprint editor pad properties",
                            aScreenshot, "Cancel" );
                toolMgr->RunAction( ACTIONS::selectionClear );
            }

            testDialog( toolMgr, PCB_ACTIONS::defaultPadProperties, "default pad properties",
                        aScreenshot, "Cancel" );
            testDialog( toolMgr, PCB_ACTIONS::checkFootprint, "footprint checker", aScreenshot,
                        "Close" );
            testDialog( toolMgr, PCB_ACTIONS::cleanupGraphics, "cleanup graphics", aScreenshot,
                        "Cancel" );
            testDialog( toolMgr, PCB_ACTIONS::enumeratePads, "enumerate pads", aScreenshot,
                        "Cancel" );
        }

        if( !aScreenshot.isEmpty() )
        {
            QString path = aScreenshot;
            path.replace( QStringLiteral( ".png" ), QStringLiteral( "-footprint-editor.png" ) );
            frame->screen()->grabWindow( frame->winId() ).save( path );
        }
    }
    else
    {
        check( false, "the footprint editor loads the footprint of the board" );
    }

    frame->GetScreen()->SetContentModified( false );
    closeFrame( frame, "the footprint editor" );
}


void testPcbEditor( KIWAY& aKiway, const wxString& aBoardFile, const QString& aScreenshot )
{
    std::printf( "PCB_EDIT_FRAME\n" );

    PCB_EDIT_FRAME* frame = createFrame<PCB_EDIT_FRAME>( aKiway, FRAME_PCB_EDITOR );
    check( frame != nullptr, "the KIFACE makes the PCB editor" );

    if( !frame )
        return;

    frame->show();
    settle( 300 );
    check( frame->isVisible(), "the PCB editor is shown" );
    check( frame->GetCanvas() != nullptr, "the PCB editor has a canvas" );
    check( frame->GetToolManager() != nullptr, "the PCB editor has its tools" );

    check( frame->OpenProjectFiles( { aBoardFile } ), "the PCB editor opens the board" );
    settle( 300 );

    BOARD* board = frame->GetBoard();
    check( board && !board->Footprints().empty(), "the board has its footprints" );
    check( board && !board->Tracks().empty(), "the board has its tracks" );

    if( TOOL_MANAGER* toolMgr = frame->GetToolManager() )
    {
        toolMgr->RunAction( ACTIONS::zoomFitScreen );
        settle( 100 );
        toolMgr->RunAction( ACTIONS::zoomInCenter );
        settle( 100 );
        check( true, "the zoom actions run" );

        // The canvas draws the board (with the GAL of the settings: the OpenGL one draws in
        // its own window, which is not in the grab of the frame)
        QImage image;
        bool   varied = false;

        for( int attempt = 0; attempt < 20 && !varied; ++attempt )
        {
            frame->GetCanvas()->ForceRefresh();
            settle( 100 );

            if( !frame->GetCanvas()->GetScreenshot( image ) || image.width() < 100 )
                continue;

            for( int y = 0; y < image.height() && !varied; y += 7 )
            {
                for( int x = 0; x < image.width() && !varied; x += 7 )
                    varied = image.pixel( x, y ) != image.pixel( 0, 0 );
            }
        }

        check( varied, "the canvas draws the board" );

        if( !aScreenshot.isEmpty() )
        {
            QString path = aScreenshot;
            path.replace( QStringLiteral( ".png" ), QStringLiteral( "-canvas.png" ) );
            check( image.save( path ), "screenshot of the canvas" );
        }
    }

    // The tools of the editor on the board
    if( TOOL_MANAGER* toolMgr = frame->GetToolManager(); toolMgr && board )
    {
        PCB_SELECTION_TOOL* selTool = toolMgr->GetTool<PCB_SELECTION_TOOL>();
        size_t              footprints = board->Footprints().size();
        size_t              tracks = board->Tracks().size();

        toolMgr->RunAction( ACTIONS::selectAll );
        settle( 100 );
        check( selTool && selTool->GetSelection().Size() > 0, "select all" );

        toolMgr->RunAction( ACTIONS::doDelete );
        settle( 100 );
        check( board->Footprints().empty() && board->Tracks().empty(), "delete the selection" );

        toolMgr->RunAction( ACTIONS::undo );
        settle( 100 );
        check( board->Footprints().size() == footprints && board->Tracks().size() == tracks,
               "undo the deletion" );

        toolMgr->RunAction( ACTIONS::selectionClear );
        settle( 50 );

        toolMgr->RunAction( PCB_ACTIONS::zoneFillAll );
        settle( 300 );

        bool filled = !board->Zones().empty();

        for( ZONE* zone : board->Zones() )
        {
            if( !zone->GetIsRuleArea() && !zone->IsFilled() )
                filled = false;
        }

        check( filled, "fill the zones" );

        // The dialogs of the editor: board setup (quasi-modal), the properties of a footprint
        // (quasi-modal), DRC (not modal)
        testDialog( toolMgr, PCB_ACTIONS::boardSetup, "board setup", aScreenshot, "Cancel" );

        // The properties of an item: selected, then its dialog
        auto testProperties =
                [&]( BOARD_ITEM* aItem, const char* aName )
                {
                    if( !aItem )
                        return;

                    toolMgr->RunAction( ACTIONS::selectionClear );
                    selTool->AddItemToSel( aItem );
                    settle( 50 );
                    testDialog( toolMgr, PCB_ACTIONS::properties, aName, aScreenshot, "Cancel" );
                    toolMgr->RunAction( ACTIONS::selectionClear );
                };

        auto firstTrack =
                [&]( KICAD_T aType ) -> BOARD_ITEM*
                {
                    for( PCB_TRACK* track : board->Tracks() )
                    {
                        if( track->Type() == aType )
                            return track;
                    }

                    return nullptr;
                };

        if( !board->Footprints().empty() )
        {
            FOOTPRINT* footprint = board->Footprints().front();
            testProperties( footprint, "footprint properties" );

            if( !footprint->Pads().empty() )
                testProperties( footprint->Pads().front(), "pad properties" );
        }

        testProperties( firstTrack( PCB_TRACE_T ), "track properties" );
        testProperties( firstTrack( PCB_VIA_T ), "via properties" );

        if( !board->Zones().empty() )
            testProperties( board->Zones().front(), "zone properties" );

        for( BOARD_ITEM* item : board->Drawings() )
        {
            if( item->Type() == PCB_TEXT_T )
            {
                testProperties( item, "text properties" );
                break;
            }
        }

        testDialog( toolMgr, ACTIONS::plot, "plot", aScreenshot, "Close" );

        // The other dialogs of the menus: preferences, libraries, edits, fabrication outputs
        // Preferences: every page is shown (the pages of the KIFACEs are made when shown)
        int prefPages = 0;

        testDialog( toolMgr, ACTIONS::openPreferences, "preferences", aScreenshot, "Cancel",
                    [&]( QDialog* aDialog )
                    {
                        PAGED_DIALOG* dialog = dynamic_cast<PAGED_DIALOG*>( aDialog );

                        if( !dialog )
                            return;

                        WX_TREEBOOK* book = dialog->GetTreebook();

                        for( size_t i = 0; i < book->GetPageCount(); ++i )
                        {
                            book->SetSelection( i );
                            settle( 30 );

                            if( book->GetCurrentPage() )
                                prefPages++;

                            if( !aScreenshot.isEmpty() )
                            {
                                QString path = aScreenshot;
                                path.replace( QStringLiteral( ".png" ),
                                              QStringLiteral( "-preferences-%1.png" ).arg( i ) );
                                aDialog->grab().save( path );
                            }
                        }

                        book->SetSelection( 0 );
                        settle( 30 );
                    } );

        check( prefPages >= 25, "the preferences show their pages" );
        testDialog( toolMgr, ACTIONS::configurePaths, "paths", aScreenshot, "Cancel" );
        testDialog( toolMgr, ACTIONS::showFootprintLibTable, "footprint libraries", aScreenshot,
                    "Cancel" );
        testDialog( toolMgr, PCB_ACTIONS::editTracksAndVias, "edit tracks and vias", aScreenshot,
                    "Close" );
        testDialog( toolMgr, PCB_ACTIONS::editTextAndGraphics, "edit text and graphics",
                    aScreenshot, "Close" );
        testDialog( toolMgr, PCB_ACTIONS::globalDeletions, "global deletions", aScreenshot,
                    "Cancel" );
        testDialog( toolMgr, PCB_ACTIONS::cleanupTracksAndVias, "cleanup tracks and vias",
                    aScreenshot, "Cancel" );
        testDialog( toolMgr, PCB_ACTIONS::boardStatistics, "board statistics", aScreenshot,
                    "Close" );
        testDialog( toolMgr, PCB_ACTIONS::generateDrillFiles, "drill files", aScreenshot,
                    "Close" );
        testDialog( toolMgr, PCB_ACTIONS::generatePosFile, "position files", aScreenshot,
                    "Close" );
        // IPC-2581: the board is exported (next to the board, by default)
        wxFileName ipcFile( board->GetFileName() );
        ipcFile.SetExt( wxS( "xml" ) );

        testDialog( toolMgr, PCB_ACTIONS::generateIPC2581File, "IPC-2581", aScreenshot, "Close",
                    [&]( QDialog* aDialog )
                    {
                        pressButton( aDialog, QStringLiteral( "Export" ) );
                        settle( 1000 );
                    } );

        check( ipcFile.FileExists(), "the IPC-2581 file is exported" );

        // STEP: without the warning about the scaled models of the board, shown before
        Pgm().GetCommonSettings()->m_DoNotShowAgain.scaled_3d_models_warning = true;
        testDialog( toolMgr, PCB_ACTIONS::exportSTEP, "STEP export", aScreenshot, "Close" );

        // DRC: run, then closed
        bool drcRun = false;

        testDialog( toolMgr, PCB_ACTIONS::runDRC, "DRC", aScreenshot, "Close",
                    [&]( QDialog* aDialog )
                    {
                        drcRun = pressButton( aDialog, QStringLiteral( "Run DRC" ) );

                        // The DRC runs in the dialog (its progress reporter), then shows the
                        // violations
                        settle( 1500 );
                    } );

        check( drcRun, "run DRC" );
        check( !board->Markers().empty(), "the DRC finds the violations of the board" );
    }

    if( !aScreenshot.isEmpty() )
    {
        // From the display: the grab of the widgets has no OpenGL window (a native window)
        QPixmap shot = frame->screen()->grabWindow( frame->winId() );
        check( shot.save( aScreenshot ), "screenshot of the PCB editor" );

        QString path = aScreenshot;
        path.replace( QStringLiteral( ".png" ), QStringLiteral( "-grab.png" ) );
        frame->grab().save( path );
    }

    // The footprint editor, with a footprint of the board
    testFootprintEditor( aKiway, board->Footprints().empty() ? nullptr : board->Footprints().front(),
                         aScreenshot );

    // The changes of the test are not saved (the editor would ask)
    frame->GetScreen()->SetContentModified( false );
    closeFrame( frame, "the PCB editor" );
}


} // namespace


int main( int argc, char** argv )
{
    if( argc < 2 )
    {
        std::fprintf( stderr, "usage: %s <board.kicad_pcb> [screenshot.png]\n", argv[0] );
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

    // The board is opened from a copy (the editor writes the project file next to it)
    QTemporaryDir dir;
    QString       copy = dir.filePath( QFileInfo( QString::fromUtf8( argv[1] ) ).fileName() );
    check( QFile::copy( QString::fromUtf8( argv[1] ), copy ), "copy the board" );

    testStartWizard( argc > 2 ? QString::fromUtf8( argv[2] ) : QString() );
    program.GetLibraryManager().LoadGlobalTables();

    testPcbEditor( kiway, wxString::FromUTF8( copy.toUtf8().constData() ),
                   argc > 2 ? QString::fromUtf8( argv[2] ) : QString() );

    Kiface().OnKifaceEnd();
    program.Destroy();

    std::printf( "%s\n", s_failures ? "FAILED" : "PASSED" );
    return s_failures ? 1 : 0;
}
