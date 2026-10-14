# libkiqt: the Qt widgets shared by kicad-qt and the Qt KIFACEs, and the GUI services of the Qt
# build that are one for the program (the icons, the cursors, the message dialogs, the event
# loop and the keyboard)

# The sources of the OpenGL GAL of KiCad (common/gal/opengl), toolkit-independent
set( KIQT_OPENGL_GAL_SRCS
    opengl_gal.cpp
    gl_resources.cpp
    shader.cpp
    vertex_item.cpp
    vertex_container.cpp
    cached_container.cpp
    cached_container_gpu.cpp
    cached_container_ram.cpp
    noncached_container.cpp
    vertex_manager.cpp
    gpu_manager.cpp
    antialiasing.cpp
    opengl_compositor.cpp
    utils.cpp
    )

list( TRANSFORM KIQT_OPENGL_GAL_SRCS PREPEND ${CMAKE_SOURCE_DIR}/common/gal/opengl/ )

set( KIQT_VIEW_CONTROLS_SRCS
    ${CMAKE_SOURCE_DIR}/common/view/view_controls.cpp
    ${CMAKE_SOURCE_DIR}/common/view/zoom_controller.cpp
    )

add_library( kiqt SHARED
    gal_canvas.cpp
    property_panel.cpp
    qt_cairo_gal.cpp
    qt_document.cpp
    ui_hooks.cpp

    bitmap_store.cpp
    raster_image_qt.cpp
    confirm.cpp
    gal/cursors.cpp
    toolkit/file_watcher_qt.cpp
    toolkit/gui_services_qt.cpp
    toolkit/gui_timer_qt.cpp
    toolkit/event_blocker_qt.cpp
    toolkit/choice_qt.cpp
    toolkit/process_qt.cpp
    toolkit/single_instance_qt.cpp
    toolkit/qt_keys.cpp
    widgets/bitmap_button.cpp
    widgets/bitmap_toggle.cpp
    widgets/html_window.cpp
    widgets/wx_html_report_box.cpp
    widgets/msgpanel.cpp
    widgets/spin_button.cpp
    widgets/wx_infobar.cpp
    widgets/wx_panel.cpp
    widgets/wx_progress_reporters.cpp
    validators_qt.cpp
    ${CMAKE_SOURCE_DIR}/qt/libs/kiplatform/ui.cpp
    ${CMAKE_SOURCE_DIR}/qt/wxcompat/kiwx_gui.cpp
    ${CMAKE_SOURCE_DIR}/qt/wxcompat/kiwx_gdi.cpp

    # The controls of the views (part of the GAL library) and the OpenGL GAL of KiCad, on the
    # OpenGL canvas of Qt
    ${KIQT_VIEW_CONTROLS_SRCS}
    ${KIQT_OPENGL_GAL_SRCS}
    gal/hidpi_gl_canvas.cpp
    gal/hidpi_gl_3D_canvas_qt.cpp
    ${CMAKE_SOURCE_DIR}/common/gal/hidpi_gl_3D_canvas.cpp
    gal/opengl_gal_qt.cpp
    kicad_gl/gl_context_mgr.cpp
    kicad_gl/gl_utils.cpp
    )

set_target_properties( kiqt PROPERTIES
    AUTOMOC ON
    OUTPUT_NAME kiqt
    )

# The OpenGL GAL is part of the GAL library (GAL_API)
set_source_files_properties( ${KIQT_OPENGL_GAL_SRCS} ${KIQT_VIEW_CONTROLS_SRCS}
                             gal/hidpi_gl_canvas.cpp gal/opengl_gal_qt.cpp
                             gal/hidpi_gl_3D_canvas_qt.cpp
                             ${CMAKE_SOURCE_DIR}/common/gal/hidpi_gl_3D_canvas.cpp
    PROPERTIES COMPILE_DEFINITIONS GAL_DLL=1 )

# The shaders of the OpenGL GAL (as in common/gal/CMakeLists.txt, in the Qt directories)
function( kiqt_add_shader inFile shaderName )
    set( _cpp ${CMAKE_CURRENT_BINARY_DIR}/shaders/${shaderName}.cpp )
    set( _hdir ${CMAKE_CURRENT_BINARY_DIR}/shaders/include/ )

    add_custom_command(
        OUTPUT ${_cpp} ${_hdir}/${shaderName}.h
        COMMAND ${CMAKE_COMMAND}
            -DSOURCE_FILE="${CMAKE_SOURCE_DIR}/common/gal/shaders/${inFile}"
            -DOUT_CPP_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders/"
            -DOUT_HEADER_DIR="${_hdir}"
            -DOUT_CPP_FILENAME="${shaderName}.cpp"
            -DOUT_HEADER_FILENAME="${shaderName}.h"
            -DOUT_VAR_NAME="${shaderName}"
            -P ${KICAD_CMAKE_MODULE_PATH}/BuildSteps/CreateShaderCpp.cmake
        DEPENDS ${CMAKE_SOURCE_DIR}/common/gal/shaders/${inFile}
                ${KICAD_CMAKE_MODULE_PATH}/BuildSteps/CreateShaderCpp.cmake
        )

    target_sources( kiqt PRIVATE ${_cpp} )
    target_include_directories( kiqt PRIVATE ${_hdir} )
