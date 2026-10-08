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

#ifndef BACKGROUND_JOB_LIST_H
#define BACKGROUND_JOB_LIST_H

#include <memory>

struct BACKGROUND_JOB;


/**
 * The window listing the background jobs, shown by BACKGROUND_JOBS_MONITOR::ShowList().  Each GUI
 * toolkit has its window (background_jobs_list_wx.cpp, qt/common/background_jobs_list_qt.cpp),
 * which calls BACKGROUND_JOBS_MONITOR::listClosed() when it closes.
 */
class BACKGROUND_JOB_LIST
{
public:
    virtual ~BACKGROUND_JOB_LIST() = default;

    virtual void Add( std::shared_ptr<BACKGROUND_JOB> aJob ) = 0;

    virtual void Remove( std::shared_ptr<BACKGROUND_JOB> aJob ) = 0;

    virtual void UpdateJob( std::shared_ptr<BACKGROUND_JOB> aJob ) = 0;
};

#endif // BACKGROUND_JOB_LIST_H
