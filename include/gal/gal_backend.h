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

namespace KIGFX
{

/**
 * The backends of the graphics abstraction layer, stored in the settings (also available as
 * EDA_DRAW_PANEL_GAL::GAL_TYPE, without the GUI header).
 */
enum GAL_BACKEND_TYPE
{
    GAL_TYPE_UNKNOWN = -1,  ///< not specified: a GAL engine must be set by the client
    GAL_TYPE_NONE = 0,      ///< GAL not used (the legacy wxDC engine is used)
    GAL_TYPE_OPENGL,        ///< OpenGL implementation
    GAL_TYPE_CAIRO,         ///< Cairo implementation
    GAL_TYPE_LAST           ///< Sentinel, do not use as a parameter
};

} // namespace KIGFX
