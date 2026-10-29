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
 * @file test_kicad_shell.cpp
 * The test of kicad-qt: the program itself (kicad_qt.cpp, its main() renamed), driven from the
 * event loop of its application.  Its start wizard is finished, a board opens in the PCB editor
 * of KiCad (the frame of _pcbnew_qtgui.kiface), a schematic of another project in the schematic
 * editor of KiCad (_eeschema_qtgui.kiface) once the editors of the board's project are closed,
 * the PCB editor shows the Python console (_kipython_qtgui.kiface), the schematic editor opens CvPcb (_cvpcb_qtgui.kiface) to assign the footprints of the
 * schematic, the launcher opens the symbol and footprint editors, and the shell closes the editors with it.
 *
 * Usage: qa_qt_kicad_shell <board.kicad_pcb> [schematic.kicad_sch] [screenshot.png]
 */

// The program, its main() renamed (the test runs it)
#define main kicad_qt_main
#include "../kicad/kicad_qt.cpp"
#undef main

#include <cstdio>

#include <QCoreApplication>
#include <QDialog>
#include <QAbstractItemView>
#include <QDir>
#include <QDockWidget>
#include <QImage>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScreen>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>

#include <wx/wizard.h>

#include <class_draw_panel_gal.h>
#include <frame_type.h>
#include <kiway_player.h>
#include <tool/tool_manager.h>

#include "../kicad/home_widgets.h"


