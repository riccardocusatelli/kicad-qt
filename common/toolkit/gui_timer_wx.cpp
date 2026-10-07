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

#include <wx/timer.h>


class GUI_TIMER::IMPL : public wxTimer
{
public:
    void Notify() override
    {
        if( m_handler )
            m_handler();
    }

    std::function<void()> m_handler;
};


GUI_TIMER::GUI_TIMER( std::function<void()> aHandler ) :
        m_impl( std::make_unique<IMPL>() )
{
    m_impl->m_handler = std::move( aHandler );
}


GUI_TIMER::~GUI_TIMER() = default;


void GUI_TIMER::SetHandler( std::function<void()> aHandler )
{
    m_impl->m_handler = std::move( aHandler );
}


void GUI_TIMER::Start( int aMilliseconds )
{
    m_impl->Start( aMilliseconds, wxTIMER_CONTINUOUS );
}


void GUI_TIMER::StartOnce( int aMilliseconds )
{
    m_impl->StartOnce( aMilliseconds );
}


void GUI_TIMER::Stop()
{
    m_impl->Stop();
}


bool GUI_TIMER::IsRunning() const
{
    return m_impl->IsRunning();
}
