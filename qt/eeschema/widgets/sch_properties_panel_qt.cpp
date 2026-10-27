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

/**
 * @file sch_properties_panel_qt.cpp
 * The parts of SCH_PROPERTIES_PANEL with Qt (eeschema/widgets/sch_properties_panel_wx.cpp with
 * wxWidgets): the editors of its grid.  The grid of the panel has its own editors (units,
 * checkboxes, colors, footprints and links), following the frame of the panel.
 */

#include <widgets/sch_properties_panel.h>

#include <sch_base_frame.h>
#include <widgets/property_grid.h>


/// The state of the editors: the grid owns them.
struct SCH_PROPERTIES_PANEL::TOOLKIT_STATE
{
};


void SCH_PROPERTIES_PANEL::TOOLKIT_STATE_DELETER::operator()( TOOLKIT_STATE* aState ) const
{
    delete aState;
}


void SCH_PROPERTIES_PANEL::registerEditors()
{
    m_toolkitState.reset( new TOOLKIT_STATE() );

    // the unit editors follow the units of the frame, the footprint chooser opens from it
    m_grid->SetEditorFrame( m_frame );
}
