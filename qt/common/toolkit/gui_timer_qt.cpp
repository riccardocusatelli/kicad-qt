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


#include <toolkit/gui_timer.h>

#include <QTimer>


class GUI_TIMER::IMPL
{
public:
    QTimer                m_timer;
    std::function<void()> m_handler;
};


GUI_TIMER::GUI_TIMER( std::function<void()> aHandler ) :
        m_impl( std::make_unique<IMPL>() )
{
    m_impl->m_handler = std::move( aHandler );

    QObject::connect( &m_impl->m_timer, &QTimer::timeout,
                      [this]()
                      {
                          if( m_impl->m_handler )
                              m_impl->m_handler();
                      } );
}


GUI_TIMER::~GUI_TIMER() = default;


void GUI_TIMER::SetHandler( std::function<void()> aHandler )
{
    m_impl->m_handler = std::move( aHandler );
}


void GUI_TIMER::Start( int aMilliseconds )
{
    m_impl->m_timer.setSingleShot( false );
    m_impl->m_timer.start( aMilliseconds );
}


void GUI_TIMER::StartOnce( int aMilliseconds )
{
    m_impl->m_timer.setSingleShot( true );
    m_impl->m_timer.start( aMilliseconds );
}


void GUI_TIMER::Stop()
{
    m_impl->m_timer.stop();
}


bool GUI_TIMER::IsRunning() const
{
    return m_impl->m_timer.isActive();
}
