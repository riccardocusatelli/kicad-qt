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
 * @file qa_qt_menus.h
 * Every item of the menus of a frame, chosen one after the other as a user would: the windows
 * it opens (dialogs, message boxes, file dialogs, other frames) are checked and closed, the
 * frame and the program survive.  A dialog fits its content and the screen.
 */

#ifndef QA_QT_MENUS_H
#define QA_QT_MENUS_H

#include <cstdio>
#include <map>
#include <set>
#include <string>

#include <functional>

#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QElapsedTimer>
#include <QPushButton>
#include <QDockWidget>
#include <QMenu>
#include <QMenuBar>
#include <QPointer>
#include <QScreen>
#include <QTimer>

#include <kiway_player.h>
#include <tool/actions.h>
#include <tool/tool_manager.h>


namespace QA_QT_MENUS
{

/// The check of the test: its result is printed and counted by the test.
using CHECK = std::function<void( bool, const char* )>;


/// Let the event loop run.
inline void settle( int aMilliseconds )
{
    QElapsedTimer timer;
    timer.start();

    while( timer.elapsed() < aMilliseconds )
        QCoreApplication::processEvents( QEventLoop::AllEvents, 10 );
}


/// Press a button of a dialog, by its label.
inline bool pressButton( QWidget* aDialog, const QString& aLabel )
{
    for( QPushButton* button : aDialog->findChildren<QPushButton*>() )
    {
        if( button->isVisible() && button->isEnabled()
                && button->text().remove( QLatin1Char( '&' ) ) == aLabel )
        {
            button->click();
            return true;
        }
    }

    return false;
}


/// The label of a menu item, without its mnemonic, its shortcut and its ellipsis.
inline QString itemLabel( const QAction* aAction )
{
    QString label = aAction->text();
    label = label.section( QLatin1Char( '\t' ), 0, 0 );
    label.remove( QLatin1Char( '&' ) );
    label = label.trimmed();

    if( label.endsWith( QStringLiteral( "..." ) ) )
        label.chop( 3 );
    else if( label.endsWith( QChar( 0x2026 ) ) )
        label.chop( 1 );

    return label.trimmed();
}


/// The windows of the program on the screen (not the menus, tooltips and floating panels).
inline std::set<QWidget*> shownWindows()
{
    std::set<QWidget*> windows;

    for( QWidget* widget : QApplication::topLevelWidgets() )
    {
        if( !widget->isVisible() || qobject_cast<QMenu*>( widget )
                || qobject_cast<QDockWidget*>( widget ) )
        {
            continue;
        }

        Qt::WindowType type = widget->windowType();

        if( type == Qt::Popup || type == Qt::ToolTip || type == Qt::SplashScreen )
            continue;

        windows.insert( widget );
    }

    return windows;
}


/**
 * The windows opened by the items: each one is checked once (a dialog fits its content and the
 * screen), then closed: Cancel or Close first, then the answers that change nothing (No,
 * Discard), then OK; at the end rejected, hidden.
 */
class WINDOW_WATCH
{
public:
    WINDOW_WATCH( const CHECK& aCheck, const QString& aScreenshot, const std::string& aFrame ) :
            m_check( aCheck ),
            m_screenshot( aScreenshot ),
            m_frame( aFrame )
    {
        QObject::connect( &m_timer, &QTimer::timeout, [this]() { poll(); } );
        m_timer.start( 50 );
    }

    /// The windows already there (the frame, the frames opened before): not the item's.
    void SetKnown( const std::set<QWidget*>& aKnown )
    {
        m_known.clear();

        for( QWidget* widget : aKnown )
            m_known.emplace_back( widget );

        m_attempts.clear();
        m_seen.clear();
        m_inspected.clear();
        m_item.clear();
    }

    void SetItem( const QString& aItem ) { m_item = aItem; }

    /**
     * The item runs (its action has not returned): a tool waiting for the user (an interactive
     * move, waiting for a click) is cancelled, as with Escape.
     */
    void SetRunning( TOOL_MANAGER* aToolMgr )
    {
        m_running = aToolMgr;
        m_runTimer.start();
        m_cancels = 0;
    }

    int Cancels() const { return m_cancels; }