namespace
{
int     s_failures = 0;
QString s_board;
QString s_schematic;
QString s_screenshot;
bool    s_started = false;


void check( bool aCondition, const char* aWhat )
{
    std::printf( "  %s  %s\n", aCondition ? "ok  " : "FAIL", aWhat );

    if( !aCondition )
        s_failures++;
}


/// Let the event loop run (the events posted by the frames, their timers).
void settle( int aMilliseconds = 50 )
{
    QElapsedTimer timer;
    timer.start();

    while( timer.elapsed() < aMilliseconds )
        QCoreApplication::processEvents( QEventLoop::AllEvents, 10 );
}


/// Press a button of a dialog, by its label.
/**
 * The image of the canvas of an editor (read from the canvas: the window of the OpenGL canvas is
 * not in the grab of the screen), once it is drawn.
 *
 * @return the number of the sampled pixels that are not of the color of the background (the one
 *         of the corner).
 */
int canvasDrawing( KIWAY_PLAYER* aFrame, QImage& aImage )
{
    auto* canvas = dynamic_cast<EDA_DRAW_PANEL_GAL*>( aFrame->GetToolCanvas() );
    int   drawn = 0;

    for( int attempt = 0; canvas && attempt < 20 && !drawn; ++attempt )
    {
        canvas->ForceRefresh();
        settle( 100 );

        if( !canvas->GetScreenshot( aImage ) || aImage.width() < 100 )
            continue;

        for( int y = 0; y < aImage.height(); y += 7 )
        {
            for( int x = 0; x < aImage.width(); x += 7 )
            {
                if( aImage.pixel( x, y ) != aImage.pixel( 0, 0 ) )
                    drawn++;
            }
        }
    }

    return drawn;
}


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


/// The shell: a board in the PCB editor, a schematic in the schematic editor, the library
/// editors, then the shell closes them.
void runScenario( MAIN_WINDOW* aWindow )
{
    std::printf( "KICAD-QT\n" );
    check( aWindow->isVisible(), "the shell is shown" );

    // A board: the PCB editor of KiCad opens it
    aWindow->OpenPath( s_board );
    settle( 1000 );

    KIWAY_PLAYER* pcb = s_kiway.Player( FRAME_PCB_EDITOR, false );
    check( pcb != nullptr, "the KIFACE of pcbnew with the GUI makes the PCB editor" );
    check( pcb && pcb->isVisible(), "the PCB editor is shown" );
    check( pcb && pcb->windowTitle().contains( QFileInfo( s_board ).completeBaseName() ),
           "the PCB editor has the board" );

    // The Python console of the PCB editor (_kipython_qtgui.kiface): a line of Python reads the
    // board of the editor
    if( pcb && pcb->GetToolManager() )
    {
        pcb->GetToolManager()->RunAction( "pcbnew.Control.showPythonConsole" );
        settle( 500 );

        KIWAY_PLAYER* console = s_kiway.Player( FRAME_PYTHON, false );
        check( console && console->isVisible(), "the PCB editor shows the Python console" );

        QLineEdit*      input = console ? console->findChild<QLineEdit*>() : nullptr;
        QPlainTextEdit* output = console ? console->findChild<QPlainTextEdit*>() : nullptr;
        check( input && output, "the console has its input and output" );

        if( input && output )
        {
            check( output->toPlainText().contains( QStringLiteral( "Python " ) ),
                   "the console shows the version of Python" );

            input->setText( QStringLiteral(
                    "print( 'footprints:', len( pcbnew.GetBoard().GetFootprints() ) )" ) );
            Q_EMIT input->returnPressed();
            settle( 100 );

            check( output->toPlainText().contains( QStringLiteral( "footprints: " ) ),
                   "the console runs Python with the module of pcbnew" );

            input->setText( QStringLiteral( "1 / 0" ) );
            Q_EMIT input->returnPressed();
            settle( 100 );

            check( output->toPlainText().contains( QStringLiteral( "ZeroDivisionError" ) ),
                   "the console shows the errors of Python" );
            std::printf( "%s\n", qPrintable( output->toPlainText().section( '\n', -6 ) ) );

            if( !s_screenshot.isEmpty() )
            {
                QFileInfo shot( s_screenshot );
                console->grab().save( shot.path() + QStringLiteral( "/" ) + shot.completeBaseName()
                                      + QStringLiteral( "-python." ) + shot.suffix() );
            }
        }

        // The console is hidden again (the action toggles it)
        pcb->GetToolManager()->RunAction( "pcbnew.Control.showPythonConsole" );
        settle( 200 );
        check( !console || !console->isVisible(), "the console is hidden" );
    }

    // A schematic of another project: the editors of the board's project close (as in the
    // project manager of KiCad), then the schematic editor of KiCad opens it
    KIWAY_PLAYER* sch = nullptr;

    if( !s_schematic.isEmpty() )
    {
        aWindow->OpenPath( s_schematic );
        settle( 1000 );

        check( !s_kiway.Player( FRAME_PCB_EDITOR, false ),
               "the PCB editor of the previous project is closed" );
        pcb = nullptr;

        sch = s_kiway.Player( FRAME_SCH, false );
        check( sch != nullptr, "the KIFACE of eeschema with the GUI makes the schematic editor" );
        check( sch && sch->isVisible(), "the schematic editor is shown" );
        check( sch && sch->windowTitle().contains( QFileInfo( s_schematic ).completeBaseName() ),
               "the schematic editor has the schematic" );
    }

    // The footprint assignment of the schematic: the schematic editor opens CvPcb (the frame of
    // _cvpcb_qtgui.kiface) and sends it the symbols of its netlist
    KIWAY_PLAYER* cvpcb = nullptr;

    if( sch && sch->GetToolManager() )
    {
        // The messages of CvPcb (the footprint library table of the test configuration is
        // empty: CvPcb says it) are answered
        QStringList messages;
        QTimer      answer;

        QObject::connect( &answer, &QTimer::timeout,
                          [&]()
                          {
                              auto* dialog =
                                      qobject_cast<QDialog*>( QApplication::activeModalWidget() );

                              if( !dialog )
                                  return;

                              messages << dialog->windowTitle();
                              std::printf( "    message: \"%s\"\n", qPrintable( dialog->windowTitle() ) );

                              if( !pressButton( dialog, QStringLiteral( "OK" ) ) )
                                  dialog->reject();
                          } );

        answer.start( 100 );
        sch->GetToolManager()->RunAction( "eeschema.EditorControl.assignFootprints" );
        settle( 1000 );
        answer.stop();
        settle( 200 );

        check( messages.size() <= 1, "CvPcb opens without errors other than the empty library "
                                     "table of the test" );

        cvpcb = s_kiway.Player( FRAME_CVPCB, false );
        check( cvpcb != nullptr, "the KIFACE of cvpcb with the GUI makes CvPcb" );
        check( cvpcb && cvpcb->isVisible(), "CvPcb is shown" );

        // Its lists: the libraries, the symbols of the schematic and the footprints
        int lists = 0;
        int filled = 0;

        if( cvpcb )
        {
            for( QAbstractItemView* view : cvpcb->findChildren<QAbstractItemView*>() )
            {
                if( !view->isVisible() || !view->model() )
                    continue;

                lists++;

                if( view->model()->rowCount() > 0 )
                    filled++;
            }
        }

        std::printf( "    CvPcb: %d lists, %d with rows\n", lists, filled );
        check( lists >= 3, "CvPcb shows its three lists" );
        check( filled >= 1, "CvPcb lists the symbols of the schematic" );

        if( cvpcb && !s_screenshot.isEmpty() )
        {
            QFileInfo shot( s_screenshot );
            cvpcb->grab().save( shot.path() + QStringLiteral( "/" ) + shot.completeBaseName()
                                + QStringLiteral( "-cvpcb." ) + shot.suffix() );
        }
    }

    // The launcher: the library editors
    LAUNCHER_PANEL* launcher = aWindow->findChild<LAUNCHER_PANEL*>();
    check( launcher != nullptr, "the shell has its launcher" );

    if( launcher )
    {
        Q_EMIT launcher->symbolEditorRequested();
        settle( 1000 );
        Q_EMIT launcher->footprintEditorRequested();
        settle( 1000 );
    }

    KIWAY_PLAYER* symbols = s_kiway.Player( FRAME_SCH_SYMBOL_EDITOR, false );
    check( symbols && symbols->isVisible(), "the launcher opens the symbol editor" );

    KIWAY_PLAYER* footprints = s_kiway.Player( FRAME_FOOTPRINT_EDITOR, false );
    check( footprints && footprints->isVisible(), "the launcher opens the footprint editor" );

    // The drawing sheet editor, with the default drawing sheet and its properties
    if( launcher )
    {
        Q_EMIT launcher->toolRequested( FRAME_PL_EDITOR );
        settle( 1000 );
    }

    KIWAY_PLAYER* drawingSheets = s_kiway.Player( FRAME_PL_EDITOR, false );
    check( drawingSheets && drawingSheets->isVisible(),
           "the launcher opens the drawing sheet editor" );

    if( drawingSheets )
    {
        QDockWidget* properties = nullptr;

        for( QDockWidget* dock : drawingSheets->findChildren<QDockWidget*>() )
        {
            if( dock->windowTitle() == QStringLiteral( "Properties" ) )
                properties = dock;
        }

        check( properties && properties->isVisible(),
               "the drawing sheet editor shows the properties" );

        // The canvas draws the drawing sheet
        QImage image;
        check( canvasDrawing( drawingSheets, image ) > 0,
               "the drawing sheet editor draws the drawing sheet" );

        if( !s_screenshot.isEmpty() )
        {
            QString path = s_screenshot;
            path.replace( QStringLiteral( ".png" ), QStringLiteral( "-pleditor.png" ) );
            check( image.save( path ), "screenshot of the drawing sheet editor" );
        }
    }

    // The Gerber viewer, from the launcher: it shows its layers and draws a Gerber file
    if( launcher )
    {
        Q_EMIT launcher->toolRequested( FRAME_GERBER );
        settle( 1000 );
    }

    KIWAY_PLAYER* gerbers = s_kiway.Player( FRAME_GERBER, false );
    check( gerbers && gerbers->isVisible(), "the launcher opens the Gerber viewer" );

    if( gerbers )
    {
        QDockWidget* layers = nullptr;

        for( QDockWidget* dock : gerbers->findChildren<QDockWidget*>() )
        {
            if( dock->windowTitle() == QStringLiteral( "Layers Manager" ) )
                layers = dock;
        }

        check( layers && layers->isVisible(), "the Gerber viewer shows its layers" );

        QImage empty;
        int    emptyDrawn = canvasDrawing( gerbers, empty );

        const std::vector<wxString> files( 1, wxString( QA_GERBER_FILE ) );
        check( gerbers->OpenProjectFiles( files ), "the Gerber viewer loads a Gerber file" );
        settle( 500 );

        QImage image;
        int    drawn = canvasDrawing( gerbers, image );
        std::printf( "    drawn: %d (empty: %d)\n", drawn, emptyDrawn );
        check( drawn > emptyDrawn + 100, "the Gerber viewer draws the Gerber file" );

        if( !s_screenshot.isEmpty() )
        {
            QString path = s_screenshot;
            path.replace( QStringLiteral( ".png" ), QStringLiteral( "-gerbview.png" ) );
            check( image.save( path ), "screenshot of the Gerber viewer" );
        }
    }

    // Only the shell and the editors are open (the progress of the loading is closed)
    int others = 0;

    for( QWidget* widget : QApplication::topLevelWidgets() )
    {
        if( widget->isVisible() && widget != aWindow && widget != pcb && widget != sch
            && widget != symbols && widget != footprints && widget != cvpcb
            && widget != drawingSheets && widget != gerbers )
        {
            std::printf( "    open: %s \"%s\"\n", widget->metaObject()->className(),
                         qPrintable( widget->windowTitle() ) );
            others++;
        }
    }

    check( others == 0, "no other window is left open" );

    if( !s_screenshot.isEmpty() )
    {
        QScreen* screen = QGuiApplication::primaryScreen();
        check( screen->grabWindow( 0 ).save( s_screenshot ), "screenshot of the screen" );
    }

    // The changes of the test are not saved: the editors are closed, then the shell
    check( s_kiway.PlayersClose( true ), "the shell closes the editors" );
    settle( 200 );
    check( !s_kiway.Player( FRAME_PCB_EDITOR, false ) && !s_kiway.Player( FRAME_SCH, false )
                   && !s_kiway.Player( FRAME_SCH_SYMBOL_EDITOR, false )
                   && !s_kiway.Player( FRAME_FOOTPRINT_EDITOR, false )
                   && !s_kiway.Player( FRAME_CVPCB, false )
                   && !s_kiway.Player( FRAME_PL_EDITOR, false )
                   && !s_kiway.Player( FRAME_GERBER, false ),
           "the editors are closed" );

    aWindow->close();
}


/// Run by the application of the program, once it exists: the start wizard of the new
/// configuration is finished, then the scenario runs in the event loop of the shell.
void startTest()
{
    QTimer* poll = new QTimer();

    QObject::connect( poll, &QTimer::timeout, poll,
                      [poll]()
                      {
                          for( QWidget* widget : QApplication::topLevelWidgets() )
                          {
                              QDialog* dialog = qobject_cast<QDialog*>( widget );

                              if( dialog && dialog->isVisible() && dynamic_cast<wxWizard*>( dialog ) )
                              {
                                  if( !pressButton( dialog, QStringLiteral( "Finish" ) ) )
                                      pressButton( dialog, QStringLiteral( "Next >" ) );

                                  return;
                              }
                          }

                          // The shell, in its event loop (not in the one of a dialog)
                          if( s_started || QThread::currentThread()->loopLevel() != 1 )
                              return;

                          for( QWidget* widget : QApplication::topLevelWidgets() )
                          {
                              if( MAIN_WINDOW* window = qobject_cast<MAIN_WINDOW*>( widget ) )
                              {
                                  if( window->isVisible() )
                                  {
                                      s_started = true;
                                      poll->stop();
                                      poll->deleteLater();
                                      runScenario( window );
                                      return;
                                  }
                              }
                          }
                      } );

    poll->start( 100 );
}

Q_COREAPP_STARTUP_FUNCTION( startTest )
} // namespace


