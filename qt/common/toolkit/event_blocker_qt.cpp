/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <toolkit/event_blocker.h>

#include <QEvent>
#include <QPointer>
#include <QWidget>


namespace
{
/// Eat the input events of a window.
class INPUT_FILTER : public QObject
{
public:
    bool eventFilter( QObject* aWatched, QEvent* aEvent ) override
    {
        switch( aEvent->type() )
        {
        case QEvent::KeyPress:
        case QEvent::KeyRelease:
        case QEvent::ShortcutOverride:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseMove:
        case QEvent::Wheel:
        case QEvent::ContextMenu:
        case QEvent::TabletPress:
        case QEvent::TabletRelease:
        case QEvent::TabletMove:
        case QEvent::TouchBegin:
        case QEvent::TouchUpdate:
        case QEvent::TouchEnd:
        case QEvent::Drop:
        case QEvent::DragEnter:
        case QEvent::DragMove:
            return true;

        default:
            return false;
        }
    }
};
}


class EVENT_BLOCKER::IMPL
{
public:
    IMPL( QWidget* aWindow ) :
            m_window( aWindow )
    {
        if( m_window )
            m_window->installEventFilter( &m_filter );
    }

    ~IMPL()
    {
        if( m_window )
            m_window->removeEventFilter( &m_filter );
    }

private:
    QPointer<QWidget> m_window;
    INPUT_FILTER      m_filter;
};


EVENT_BLOCKER::EVENT_BLOCKER( TOOLKIT_WINDOW* aWindow ) :
        m_impl( std::make_unique<IMPL>( aWindow ) )
{
}


EVENT_BLOCKER::~EVENT_BLOCKER() = default;
