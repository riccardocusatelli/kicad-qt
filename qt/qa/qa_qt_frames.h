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
 * @file qa_qt_frames.h
 * What the tests of the frames of the editors on Qt share: the checks, the event loop, the
 * dialogs opened by the actions, the frames made by the KIWAY and the program of the test.
 */

#ifndef QA_QT_FRAMES_H
#define QA_QT_FRAMES_H

#include <cstdio>
#include <functional>
#include <string>
#include <kiqt.h>     // Qt, without its METHOD macro
#include <QApplication>
#include <QDialog>
#include <QElapsedTimer>
#include <QPointer>
#include <QPushButton>
#include <QTimer>

#include <api/api_server.h>
#include <eda_base_frame.h>
#include <frame_type.h>
#include <kiway.h>
#include <pgm_gui.h>
#include <settings/settings_manager.h>
#include <startwizard/startwizard.h>
#include <wx/filename.h>
#include <wx/wizard.h>
#include <tool/tool_action.h>
#include <tool/tool_manager.h>


namespace QA_QT_FRAMES
{

inline int s_failures = 0;


inline void check( bool aOk, const char* aWhat )
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
inline void settle( int aMilliseconds = 50 )
{
    QElapsedTimer timer;
    timer.start();

    while( timer.elapsed() < aMilliseconds )
        QCoreApplication::processEvents( QEventLoop::AllEvents, 10 );
}


/// Run a function on the next dialog that opens (modal, or quasi-modal: a dialog with its own
/// event loop, which is not a modal widget).  @return the poll of the dialogs (deleted once the
/// dialog is found).
inline QTimer* onNextDialog( std::function<void( QDialog* )> aFunction )
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
inline bool pressButton( QWidget* aDialog, const QString& aLabel )
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
inline void testDialog( TOOL_MANAGER* aToolMgr, const TOOL_ACTION& aAction, const char* aName,
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


/// A frame of the KIFACE, made by the KIWAY (its player: the frames find each other).
template <typename FRAME>
FRAME* createFrame( KIWAY& aKiway, FRAME_T aFrameType )
{
    return dynamic_cast<FRAME*>( aKiway.Player( aFrameType, true ) );
}


/// Close a frame, then delete it (its deferred delete: the event loop of the program would).
inline void closeFrame( EDA_BASE_FRAME* aFrame, const char* aName )
{
    QPointer<QWidget> alive( aFrame );

    aFrame->Close( true );
    settle( 100 );
    QCoreApplication::sendPostedEvents( nullptr, QEvent::DeferredDelete );

    check( alive.isNull(), ( std::string( aName ) + " is closed and deleted" ).c_str() );
}


/**
 * The start wizard of a new configuration (the settings of the test are new): its pages are
 * shown, the wizard is finished, and it makes the global library tables.
 */
inline void testStartWizard( const QString& aScreenshot )
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


} // namespace QA_QT_FRAMES

#endif // QA_QT_FRAMES_H