    /// The windows of the item still open.
    int Open() const
    {
        int count = 0;

        for( QWidget* widget : shownWindows() )
        {
            if( !isKnown( widget ) )
                count++;
        }

        return count;
    }

    int Dialogs() const { return m_dialogs; }
    int Bad() const { return m_bad; }

private:
    bool isKnown( QWidget* aWidget ) const
    {
        for( const QPointer<QWidget>& known : m_known )
        {
            if( known.data() == aWidget )
                return true;
        }

        return false;
    }

    void poll()
    {
        // Not again from a dialog the poll closes (its event loop runs this timer)
        if( m_busy )
            return;

        m_busy = true;

        for( QWidget* widget : shownWindows() )
        {
            if( isKnown( widget ) )
                continue;

            int& attempts = m_attempts[widget];

            // Checked once it has its size (a dialog sizes itself once shown, with its posted
            // size event): 200 ms after it is shown, as the user sees it
            if( !m_inspected.count( widget ) )
            {
                auto [seen, first] = m_seen.try_emplace( widget, m_ticks );

                if( m_ticks - seen->second < 4 )
                    continue;

                m_inspected.insert( widget );
                inspect( widget );
            }

            // A frame closes as the user closes it: not while a modal dialog is open (its
            // window takes no input) nor while the item's action runs (its tool would be
            // deleted under it), but a frame shown as a modal (a chooser) is what the action
            // waits for
            KIWAY_PLAYER* player = dynamic_cast<KIWAY_PLAYER*>( widget );
            bool          modalFrame = player && player->IsModal();

            if( !qobject_cast<QDialog*>( widget ) && !modalFrame
                    && ( m_running || QApplication::activeModalWidget() ) )
            {
                continue;
            }

            // A frame that closes asks its questions (a dialog of its own): one try a second
            if( attempts > 0 && ( m_ticks - m_lastTry[widget] ) < 20 )
                continue;

            m_lastTry[widget] = m_ticks;
            closeWindow( widget, attempts++ );
            m_runTimer.restart();
        }

        // A dialog hidden in its event loop (closed before its loop started, as when it asks
        // its question while it is made): it ends as Cancel
        if( QDialog* modal = qobject_cast<QDialog*>( QApplication::activeModalWidget() ) )
        {
            if( !modal->isVisible() && !isKnown( modal ) && ( m_ticks % 20 ) == 0 )
            {
                std::printf( "    hidden dialog \"%s\": cancelled\n",
                             qPrintable( modal->windowTitle() ) );
                // Shown again, so that closing it ends its event loop
                modal->show();
                modal->done( QDialog::Rejected );
                m_runTimer.restart();
            }
        }

        // No dialog, the action still runs: a tool waits for the user
        if( m_running && !QApplication::activeModalWidget() && m_runTimer.elapsed() > 1500
                && m_cancels < 10 )
        {
            std::printf( "    the tool waits: cancelled\n" );
            m_cancels++;
            m_runTimer.restart();
            m_running->RunAction( ACTIONS::cancelInteractive );
        }

        m_ticks++;
        m_busy = false;
    }

    void inspect( QWidget* aWindow )
    {
        QDialog* dialog = qobject_cast<QDialog*>( aWindow );
        QString  title = aWindow->windowTitle();

        if( !dialog )
        {
            std::printf( "    window \"%s\" (%s)\n", title.toUtf8().constData(),
                         aWindow->metaObject()->className() );
            return;
        }

        m_dialogs++;

        // Laid out, as the user sees it
        QCoreApplication::processEvents();

        QSize   content = dialog->minimumSizeHint();
        QRect   frame = dialog->frameGeometry();
        QScreen* screen = dialog->screen();
        QRect   available = screen ? screen->availableGeometry() : frame;

        bool fitsContent = dialog->width() >= content.width()
                           && dialog->height() >= content.height();
        bool fitsScreen = frame.width() <= available.width()
                          && frame.height() <= available.height();

        std::printf( "    dialog \"%s\" (%s) %dx%d\n", title.toUtf8().constData(),
                     dialog->metaObject()->className(), dialog->width(), dialog->height() );

        if( !fitsContent || !fitsScreen )
        {
            m_bad++;

            std::string what = m_frame + ", " + m_item.toStdString() + ": the dialog \""
                               + title.toStdString() + "\" fits "
                               + ( fitsContent ? "the screen" : "its content" );

            if( !fitsContent )
                std::printf( "    size %dx%d, content %dx%d\n", dialog->width(),
                             dialog->height(), content.width(), content.height() );

            if( !fitsScreen )
                std::printf( "    frame %dx%d, screen %dx%d\n", frame.width(), frame.height(),
                             available.width(), available.height() );

            m_check( false, what.c_str() );

            if( !m_screenshot.isEmpty() )
            {
                QString path = m_screenshot;
                path.replace( QStringLiteral( ".png" ),
                              QStringLiteral( "-menu-%1.png" ).arg( m_bad ) );
                dialog->grab().save( path );
            }
        }
    }

