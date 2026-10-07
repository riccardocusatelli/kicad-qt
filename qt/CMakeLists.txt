#
#  This program source code file is part of KICAD, a free EDA CAD application.
#
#  Copyright The KiCad Developers, see AUTHORS.txt for contributors.
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 3
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

# The Qt shell of KiCad (KICAD_QT_SHELL, in the KICAD_WX_FREE build):
#   common/   libkiqt: the canvas on the Cairo GAL, the documents, the property panel, KIWAY;
#             libkiqt_common: the tool framework of KiCad with the Qt frames, canvas and menus
#   include/  the headers of the Qt versions of the GUI classes of KiCad
#   pcbnew/   _pcbnew_qt.kiface: the headless pcbnew KIFACE with the board documents
#   eeschema/ _eeschema_qt.kiface: the headless eeschema KIFACE with the schematic documents
#   kicad/    kicad-qt: the main window, which loads the two KIFACEs

find_package( Qt6 6.2 REQUIRED COMPONENTS Widgets )

# The headers of the Qt versions of the GUI classes of KiCad (frames, canvas, menus, dialogs...)
# come before the ones of the wxWidgets build: the tools of KiCad include them unchanged
include_directories( BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/include )

# Sources of the model the documents use, besides the ones kicad-cli links
# (see cmake/WxFree.cmake)
set_property( GLOBAL APPEND PROPERTY KICAD_WX_FREE_SOURCES_pcbcommon
    pcbnew/board_undo_redo.cpp
    pcbnew/kicad_clipboard.cpp
    pcbnew/pcb_view.cpp
    )
set_property( GLOBAL APPEND PROPERTY KICAD_WX_FREE_SOURCES_eeschema_kiface_objects
    eeschema/sch_painter.cpp
    eeschema/sch_paste.cpp
    eeschema/sch_undo_redo.cpp
    eeschema/sch_view.cpp
    eeschema/tools/sch_selection.cpp
    )
set_property( GLOBAL APPEND PROPERTY KICAD_WX_FREE_SOURCES_common
    common/tool/selection.cpp
    )
set_property( GLOBAL APPEND PROPERTY KICAD_WX_FREE_SOURCES_core
    libs/core/wx_stl_compat.cpp
    )

# The sources compiled into each Qt KIFACE, with its model (PROPERTY_MANAGER, undo lists)
set( KIQT_KIFACE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/common/inspectable_rows.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common/undo_stack.cpp
    )

add_subdirectory( common )
add_subdirectory( pcbnew )
add_subdirectory( eeschema )
add_subdirectory( kicad )
add_subdirectory( qa )

set_property( GLOBAL APPEND PROPERTY KICAD_WX_FREE_KEPT_TARGETS
    kiqt kiqt_common libcontext
    pcbnew_qt_kiface pcbnew_qt_kiface_iface
    eeschema_qt_kiface eeschema_qt_kiface_iface
    kicad-qt
    qa_qt_tool_framework qa_qt_tool_framework_objects
    )
