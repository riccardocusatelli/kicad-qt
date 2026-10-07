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
#include <wx/string.h>


/**
 * Watches a file of the disk: calls its handler from the event loop of the GUI toolkit when
 * the file changes (written, replaced, created or removed).
 *
 * Each GUI build (wxWidgets, Qt) has its own implementation
 * (common/toolkit/file_watcher_wx.cpp, qt/common/toolkit/file_watcher_qt.cpp).
 */
class APIVISIBLE FILE_WATCHER
{
public:
    /**
     * @param aHandler is called with the full path of the watched file when it changes.
     */
    explicit FILE_WATCHER( std::function<void( const wxString& aPath )> aHandler );
    ~FILE_WATCHER();

    FILE_WATCHER( const FILE_WATCHER& ) = delete;
    FILE_WATCHER& operator=( const FILE_WATCHER& ) = delete;

    /**
     * Watch @a aPath (an absolute path) instead of the file watched before.
     *
     * @return false if the file cannot be watched.
     */
    bool WatchFile( const wxString& aPath );

    /// Stop watching.
    void Clear();

private:
    class IMPL;
    std::unique_ptr<IMPL> m_impl;
};