endfunction()

foreach( _shader kicad_frag kicad_vert smaa_base smaa_pass_1_frag_color smaa_pass_1_frag_luma
                 smaa_pass_1_vert smaa_pass_2_frag smaa_pass_2_vert smaa_pass_3_frag
                 smaa_pass_3_vert xor_diff_frag xor_diff_vert )
    kiqt_add_shader( ${_shader}.glsl glsl_${_shader} )
endforeach()

target_include_directories( kiqt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )

target_link_libraries( kiqt
    PUBLIC
        Qt6::Widgets
        gal_model
        glad
        ${OPENGL_LIBRARIES}
        kicommon_model
        kiplatform_core
        kimath
        core
        ${wxWidgets_BASE_LIBRARIES}
    )

# The classes of kiqt are exported (KIQT_API)
target_compile_definitions( kiqt PRIVATE KIQT_DLL )
kiqt_gui_target( kiqt )

if( CMAKE_SYSTEM_NAME STREQUAL "Linux" )
    target_link_options( kiqt PRIVATE "LINKER:--no-undefined" )
endif()

install( TARGETS kiqt
    LIBRARY DESTINATION ${KICAD_LIB}
    COMPONENT binary
    )


# libkiqt_common (static, in each Qt KIFACE as the common library of the wxWidgets build): the
# tool framework of KiCad (common/tool) with the Qt versions of the frames, the canvas, the
# menus, the input, KIWAY and the base of the dialogs (qt/include has their headers)

set( KIQT_COMMON_TOOL_SRCS
    action_manager.cpp
    action_toolbar_controls.cpp
    actions.cpp
    common_tools.cpp
    conditional_menu.cpp
    construction_manager.cpp
    edit_constraints.cpp
    edit_points.cpp
    editor_conditions.cpp
    grid_helper.cpp
    grid_menu.cpp
    picker_tool.cpp
    point_editor_behavior.cpp
    selection_conditions.cpp
    selection_tool.cpp
    tool_action.cpp
    tool_base.cpp
    tool_event.cpp
    tool_interactive.cpp
    tool_manager.cpp
    tool_menu.cpp
    tools_holder.cpp
    zoom_menu.cpp
    zoom_tool.cpp
    )

list( TRANSFORM KIQT_COMMON_TOOL_SRCS PREPEND ${CMAKE_SOURCE_DIR}/common/tool/ )

kiqt_generate_fbp( KIQT_COMMON_FBP_SRCS
    ${CMAKE_SOURCE_DIR}/common/dialogs/dialog_unit_entry_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/eda_view_switcher_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/dialog_color_picker_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/eda_list_dialog_base.fbp
    ${CMAKE_SOURCE_DIR}/common/widgets/search_pane_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/dialog_design_block_properties_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/dialog_text_entry_base.fbp
    )

