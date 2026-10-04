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

#ifndef KIFACE_GUI_H
#define KIFACE_GUI_H

#include <kiface_base.h>


/**
 * The KIFACE_BASE of the editors: it has the actions of their tools.
 *
 * The KIFACEs without GUI (loaded by kicad-cli) derive from KIFACE_BASE.
 */
class KIFACE_GUI : public KIFACE_BASE
{
public:
    KIFACE_GUI( const char* aKifaceName, KIWAY_FACES::FACE_T aId ) :
            KIFACE_BASE( aKifaceName, aId )
    {
    }

    /// Append the registered tool actions to @a aActions.
    void GetActions( std::vector<TOOL_ACTION*>& aActions ) const override;
};

#endif // KIFACE_GUI_H
