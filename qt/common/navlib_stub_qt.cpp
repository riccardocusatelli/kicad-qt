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
 * @file navlib_stub_qt.cpp
 * The SpaceMouse plug-ins of the editors with Qt on Windows and macOS, without the 3Dconnexion
 * navlib (the wxWidgets build uses the navlib directories of the editors): the frames create them
 * as with wxWidgets, and they do nothing.  On Linux the 3D mice go through libspnav.
 */

#include "../../pcbnew/navlib/nl_pcbnew_plugin.h"
#include "../../3d-viewer/3d_navlib/nl_3d_viewer_plugin.h"
#include "../../3d-viewer/3d_navlib/nl_footprint_properties_plugin.h"
#include "../../gerbview/navlib/nl_gerbview_plugin.h"
#include "../../pagelayout_editor/navlib/nl_pl_editor_plugin.h"
#include "../../eeschema/navlib/nl_schematic_plugin.h"


// The implementations are never created: complete types for the destructors of std::unique_ptr
class NL_PCBNEW_PLUGIN_IMPL {};
class NL_3D_VIEWER_PLUGIN_IMPL {};
class NL_FOOTPRINT_PROPERTIES_PLUGIN_IMPL {};
class NL_GERBVIEW_PLUGIN_IMPL {};
class NL_PL_EDITOR_PLUGIN_IMPL {};
class NL_SCHEMATIC_PLUGIN_IMPL {};


NL_PCBNEW_PLUGIN::NL_PCBNEW_PLUGIN( PCB_DRAW_PANEL_GAL* aViewport ) {}
NL_PCBNEW_PLUGIN::~NL_PCBNEW_PLUGIN() = default;
void NL_PCBNEW_PLUGIN::SetFocus( bool aFocus ) {}

NL_3D_VIEWER_PLUGIN::NL_3D_VIEWER_PLUGIN( EDA_3D_CANVAS* aViewport ) {}
NL_3D_VIEWER_PLUGIN::~NL_3D_VIEWER_PLUGIN() = default;
void NL_3D_VIEWER_PLUGIN::SetFocus( bool aFocus ) {}

NL_FOOTPRINT_PROPERTIES_PLUGIN::NL_FOOTPRINT_PROPERTIES_PLUGIN( EDA_3D_CANVAS* aViewport ) {}
NL_FOOTPRINT_PROPERTIES_PLUGIN::~NL_FOOTPRINT_PROPERTIES_PLUGIN() = default;
void NL_FOOTPRINT_PROPERTIES_PLUGIN::SetFocus( bool aFocus ) {}

NL_GERBVIEW_PLUGIN::NL_GERBVIEW_PLUGIN() {}
NL_GERBVIEW_PLUGIN::~NL_GERBVIEW_PLUGIN() = default;
void NL_GERBVIEW_PLUGIN::SetCanvas( EDA_DRAW_PANEL_GAL* aViewport ) {}
void NL_GERBVIEW_PLUGIN::SetFocus( bool aFocus ) {}

NL_PL_EDITOR_PLUGIN::NL_PL_EDITOR_PLUGIN() {}
NL_PL_EDITOR_PLUGIN::~NL_PL_EDITOR_PLUGIN() = default;
void NL_PL_EDITOR_PLUGIN::SetCanvas( EDA_DRAW_PANEL_GAL* aViewport ) {}
void NL_PL_EDITOR_PLUGIN::SetFocus( bool aFocus ) {}

NL_SCHEMATIC_PLUGIN::NL_SCHEMATIC_PLUGIN() {}
NL_SCHEMATIC_PLUGIN::~NL_SCHEMATIC_PLUGIN() = default;
void NL_SCHEMATIC_PLUGIN::SetCanvas( EDA_DRAW_PANEL_GAL* aViewport ) {}
void NL_SCHEMATIC_PLUGIN::SetFocus( bool aFocus ) {}