add_library( kiqt_common STATIC
    ${KIQT_COMMON_TOOL_SRCS}
    ${KIQT_COMMON_FBP_SRCS}

    ${CMAKE_SOURCE_DIR}/common/dpi_scaling.cpp
    ${CMAKE_SOURCE_DIR}/common/dpi_scaling_common.cpp
    ${CMAKE_SOURCE_DIR}/common/gal_display_options_common.cpp
    ${CMAKE_SOURCE_DIR}/common/hotkeys_basic.cpp
    ${CMAKE_SOURCE_DIR}/common/kiway_holder.cpp
    ${CMAKE_SOURCE_DIR}/common/preview_items/selection_area.cpp
    ${CMAKE_SOURCE_DIR}/common/preview_items/simple_overlay_item.cpp
    ${CMAKE_SOURCE_DIR}/common/widgets/msgpanel_item.cpp

    # the properties panels: the toolkit-independent logic, the values of the properties
    ${CMAKE_SOURCE_DIR}/common/properties/color4d_variant.cpp
    ${CMAKE_SOURCE_DIR}/common/properties/eda_angle_variant.cpp
    ${CMAKE_SOURCE_DIR}/common/properties/pg_value_text.cpp
    ${CMAKE_SOURCE_DIR}/common/properties/std_optional_variants.cpp
    ${CMAKE_SOURCE_DIR}/common/widgets/properties_panel.cpp

    # the trees of the libraries: the model and what the adapter does, toolkit-independent
    ${CMAKE_SOURCE_DIR}/common/lib_tree_model.cpp
    ${CMAKE_SOURCE_DIR}/common/lib_tree_model_adapter.cpp
    ${CMAKE_SOURCE_DIR}/common/design_block_tree_model_adapter.cpp
    lib_tree_model_adapter_qt.cpp
    widgets/lib_tree.cpp

    # the design blocks panes: what they do, toolkit-independent, and their Qt widgets
    ${CMAKE_SOURCE_DIR}/common/widgets/design_block_pane.cpp
    ${CMAKE_SOURCE_DIR}/common/widgets/panel_design_block_chooser.cpp
    dialogs/dialog_design_block_properties_qt.cpp
    widgets/design_block_pane_qt.cpp
    widgets/panel_design_block_chooser_qt.cpp

    background_jobs_list_qt.cpp
    dialog_shim.cpp
    widgets/ui_common_qt.cpp
    widgets/paged_dialog_qt.cpp
    widgets/wx_treebook_qt.cpp
    eda_dde_qt.cpp
    notifications_list_qt.cpp
    pgm_gui_qt.cpp
    ${CMAKE_SOURCE_DIR}/common/background_jobs_monitor.cpp
    ${CMAKE_SOURCE_DIR}/common/notifications_manager.cpp
    ${CMAKE_SOURCE_DIR}/common/pgm_gui.cpp
    dialogs/dialog_color_picker.cpp
    dialogs/dialog_text_entry.cpp
    dialogs/dialog_unit_entry.cpp
    dialogs/eda_list_dialog.cpp
    dialogs/eda_view_switcher.cpp
    draw_panel_gal.cpp
    eda_base_frame.cpp
    eda_base_frame_help.cpp
    eda_draw_frame.cpp
    kiway.cpp
    kiway_player.cpp
    properties/pg_properties.cpp
    tool/action_menu.cpp
    tool/action_toolbar.cpp
    tool/tool_dispatcher.cpp
    view/qt_view_controls.cpp
    widgets/color_swatch.cpp
    widgets/filter_combobox.cpp
    widgets/kistatusbar.cpp
    widgets/layer_box_selector.cpp
    widgets/layer_presentation_qt.cpp
    widgets/properties_panel_qt.cpp
    widgets/property_grid.cpp
    widgets/search_pane.cpp
    widgets/std_bitmap_button.cpp
    widgets/text_ctrl_eval.cpp
    widgets/unit_binder.cpp
    widgets/wx_busy_indicator.cpp
    )

set_target_properties( kiqt_common PROPERTIES POSITION_INDEPENDENT_CODE ON )

# The code of KiCad includes Python.h, whose structures have members named as the keywords of Qt
target_compile_definitions( kiqt_common PRIVATE QT_NO_KEYWORDS )
kiqt_gui_target( kiqt_common )

target_include_directories( kiqt_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    $<TARGET_PROPERTY:common,INTERFACE_INCLUDE_DIRECTORIES>
    )

target_link_libraries( kiqt_common
    PUBLIC
        kiqt
        libcontext
        Qt6::Network
    )

# Python scripting and the IPC API of KiCad: the interpreter, the server of the requests and the
# plugins (in kicommon in the wxWidgets build), with the program services of PGM_GUI
target_sources( kiqt_common PRIVATE
    ${CMAKE_SOURCE_DIR}/scripting/python_scripting.cpp
    )

target_include_directories( kiqt_common PRIVATE ${CMAKE_SOURCE_DIR}/scripting )
target_include_directories( kiqt_common SYSTEM PUBLIC ${PYTHON_INCLUDE_DIRS} )
target_link_libraries( kiqt_common PUBLIC ${PYTHON_LIBRARIES} pybind11::embed )

if( KICAD_IPC_API )
    target_sources( kiqt_common PRIVATE
        ${CMAKE_SOURCE_DIR}/common/api/api_handler.cpp
        ${CMAKE_SOURCE_DIR}/common/api/api_plugin.cpp
        ${CMAKE_SOURCE_DIR}/common/api/api_plugin_manager.cpp
        ${CMAKE_SOURCE_DIR}/common/api/api_server.cpp
        ${CMAKE_SOURCE_DIR}/scripting/python_manager.cpp
        )

    target_link_libraries( kiqt_common PUBLIC kinng kiapi )
endif()
