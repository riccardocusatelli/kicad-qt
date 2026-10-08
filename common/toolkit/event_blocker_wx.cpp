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

#include <wx/event.h>
#include <wx/window.h>


class EVENT_BLOCKER::IMPL
{
public:
    IMPL( wxWindow* aWindow ) : m_blocker( aWindow ) {}

private:
    wxEventBlocker m_blocker;
};


EVENT_BLOCKER::EVENT_BLOCKER( TOOLKIT_WINDOW* aWindow ) :
        m_impl( std::make_unique<IMPL>( aWindow ) )
{
}


EVENT_BLOCKER::~EVENT_BLOCKER() = default;
