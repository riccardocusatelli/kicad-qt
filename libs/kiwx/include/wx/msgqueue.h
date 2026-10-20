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

#pragma once

// wxMessageQueue of wxWidgets 3.2: a queue of messages between threads, on the mutex and the
// condition variable of the standard library

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>

enum wxMessageQueueError
{
    wxMSGQUEUE_NO_ERROR = 0,
    wxMSGQUEUE_TIMEOUT,
    wxMSGQUEUE_MISC_ERROR
};


template <typename T>
class wxMessageQueue
{
public:
    typedef T Message;

    wxMessageQueue() = default;

    wxMessageQueue( const wxMessageQueue& ) = delete;
    wxMessageQueue& operator=( const wxMessageQueue& ) = delete;

    wxMessageQueueError Post( const Message& aMsg )
    {
        {
            std::lock_guard<std::mutex> lock( m_mutex );
            m_messages.push( aMsg );
        }

        m_condition.notify_one();
        return wxMSGQUEUE_NO_ERROR;
    }

    wxMessageQueueError Clear()
    {
        std::lock_guard<std::mutex> lock( m_mutex );
        m_messages = std::queue<T>();
        return wxMSGQUEUE_NO_ERROR;
    }

    // Wait at most aTimeout milliseconds for a message
    wxMessageQueueError ReceiveTimeout( long aTimeout, T& aMsg )
    {
        std::unique_lock<std::mutex> lock( m_mutex );

        if( !m_condition.wait_for( lock, std::chrono::milliseconds( aTimeout ),
                                   [this]() { return !m_messages.empty(); } ) )
        {
            return wxMSGQUEUE_TIMEOUT;
        }

        aMsg = m_messages.front();
        m_messages.pop();
        return wxMSGQUEUE_NO_ERROR;
    }

    wxMessageQueueError Receive( T& aMsg )
    {
        std::unique_lock<std::mutex> lock( m_mutex );
        m_condition.wait( lock, [this]() { return !m_messages.empty(); } );

        aMsg = m_messages.front();
        m_messages.pop();
        return wxMSGQUEUE_NO_ERROR;
    }

    bool IsOk() const { return true; }

private:
    std::mutex              m_mutex;
    std::condition_variable m_condition;
    std::queue<T>           m_messages;
};