    void closeWindow( QWidget* aWindow, int aAttempt )
    {
        QPointer<QWidget> window( aWindow );

        if( QDialog* dialog = qobject_cast<QDialog*>( aWindow ) )
        {
            static const char* const answers[][6] = {
                { "Cancel", "Close", nullptr },
                { "No", "Don't Save", "Discard Changes", "Discard", "Skip", nullptr },
                { "OK", "Yes", "Continue", nullptr },
            };

            if( aAttempt < 3 )
            {
                for( const char* const* answer = answers[aAttempt]; *answer; ++answer )
                {
                    if( pressButton( dialog, QString::fromUtf8( *answer ) ) )
                        return;
                }

                if( aAttempt == 0 && window )
                    dialog->reject();
            }
            else if( aAttempt == 3 )
            {
                dialog->done( QDialog::Rejected );
            }
            else
            {
                dialog->hide();
            }

            return;
        }

        // A frame (a viewer, an editor, a modal chooser): closed as the user would
        if( KIWAY_PLAYER* player = dynamic_cast<KIWAY_PLAYER*>( aWindow ) )
        {
            // Its close button (the frame ends its modal loop itself); forced, then dismissed
            if( aAttempt < 3 )
                player->Close( aAttempt > 0 );
            else if( player->IsModal() )
                player->DismissModal( false );

            return;
        }

        if( aAttempt < 2 )
            aWindow->close();
        else
            aWindow->hide();
    }

