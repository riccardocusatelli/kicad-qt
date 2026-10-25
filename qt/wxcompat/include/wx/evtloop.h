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
 * @file evtloop.h
 * The event loop (wxEventLoopBase) of the wxWidgets compatibility layer: the event loop of Qt.
 */

#pragma once

#include <QCoreApplication>
#include <QThread>


class wxEventLoopBase
{
public:
    virtual ~wxEventLoopBase() = default;

    /// The running event loop, or nullptr (before the main loop runs, or outside the main
    /// thread).
    static wxEventLoopBase* GetActive()
    {
        static wxEventLoopBase s_loop;
        QCoreApplication* app = QCoreApplication::instance();

        if( !app || QThread::currentThread() != app->thread()
            || QThread::currentThread()->loopLevel() == 0 )
        {
            return nullptr;
        }

        return &s_loop;
    }

    bool IsRunning() const { return true; }

    /// Process the pending events.
    bool Pending() const { return true; }
    bool Dispatch()
    {
        QCoreApplication::processEvents();
        return true;
    }

    void YieldFor( long aEventsToProcess ) { QCoreApplication::processEvents(); }
};


using wxEventLoop = wxEventLoopBase;
using wxGUIEventLoop = wxEventLoopBase;
