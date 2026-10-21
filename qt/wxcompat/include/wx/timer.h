/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file timer.h
 * The timers of the wxWidgets compatibility layer (wxTimer, wxTimerEvent): a QTimer that sends
 * wxEVT_TIMER to the owner of the timer (the timer itself by default, as wxWidgets).
 */

#pragma once

#include <wx/kiwx_events.h>

#include <memory>

#include <QTimer>


/// The timers end (wxEVT_TIMER).
inline const wxEventType wxEVT_TIMER = wxNewEventType();

#define wxTIMER_CONTINUOUS false
#define wxTIMER_ONE_SHOT   true


class wxTimer;


class wxTimerEvent : public wxEvent
{
public:
    explicit wxTimerEvent( wxTimer* aTimer = nullptr );

    wxTimer& GetTimer() const { return *m_timer; }
    int GetInterval() const;

private:
    wxTimer* m_timer;
};


class wxTimer : public wxEvtHandler
{
public:
    wxTimer() :
            m_owner( this )
    {
        init();
    }

    explicit wxTimer( wxEvtHandler* aOwner, int aId = wxID_ANY ) :
            m_owner( aOwner ),
            m_id( aId )
    {
        init();
    }

    ~wxTimer() override { Stop(); }

    wxTimer( const wxTimer& ) = delete;
    wxTimer& operator=( const wxTimer& ) = delete;

    void SetOwner( wxEvtHandler* aOwner, int aId = wxID_ANY )
    {
        m_owner = aOwner;
        m_id = aId;
    }

    wxEvtHandler* GetOwner() const { return m_owner; }
    int GetId() const { return m_id; }

    /// Start the timer, for @a aMilliseconds (the previous interval if < 0).
    bool Start( int aMilliseconds = -1, bool aOneShot = wxTIMER_CONTINUOUS )
    {
        if( aMilliseconds >= 0 )
            m_interval = aMilliseconds;

        m_timer->setSingleShot( aOneShot );
        m_timer->start( m_interval );
        return true;
    }

    bool StartOnce( int aMilliseconds = -1 ) { return Start( aMilliseconds, wxTIMER_ONE_SHOT ); }

    void Stop() { m_timer->stop(); }

    bool IsRunning() const { return m_timer->isActive(); }
    bool IsOneShot() const { return m_timer->isSingleShot(); }
    int GetInterval() const { return m_interval; }

    /// The timer ends: the owner gets wxEVT_TIMER.
    virtual void Notify()
    {
        if( m_owner )
        {
            wxTimerEvent event( this );
            m_owner->ProcessEvent( event );
        }
    }

private:
    void init()
    {
        m_timer = std::make_unique<QTimer>();
        QObject::connect( m_timer.get(), &QTimer::timeout,
                          [this]()
                          {
                              Notify();
                          } );
    }

    std::unique_ptr<QTimer> m_timer;
    wxEvtHandler*           m_owner;
    int                     m_id = wxID_ANY;
    int                     m_interval = 0;
};


inline wxTimerEvent::wxTimerEvent( wxTimer* aTimer ) :
        wxEvent( aTimer ? aTimer->GetId() : wxID_ANY, wxEVT_TIMER ),
        m_timer( aTimer )
{
    SetEventObject( aTimer );
}


inline int wxTimerEvent::GetInterval() const
{
    return m_timer ? m_timer->GetInterval() : 0;
}
