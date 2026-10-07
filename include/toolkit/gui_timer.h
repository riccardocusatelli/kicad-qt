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

#include <functional>
#include <memory>

#include <import_export.h>


/**
 * A timer of the user interface: calls its handler from the event loop of the GUI toolkit,
 * once or repeatedly.
 *
 * The tools use it instead of the timer of a toolkit; each GUI build (wxWidgets, Qt) has its
 * own implementation (common/toolkit/gui_timer_wx.cpp, qt/common/toolkit/gui_timer_qt.cpp).
 */
class APIVISIBLE GUI_TIMER
{
public:
    GUI_TIMER( std::function<void()> aHandler = nullptr );
    ~GUI_TIMER();

    GUI_TIMER( const GUI_TIMER& ) = delete;
    GUI_TIMER& operator=( const GUI_TIMER& ) = delete;

    /// The function called when the timer expires.
    void SetHandler( std::function<void()> aHandler );

    /// Start (or restart) the timer, which expires every @a aMilliseconds.
    void Start( int aMilliseconds );

    /// Start (or restart) the timer, which expires once after @a aMilliseconds.
    void StartOnce( int aMilliseconds );

    void Stop();

    bool IsRunning() const;

private:
    class IMPL;
    std::unique_ptr<IMPL> m_impl;
};
