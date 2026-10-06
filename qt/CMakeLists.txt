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
#   common/   libkiqt: the canvas on the Cairo GAL, the documents, the property panel
#   pcbnew/   _pcbnew_qt.kiface: the headless pcbnew KIFACE with the board documents
#   eeschema/ _eeschema_qt.kiface: the headless eeschema KIFACE with the schematic documents
#   kicad/    kicad-qt: the main window, which loads the two KIFACEs

find_package( Qt6 6.2 REQUIRED COMPONENTS Widgets )

# Sources of the model the documents use, besides the ones kicad-cli links
# (see cmake/WxFree.cmake)
set_property( GLOBAL APPEND PROPERTY KICAD_WX_FREE_SOURCES_pcbcommon
    pcbnew/pcb_view.cpp
    )
set_property( GLOBAL APPEND PROPERTY KICAD_WX_FREE_SOURCES_eeschema_kiface_objects
    eeschema/sch_painter.cpp
    eeschema/sch_view.cpp
    )

# The sources compiled into each Qt KIFACE, with its PROPERTY_MANAGER
set( KIQT_KIFACE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/common/inspectable_rows.cpp )

add_subdirectory( common )
add_subdirectory( pcbnew )
add_subdirectory( eeschema )
add_subdirectory( kicad )

set_property( GLOBAL APPEND PROPERTY KICAD_WX_FREE_KEPT_TARGETS
    kiqt
    pcbnew_qt_kiface pcbnew_qt_kiface_iface
    eeschema_qt_kiface eeschema_qt_kiface_iface
    kicad-qt
    )
