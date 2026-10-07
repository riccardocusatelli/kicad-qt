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

#pragma once

#include <tool/action_toolbar_control.h>

#include <wx/intl.h>


/*
 * Common controls for the toolbar
 */
ACTION_TOOLBAR_CONTROL ACTION_TOOLBAR_CONTROLS::gridSelect( "control.GridSelector",
                                                            _( "Grid selector" ),
                                                            _( "Grid Selection box" ),
                                                            { FRAME_SCH,
                                                              FRAME_SCH_SYMBOL_EDITOR,
                                                              FRAME_SCH_VIEWER,
                                                              FRAME_PCB_EDITOR,
                                                              FRAME_FOOTPRINT_EDITOR,
                                                              FRAME_FOOTPRINT_VIEWER,
                                                              FRAME_GERBER,
                                                              FRAME_PL_EDITOR } );


ACTION_TOOLBAR_CONTROL ACTION_TOOLBAR_CONTROLS::zoomSelect( "control.ZoomSelector",
                                                            _( "Zoom selector" ),
                                                            _( "Zoom Selection box" ),
                                                            { FRAME_SCH,
                                                              FRAME_SCH_SYMBOL_EDITOR,
                                                              FRAME_SCH_VIEWER,
                                                              FRAME_PCB_EDITOR,
                                                              FRAME_PCB_DISPLAY3D,
                                                              FRAME_FOOTPRINT_EDITOR,
                                                              FRAME_FOOTPRINT_VIEWER,
                                                              FRAME_GERBER,
                                                              FRAME_PL_EDITOR } );

ACTION_TOOLBAR_CONTROL ACTION_TOOLBAR_CONTROLS::ipcScripting( "control.IPCPlugin",
                                                              _( "IPC/Scripting plugins" ),
                                                              _( "Region to hold the IPC/Scripting action buttons" ),
                                                              { FRAME_SCH,
                                                                FRAME_PCB_EDITOR } );

ACTION_TOOLBAR_CONTROL ACTION_TOOLBAR_CONTROLS::layerSelector( "control.LayerSelector",
                                                               _( "Layer selector" ),
                                                               _( "Control to select the layer" ),
                                                               { FRAME_PCB_EDITOR,
                                                                 FRAME_FOOTPRINT_EDITOR,
                                                                 FRAME_FOOTPRINT_VIEWER,
                                                                 FRAME_GERBER } );

ACTION_TOOLBAR_CONTROL ACTION_TOOLBAR_CONTROLS::unitSelector( "control.UnitSelector",
                                                              _( "Symbol unit selector" ),
                                                              _( "Displays the current unit" ),
                                                              { FRAME_SCH_SYMBOL_EDITOR,
                                                                FRAME_SCH_VIEWER } );

ACTION_TOOLBAR_CONTROL ACTION_TOOLBAR_CONTROLS::bodyStyleSelector( "control.BodyStyleSelector",
                                                                   _( "Symbol body style selector" ),
                                                                   _( "Displays the current body style" ),
                                                                   { FRAME_SCH_SYMBOL_EDITOR,
                                                                     FRAME_SCH_VIEWER } );

ACTION_TOOLBAR_CONTROL ACTION_TOOLBAR_CONTROLS::overrideLocks( "control.OverrideLocks",
                                                               _( "Override locks" ),
                                                               _( "Allow moving of locked items with the mouse" ),
                                                               { FRAME_PCB_EDITOR } );