int main( int argc, char** argv )
{
    if( argc < 2 )
    {
        std::fprintf( stderr, "usage: %s <board.kicad_pcb> [schematic.kicad_sch] [screenshot.png]\n",
                      argv[0] );
        return 2;
    }

    // A new configuration of the test (the start wizard runs)
    QTemporaryDir configHome;

    if( !qEnvironmentVariableIsSet( "KICAD_CONFIG_HOME" ) )
        qputenv( "KICAD_CONFIG_HOME", configHome.path().toUtf8() );

    // The board is opened from a copy (the editor writes the project file next to it)
    QTemporaryDir dir;
    s_board = dir.filePath( QFileInfo( QString::fromUtf8( argv[1] ) ).fileName() );
    QFile::copy( QString::fromUtf8( argv[1] ), s_board );

    // The other arguments, by their extension
    for( int i = 2; i < argc; ++i )
    {
        const QString arg = QString::fromUtf8( argv[i] );

        if( arg.endsWith( QStringLiteral( ".kicad_sch" ) ) )
        {
            // Copied too, with its project
            s_schematic = dir.filePath( QFileInfo( arg ).fileName() );
            QFile::copy( arg, s_schematic );

            const QFileInfo source( arg );
            const QString   project = source.path() + QLatin1Char( '/' ) + source.completeBaseName()
                                    + QStringLiteral( ".kicad_pro" );

            if( QFile::exists( project ) )
                QFile::copy( project, dir.filePath( QFileInfo( project ).fileName() ) );
        }
        else
        {
            s_screenshot = arg;
        }
    }

    // The program, with its board (its arguments are the test's ones, without the board)
    char* programArgv[] = { argv[0], nullptr };
    int   ret = kicad_qt_main( 1, programArgv );

    check( s_started, "the scenario ran" );
    check( ret == 0, "the program ends" );

    std::printf( "%s\n", s_failures ? "FAILED" : "PASSED" );
    return s_failures ? 1 : 0;
}
