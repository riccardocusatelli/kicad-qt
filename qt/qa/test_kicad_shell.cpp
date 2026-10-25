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
 * of KiCad (the frame of _pcbnew_qtgui.kiface), the launcher opens the footprint editor, and the
 * shell closes the editors with it.
 *
 * Usage: qa_qt_kicad_shell <board.kicad_pcb> [screenshot.png]
 */

// The program, its main() renamed (the test runs it)
#define main kicad_qt_main
#include "../kicad/kicad_qt.cpp"
#undef main

#include <cstdio>

#include <QCoreApplication>
#include <QDialog>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QPushButton>
#include <QScreen>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>

#include <wx/wizard.h>

#include <frame_type.h>
#include <kiway_player.h>

#include "../kicad/home_widgets.h"


namespace
{
int     s_failures = 0;
QString s_board;
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


/// The shell: a board in the PCB editor, the footprint editor, then the shell closes them.
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

    // The launcher: the footprint editor
    LAUNCHER_PANEL* launcher = aWindow->findChild<LAUNCHER_PANEL*>();
    check( launcher != nullptr, "the shell has its launcher" );

    if( launcher )
        Q_EMIT launcher->footprintEditorRequested();

    settle( 1000 );

    KIWAY_PLAYER* footprints = s_kiway.Player( FRAME_FOOTPRINT_EDITOR, false );
    check( footprints && footprints->isVisible(), "the launcher opens the footprint editor" );

    // Only the shell and the editors are open (the progress of the loading is closed)
    int others = 0;

    for( QWidget* widget : QApplication::topLevelWidgets() )
    {
        if( widget->isVisible() && widget != aWindow && widget != pcb && widget != footprints )
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
    check( !s_kiway.Player( FRAME_PCB_EDITOR, false )
                   && !s_kiway.Player( FRAME_FOOTPRINT_EDITOR, false ),
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
        std::fprintf( stderr, "usage: %s <board.kicad_pcb> [screenshot.png]\n", argv[0] );
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

    if( argc > 2 )
        s_screenshot = QString::fromUtf8( argv[2] );

    // The program, with its board (its arguments are the test's ones, without the board)
    char* programArgv[] = { argv[0], nullptr };
    int   ret = kicad_qt_main( 1, programArgv );

    check( s_started, "the scenario ran" );
    check( ret == 0, "the program ends" );

    std::printf( "%s\n", s_failures ? "FAILED" : "PASSED" );
    return s_failures ? 1 : 0;
}
