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

#pragma once

#include <memory>

#include <import_export.h>
#include <toolkit/toolkit_types.h>


/**
 * Block the events of the user (mouse, keyboard) to a window while it lives, as when another
 * window is being opened over it.
 *
 * Each GUI build (wxWidgets, Qt) has its own implementation (common/toolkit/event_blocker_wx.cpp,
 * qt/common/toolkit/event_blocker_qt.cpp).
 */
class APIVISIBLE EVENT_BLOCKER
{
public:
    EVENT_BLOCKER( TOOLKIT_WINDOW* aWindow );
    ~EVENT_BLOCKER();

    EVENT_BLOCKER( const EVENT_BLOCKER& ) = delete;
    EVENT_BLOCKER& operator=( const EVENT_BLOCKER& ) = delete;

private:
    class IMPL;
    std::unique_ptr<IMPL> m_impl;
};