    QTimer                         m_timer;
    QElapsedTimer                  m_runTimer;
    TOOL_MANAGER*                  m_running = nullptr;
    int                            m_cancels = 0;
    CHECK                          m_check;
    QString                        m_screenshot;
    std::string                    m_frame;
    QString                        m_item;
    std::vector<QPointer<QWidget>> m_known;
    std::map<QWidget*, int>        m_attempts;
    std::map<QWidget*, int>        m_lastTry;
    std::map<QWidget*, int>        m_seen;
    std::set<QWidget*>             m_inspected;
    int                            m_ticks = 0;
    int                            m_dialogs = 0;
    int                            m_bad = 0;
    bool                           m_busy = false;
};


/// The items of a menu and of its submenus (each menu updated, as when it is shown).
inline void menuItems( QMenu* aMenu, const QString& aPath,
                       std::vector<std::pair<QString, QPointer<QAction>>>& aItems )
{
    Q_EMIT aMenu->aboutToShow();

    for( QAction* action : aMenu->actions() )
    {
        if( action->isSeparator() || !action->isVisible() )
            continue;

        QString path = aPath + QStringLiteral( " > " ) + itemLabel( action );

        if( QMenu* submenu = action->menu() )
            menuItems( submenu, path, aItems );
        else
            aItems.emplace_back( path, QPointer<QAction>( action ) );
    }

    Q_EMIT aMenu->aboutToHide();
}


/**
 * Every item of the menus of @a aFrame, chosen.  @a aSkip: the items not chosen (their path,
 * "File > Save", or the start of it, "Help").  The ones that end the frame, write its files or
 * start other programs are not chosen.
 */
inline void testMenus( const CHECK& check, EDA_BASE_FRAME* aFrame, const char* aName,
                       const QString& aScreenshot, const QStringList& aSkip = {} )
{
    std::printf( "MENUS of %s\n", aName );

    QPointer<QWidget> frame( aFrame );
    QMenuBar*         menuBar = aFrame->findChild<QMenuBar*>();

    check( menuBar != nullptr, ( std::string( aName ) + " has its menus" ).c_str() );

    if( !menuBar )
        return;

    // What a test does not do: end the program, write the files of the user, print, open the
    // web pages of the help, start other programs
    QStringList skip = aSkip;
    skip << QStringLiteral( "File > Quit" ) << QStringLiteral( "File > Exit" )
         << QStringLiteral( "File > Close" ) << QStringLiteral( "File > Save" )
         << QStringLiteral( "File > Print" ) << QStringLiteral( "File > Page Setup" )
         << QStringLiteral( "File > Open Recent" ) << QStringLiteral( "File > Revert" )
         << QStringLiteral( "Preferences > Set Language" )
         << QStringLiteral( "Help > Help" ) << QStringLiteral( "Help > Getting Started" )
         << QStringLiteral( "Help > Report" ) << QStringLiteral( "Help > Get Involved" )
         << QStringLiteral( "Help > Donate" ) << QStringLiteral( "Help > Check for Updates" );

    // The items of the menus, by their path: found again when a frame makes its menus again (a
    // change of its settings)
    auto collect = [&]()
    {
        std::vector<std::pair<QString, QPointer<QAction>>> items;

        if( QMenuBar* bar = frame ? frame->findChild<QMenuBar*>() : nullptr )
        {
            for( QAction* top : bar->actions() )
            {
                if( QMenu* menu = top->menu() )
                    menuItems( menu, itemLabel( top ), items );
            }
        }

        return items;
    };

    std::vector<std::pair<QString, QPointer<QAction>>> items = collect();
    std::map<QString, QPointer<QAction>>                current( items.begin(), items.end() );

    WINDOW_WATCH watch( check, aScreenshot, aName );
    int          chosen = 0;
    int          left = 0;
    int          gone = 0;
    int          missing = 0;
    TOOL_MANAGER* toolMgr = aFrame->GetToolManager();

    for( const auto& [path, first] : items )
    {
        bool skipped = false;

        for( const QString& start : skip )
        {
            if( path.startsWith( start ) )
                skipped = true;
        }

        if( skipped )
            continue;

        QPointer<QAction> action = current[path];

        // The menus were made again
        if( !action )
        {
            gone++;

            for( const auto& [newPath, newAction] : collect() )
                current[newPath] = newAction;

            action = current[path];
        }
        else if( QMenu* menu = qobject_cast<QMenu*>( action->parent() ) )
        {
            // Updated as when it is shown (enabled, checked)
            Q_EMIT menu->aboutToShow();
            Q_EMIT menu->aboutToHide();
        }

        if( !action )
        {
            std::printf( "    menu %s: gone\n", path.toUtf8().constData() );
            missing++;
            continue;
        }

        if( !action->isEnabled() )
            continue;

        std::printf( "    menu %s\n", path.toUtf8().constData() );

        watch.SetKnown( shownWindows() );
        watch.SetItem( path );
        chosen++;

        watch.SetRunning( toolMgr );
        action->trigger();
        watch.SetRunning( nullptr );
        settle( 300 );

        // The windows of the item closed (a frame may ask before it closes)
        for( int i = 0; i < 100 && watch.Open() > 0; ++i )
            settle( 100 );

        if( watch.Open() > 0 )
        {
            left++;
            check( false, ( std::string( aName ) + ", " + path.toStdString()
                            + ": its windows close" ).c_str() );
        }

        if( !frame )
        {
            check( false, ( std::string( aName ) + " survives " + path.toStdString() ).c_str() );
            return;
        }

        // A tool the item started (a drawing tool) is cancelled
        if( toolMgr )
        {
            toolMgr->RunAction( ACTIONS::cancelInteractive );
            settle( 50 );
        }
    }

    std::printf( "    %d items chosen, %d dialogs, menus made again %d times\n", chosen,
                 watch.Dialogs(), gone );
    // A small frame (the calculator) has a few items; each one is still there when its turn comes
    check( chosen > 0 && missing == 0,
           ( std::string( aName ) + ": the items of the menus are chosen" ).c_str() );
    check( frame && watch.Bad() == 0 && left == 0,
           ( std::string( aName ) + ": the dialogs of the menus fit, their windows close" )
                   .c_str() );
}


} // namespace QA_QT_MENUS

#endif // QA_QT_MENUS_H
