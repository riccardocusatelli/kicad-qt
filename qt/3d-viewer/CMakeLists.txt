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

# The 3D viewer on Qt: the OpenGL renderers and the 3D canvas of KiCad (3d-viewer), besides the
# sources kicad-cli links (the ray tracer, in the 3d-viewer library of the KICAD_WX_FREE build).

set( KIQT_3D_VIEWER_SRCS
    3d_canvas/eda_3d_canvas.cpp
    3d_canvas/eda_3d_canvas_pivot.cpp
    3d_rendering/opengl/3d_model.cpp
    3d_rendering/opengl/opengl_utils.cpp
    3d_rendering/opengl/create_scene.cpp
    3d_rendering/opengl/render_3d_opengl.cpp
    3d_rendering/opengl/layer_triangles.cpp
    3d_rendering/opengl/3d_spheres_gizmo.cpp
    3d_rendering/raytracing/render_3d_raytrace_gl.cpp
    3d_rendering/raytracing/shapes2D/4pt_polygon_2d.cpp
    3d_rendering/raytracing/shapes3D/dummy_block_3d.cpp
    3d_rendering/image.cpp
    3d_rendering/test_cases.cpp
    common_ogl/ogl_utils.cpp
    3d_math.cpp

    # The 3D viewer frame, its tools, menus and toolbars
    3d_viewer/3d_menubar.cpp
    3d_viewer/toolbars_3d.cpp
    3d_viewer/tools/eda_3d_actions.cpp
    3d_viewer/tools/eda_3d_conditions.cpp
    3d_viewer/tools/eda_3d_controller.cpp
    3d_viewer/eda_3d_viewer_frame.cpp

    # The parts of the panels without the GUI toolkit
    dialogs/appearance_controls_3D.cpp
    )

if( UNIX AND NOT APPLE )
    list( APPEND KIQT_3D_VIEWER_SRCS 3d_spacenav/spnav_viewer_plugin.cpp )
endif()

list( TRANSFORM KIQT_3D_VIEWER_SRCS PREPEND ${CMAKE_SOURCE_DIR}/3d-viewer/ )

kiqt_generate_fbp( KIQT_3D_VIEWER_FBP_SRCS
    ${CMAKE_SOURCE_DIR}/3d-viewer/dialogs/appearance_controls_3D_base.fbp
    ${CMAKE_SOURCE_DIR}/3d-viewer/dialogs/panel_preview_3d_model_base.fbp
    )

add_library( 3d-viewer_qt STATIC
    ${KIQT_3D_VIEWER_SRCS}
    ${KIQT_3D_VIEWER_FBP_SRCS}
    common_ogl/ogl_attr_list_qt.cpp
    3d_viewer/eda_3d_viewer_frame_qt.cpp
    dialogs/appearance_controls_3D_qt.cpp
    dialogs/dialog_export_3d_image.cpp
    dialogs/panel_preview_3d_model_qt.cpp
    )

set_target_properties( 3d-viewer_qt PROPERTIES POSITION_INDEPENDENT_CODE ON )

# The code of KiCad includes Python.h, whose structures have members named as the keywords of Qt
target_compile_definitions( 3d-viewer_qt PRIVATE QT_NO_KEYWORDS )

# Compiled as the 3d-viewer library (its include directories), the Qt headers (and the bases
# of the panels generated from the wxFormBuilder files) first
target_include_directories( 3d-viewer_qt BEFORE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${KIQT_FBP_DIR}/3d-viewer
    )
target_include_directories( 3d-viewer_qt PRIVATE
    $<TARGET_PROPERTY:3d-viewer,INCLUDE_DIRECTORIES>
    ${CMAKE_SOURCE_DIR}/3d-viewer/3d_canvas
    ${CMAKE_SOURCE_DIR}/3d-viewer/3d_cache
    ${CMAKE_SOURCE_DIR}/3d-viewer/3d_rendering
    ${CMAKE_SOURCE_DIR}/3d-viewer/3d_viewer
    ${CMAKE_SOURCE_DIR}/3d-viewer/dialogs
    ${CMAKE_SOURCE_DIR}/pcbnew
    ${CMAKE_SOURCE_DIR}
    )

target_link_libraries( 3d-viewer_qt
    PRIVATE
        kiqt_common
        nlohmann_json
        Boost::headers
    )
