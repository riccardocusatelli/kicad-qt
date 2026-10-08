/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#ifndef COMMON_WIDGETS_WX_BUSY_INDICATOR__H
#define COMMON_WIDGETS_WX_BUSY_INDICATOR__H

#include <memory>

#include <widgets/busy_indicator.h>


/**
 * Simple wrapper around the busy cursor of the GUI toolkit (wxBusyCursor), for use with the
 * generic BUSY_INDICATOR interface.
 *
 * Can be used to provide a busy cursor (spinner) to generic code that otherwise has no
 * concept of the cursors of the toolkit.  Each toolkit has its implementation
 * (common/widgets/wx_busy_indicator.cpp, and the one of the Qt build).
 */
class WX_BUSY_INDICATOR : public BUSY_INDICATOR
{
public:
    /**
     * Construct a busy indicator represented by a busy cursor, which will be
     * active as long as this object exists (just like wxBusyCursor itself).
     */
    WX_BUSY_INDICATOR();

    ~WX_BUSY_INDICATOR() override;

private:
    /// The busy cursor of the toolkit, which is the indicator.
    class CURSOR;

    std::unique_ptr<CURSOR> m_cursor;
};

#endif // COMMON_WIDGETS_WX_BUSY_INDICATOR__H
