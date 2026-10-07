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
