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

# The test of the tool framework on Qt: the upstream tools (COMMON_TOOLS, ZOOM_TOOL) on a Qt
# canvas, driven by synthetic input events.
add_headless_executable( qa_qt_tool_framework
    KIFACE  pcbnew_qt_kiface
    SOURCES test_tool_framework.cpp
    )

set_target_properties( qa_qt_tool_framework_objects PROPERTIES AUTOMOC OFF )

# The test of the 3D canvas on Qt: the OpenGL renderer and the ray tracer of the 3D viewer in a
# window of Qt (it needs a display with OpenGL).
add_headless_executable( qa_qt_3d_viewer
    KIFACE  pcbnew_qt_kiface
    SOURCES test_3d_viewer.cpp
    )

set_target_properties( qa_qt_3d_viewer_objects PROPERTIES AUTOMOC OFF )

# The headers of the 3D viewer of the Qt build (its dialogs) first
target_include_directories( qa_qt_3d_viewer_objects BEFORE PRIVATE
    ${CMAKE_SOURCE_DIR}/qt/3d-viewer
    ${KIQT_FBP_DIR}/3d-viewer
    )

# The test of the frames of pcbnew on Qt: the KIFACE with the GUI makes the original frames
# (the PCB editor, the footprint editor), which open a board.
add_headless_executable( qa_qt_pcbnew_frames
    KIFACE  pcbnew_qtgui_kiface
    SOURCES test_pcbnew_frames.cpp
    )

set_target_properties( qa_qt_pcbnew_frames_objects PROPERTIES AUTOMOC OFF )
target_sources( qa_qt_pcbnew_frames PRIVATE $<TARGET_OBJECTS:pcbnew_gui_qt> )
add_dependencies( qa_qt_pcbnew_frames pcbnew_gui_qt )

if( UNIX AND NOT APPLE )
    target_link_libraries( qa_qt_pcbnew_frames PRIVATE ${SPNAV_LIBRARIES} )
endif()
