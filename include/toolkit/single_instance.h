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
#include <wx/string.h>


/**
 * Tell if another copy of a program is running (as wxSingleInstanceChecker): each program
 * holds a lock named after it in a directory.
 *
 * Each GUI build has its own implementation (common/toolkit/single_instance_wx.cpp,
 * qt/common/toolkit/single_instance_qt.cpp).
 */
class APIVISIBLE SINGLE_INSTANCE_CHECKER
{
public:
    SINGLE_INSTANCE_CHECKER();
    ~SINGLE_INSTANCE_CHECKER();

    SINGLE_INSTANCE_CHECKER( const SINGLE_INSTANCE_CHECKER& ) = delete;
    SINGLE_INSTANCE_CHECKER& operator=( const SINGLE_INSTANCE_CHECKER& ) = delete;

    /**
     * Take the lock of the program.
     *
     * @param aName is the name of the lock.
     * @param aPath is the directory of the lock.
     * @return false if the lock could not be created.
     */
    bool Create( const wxString& aName, const wxString& aPath );

    /// @return true if another program holds the lock.
    bool IsAnotherRunning() const;

private:
    class IMPL;
    std::unique_ptr<IMPL> m_impl;
};
