/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mark Roszko <mark.roszko@gmail.com>
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

#ifndef NOTIFICATIONS_LIST_H
#define NOTIFICATIONS_LIST_H

#include <notifications_manager.h>


/**
 * The window listing the notifications, shown by NOTIFICATIONS_MANAGER::ShowList().  Each GUI
 * toolkit has its window (notifications_list_wx.cpp, qt/common/notifications_list_qt.cpp), which
 * calls NOTIFICATIONS_MANAGER::listClosed() when it closes.
 */
class NOTIFICATIONS_LIST
{
public:
    virtual ~NOTIFICATIONS_LIST() = default;

    virtual void Add( NOTIFICATION* aNoti ) = 0;

    virtual void Remove( NOTIFICATION* aNoti ) = 0;

protected:
    /// Tell the manager that the window closes.
    static void notifyClosed( NOTIFICATIONS_MANAGER* aManager, NOTIFICATIONS_LIST* aList )
    {
        aManager->listClosed( aList );
    }
};

#endif // NOTIFICATIONS_LIST_H
