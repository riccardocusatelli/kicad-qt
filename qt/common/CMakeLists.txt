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
    ${CMAKE_SOURCE_DIR}/qt/libs/kiplatform/printing.cpp
    ${CMAKE_SOURCE_DIR}/qt/libs/kiplatform/ui.cpp
    ${CMAKE_SOURCE_DIR}/qt/wxcompat/kiwx_aui.cpp
    ${CMAKE_SOURCE_DIR}/qt/wxcompat/kiwx_gui.cpp
    ${CMAKE_SOURCE_DIR}/qt/wxcompat/kiwx_gdi.cpp
    ${CMAKE_SOURCE_DIR}/qt/wxcompat/kiwx_grid.cpp
    ${CMAKE_SOURCE_DIR}/qt/wxcompat/kiwx_propgrid.cpp
    ${CMAKE_SOURCE_DIR}/qt/wxcompat/kiwx_validators.cpp
    ${CMAKE_SOURCE_DIR}/qt/wxcompat/kiwx_combo.cpp
    ${CMAKE_SOURCE_DIR}/qt/wxcompat/kiwx_menu.cpp
    ${CMAKE_SOURCE_DIR}/qt/wxcompat/kiwx_window.cpp
    ${CMAKE_SOURCE_DIR}/qt/wxcompat/kiwx_textentry.cpp
    ${CMAKE_SOURCE_DIR}/qt/wxcompat/kiwx_filedlg.cpp
    ${CMAKE_SOURCE_DIR}/qt/wxcompat/kiwx_stc.cpp
    ${CMAKE_SOURCE_DIR}/qt/wxcompat/kiwx_msgdlg.cpp
    ${CMAKE_SOURCE_DIR}/qt/wxcompat/kiwx_pickers.cpp
    ${CMAKE_SOURCE_DIR}/qt/wxcompat/kiwx_listctrl.cpp
    ${CMAKE_SOURCE_DIR}/qt/wxcompat/kiwx_treectrl.cpp
    ${CMAKE_SOURCE_DIR}/qt/wxcompat/kiwx_treelist.cpp
    ${CMAKE_SOURCE_DIR}/qt/wxcompat/kiwx_dataview.cpp
    ${CMAKE_SOURCE_DIR}/qt/wxcompat/kiwx_statusbar.cpp
    ${CMAKE_SOURCE_DIR}/qt/wxcompat/kiwx_display.cpp
    ${CMAKE_SOURCE_DIR}/qt/wxcompat/kiwx_clipboard.cpp
    ${CMAKE_SOURCE_DIR}/qt/wxcompat/kiwx_print.cpp
    ${CMAKE_SOURCE_DIR}/qt/wxcompat/kiwx_wizard.cpp

    # The controls of the views (part of the GAL library) and the OpenGL GAL of KiCad, on the
    # OpenGL canvas of Qt
    ${KIQT_OPENGL_GAL_SRCS}
    gal/hidpi_gl_canvas.cpp
    gal/hidpi_gl_3D_canvas_qt.cpp
    ${CMAKE_SOURCE_DIR}/common/gal/hidpi_gl_3D_canvas.cpp
    gal/opengl_gal_qt.cpp

    # Cairo GAL of the printouts
    ${CMAKE_SOURCE_DIR}/common/gal/cairo/cairo_print.cpp
    gal/cairo_print_ctx_qt.cpp

    kicad_gl/gl_context_mgr.cpp
    kicad_gl/gl_utils.cpp
    )

if( MSVC )
    # The sources of kiqt that define GAL_DLL (the OpenGL GAL) export the inline members of the GAL
    # base classes too, which kigal_model exports: keep the first definition
    target_link_options( kiqt PRIVATE /FORCE:MULTIPLE )

    # The Qt program classes are exported as on Linux, where every symbol is visible
    set_target_properties( kiqt PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON )
endif()

set_target_properties( kiqt PROPERTIES
    AUTOMOC ON
    OUTPUT_NAME kiqt
    )

# The OpenGL GAL and the Cairo GAL of the printouts are part of the GAL library (GAL_API)
set_source_files_properties( ${KIQT_OPENGL_GAL_SRCS}
                             gal/hidpi_gl_canvas.cpp gal/opengl_gal_qt.cpp
                             gal/hidpi_gl_3D_canvas_qt.cpp
                             ${CMAKE_SOURCE_DIR}/common/gal/hidpi_gl_3D_canvas.cpp
                             ${CMAKE_SOURCE_DIR}/common/gal/cairo/cairo_print.cpp
                             gal/cairo_print_ctx_qt.cpp
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
        Qt6::PrintSupport
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
    library_editor_control.cpp
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
    ${CMAKE_SOURCE_DIR}/common/widgets/search_pane_base.fbp
    )

# The dialogs and widgets of KiCad built unchanged on the wxWidgets compatibility layer
# (qt/wxcompat)
kiqt_generate_fbp_compat( KIQT_COMMON_COMPAT_FBP_SRCS
    ${CMAKE_SOURCE_DIR}/common/dialog_about/dialog_about_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/dialog_autosave_recovery_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/eda_list_dialog_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/dialog_book_reporter_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/dialog_configure_paths_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/dialog_display_html_text_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/dialog_grid_settings.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/git/dialog_git_repository_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/git/panel_git_repositories_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/dialog_group_properties_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/dialog_page_settings_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/dialog_print_generic_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/panel_printer_list_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/dialog_plugin_options_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/dialog_design_block_properties_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/dialog_text_entry_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/dialog_paste_special_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/panel_common_settings_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/panel_embedded_files_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/panel_grid_settings_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/panel_maintenance_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/panel_mouse_settings_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/panel_packages_and_updates_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/panel_plugin_settings_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/panel_spacemouse_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/panel_setup_netclasses_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/panel_startwizard_libraries_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/panel_startwizard_privacy_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/panel_startwizard_settings_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/panel_text_variables_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/panel_toolbar_customization_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/eda_reorderable_list_dialog_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/panel_design_block_lib_table_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/dialog_assign_netclass_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/dialog_HTML_reporter_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/dialog_import_choose_project_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/dialog_rc_job_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/panel_color_settings_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/panel_gal_options_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/panel_image_editor_base.fbp
    ${CMAKE_SOURCE_DIR}/common/widgets/wx_html_report_panel_base.fbp
    )

set( KIQT_COMMON_COMPAT_SRCS
    dialog_about/AboutDialog_main.cpp
    dialog_about/dialog_about.cpp
    dialogs/dialog_autosave_recovery.cpp
    dialogs/dialog_configure_paths.cpp
    dialogs/dialog_edit_library_tables.cpp
    dialogs/dialog_grid_settings.cpp
    dialogs/dialog_hotkey_list.cpp
    dialogs/dialog_page_settings.cpp
    dialogs/dialog_print_generic.cpp
    dialogs/dialog_plugin_options.cpp
    dialogs/dialog_design_block_properties.cpp
    dialogs/dialog_text_entry.cpp
    dialogs/eda_list_dialog.cpp
    dialogs/git/dialog_git_repository.cpp
    dialogs/git/panel_git_repos.cpp
    dialogs/html_message_box.cpp
    dialogs/panel_common_settings.cpp
    dialogs/panel_embedded_files.cpp
    dialogs/panel_grid_settings.cpp
    dialogs/panel_hotkeys_editor.cpp
    dialogs/panel_maintenance.cpp
    dialogs/panel_mouse_settings.cpp
    dialogs/panel_packages_and_updates.cpp
    dialogs/panel_plugin_settings.cpp
    dialogs/panel_spacemouse.cpp
    dialogs/panel_printer_list.cpp
    dialogs/panel_setup_severities.cpp
    dialogs/panel_text_variables.cpp
    dialogs/panel_toolbar_customization.cpp
    grid_tricks.cpp
    kidialog.cpp
    lib_table_grid_tricks.cpp
    lib_table_notebook_panel.cpp
    scintilla_tricks.cpp
    startwizard/startwizard.cpp
    startwizard/startwizard_provider_libraries.cpp
    startwizard/startwizard_provider_privacy.cpp
    startwizard/startwizard_provider_settings.cpp
    ui_events.cpp
    validators.cpp
    widgets/button_row_panel.cpp
    widgets/filter_combobox.cpp
    widgets/font_choice.cpp
    widgets/grid_bitmap_toggle.cpp
    widgets/grid_button.cpp
    widgets/grid_checkbox.cpp
    widgets/grid_color_swatch_helpers.cpp
    widgets/grid_combobox.cpp
    widgets/grid_icon_text_helpers.cpp
    widgets/grid_text_button_helpers.cpp
    widgets/grid_text_helpers.cpp
    widgets/indicator_icon.cpp
    widgets/number_badge.cpp
    widgets/split_button.cpp
    widgets/up_down_tree.cpp
    widgets/widget_hotkey_list.cpp
    widgets/wx_dataviewctrl.cpp
    widgets/wx_grid.cpp
    widgets/wx_listbox.cpp
    widgets/zoom_correction_ctrl.cpp
    ptree.cpp
    widgets/app_progress_dialog.cpp
    widgets/footprint_preview_widget.cpp
    widgets/footprint_select_widget.cpp
    widgets/listbox_tricks.cpp
    widgets/wx_bitmap_combobox.cpp
    dialogs/eda_reorderable_list_dialog.cpp
    dialogs/panel_design_block_lib_table.cpp
    dialogs/dialog_book_reporter.cpp
    dialogs/dialog_group_properties.cpp
    printout.cpp
    dialogs/dialog_assign_netclass.cpp
    dialogs/dialog_import_choose_project.cpp
    dialogs/dialog_multi_unit_entry.cpp
    dialogs/dialog_paste_special.cpp
    dialogs/dialog_rc_job.cpp
    dialogs/panel_color_settings.cpp
    dialogs/panel_gal_options.cpp
    dialogs/panel_image_editor.cpp
    dialogs/panel_setup_netclasses.cpp
    dialogs/rule_editor_dialog_base.cpp
    drawing_sheet/ds_data_model_io.cpp
    drawing_sheet/ds_proxy_undo_item.cpp
    import_gfx/dxf_import_plugin.cpp
    import_gfx/graphics_import_mgr.cpp
    libraries/lib_table_grid_data_model.cpp
    pin_numbers.cpp
    status_popup.cpp
    tool/design_block_control.cpp
    tool/embed_tool.cpp
    tool/group_tool.cpp
    tool/properties_tool.cpp
    widgets/margin_offset_binder.cpp
    widgets/rc_tree_model.cpp
    widgets/widget_save_restore.cpp
    widgets/wx_html_report_panel.cpp
    )

list( TRANSFORM KIQT_COMMON_COMPAT_SRCS PREPEND ${CMAKE_SOURCE_DIR}/common/ )

add_library( kiqt_common STATIC
    ${KIQT_COMMON_COMPAT_FBP_SRCS}
    ${KIQT_COMMON_COMPAT_SRCS}
    ${KIQT_COMMON_TOOL_SRCS}
    ${KIQT_COMMON_FBP_SRCS}

    ${CMAKE_SOURCE_DIR}/common/bitmap_base_dc.cpp
    ${CMAKE_SOURCE_DIR}/common/clipboard.cpp
    ${CMAKE_SOURCE_DIR}/common/dpi_scaling.cpp
    ${CMAKE_SOURCE_DIR}/common/dpi_scaling_common.cpp
    ${CMAKE_SOURCE_DIR}/common/drawing_sheet/ds_draw_item_print.cpp
    ${CMAKE_SOURCE_DIR}/common/eda_doc.cpp
    ${CMAKE_SOURCE_DIR}/common/eda_text_print.cpp
    ${CMAKE_SOURCE_DIR}/common/executable_names.cpp
    ${CMAKE_SOURCE_DIR}/common/gal_display_options_common.cpp
    ${CMAKE_SOURCE_DIR}/common/gestfich_gui.cpp
    ${CMAKE_SOURCE_DIR}/common/gr_basic.cpp
    ${CMAKE_SOURCE_DIR}/common/gr_text_print.cpp
    ${CMAKE_SOURCE_DIR}/common/gui_reporters.cpp
    ${CMAKE_SOURCE_DIR}/common/hotkey_store.cpp
    ${CMAKE_SOURCE_DIR}/common/hotkeys_basic.cpp
    ${CMAKE_SOURCE_DIR}/common/io/csv.cpp
    ${CMAKE_SOURCE_DIR}/common/kiface_gui.cpp
    ${CMAKE_SOURCE_DIR}/common/kiway_holder.cpp
    ${CMAKE_SOURCE_DIR}/common/launch_ext.cpp
    ${CMAKE_SOURCE_DIR}/common/origin_viewitem.cpp
    ${CMAKE_SOURCE_DIR}/common/preview_items/anchor_debug.cpp
    ${CMAKE_SOURCE_DIR}/common/preview_items/angle_item.cpp
    ${CMAKE_SOURCE_DIR}/common/preview_items/arc_assistant.cpp
    ${CMAKE_SOURCE_DIR}/common/preview_items/arc_geom_manager.cpp
    ${CMAKE_SOURCE_DIR}/common/preview_items/bezier_assistant.cpp
    ${CMAKE_SOURCE_DIR}/common/preview_items/bezier_geom_manager.cpp
    ${CMAKE_SOURCE_DIR}/common/preview_items/centreline_rect_item.cpp
    ${CMAKE_SOURCE_DIR}/common/preview_items/construction_geom.cpp
    ${CMAKE_SOURCE_DIR}/common/preview_items/draw_context.cpp
    ${CMAKE_SOURCE_DIR}/common/preview_items/item_drawing_utils.cpp
    ${CMAKE_SOURCE_DIR}/common/preview_items/polygon_geom_manager.cpp
    ${CMAKE_SOURCE_DIR}/common/preview_items/polygon_item.cpp
    ${CMAKE_SOURCE_DIR}/common/preview_items/preview_utils.cpp
    ${CMAKE_SOURCE_DIR}/common/preview_items/ruler_item.cpp
    ${CMAKE_SOURCE_DIR}/common/preview_items/selection_area.cpp
    ${CMAKE_SOURCE_DIR}/common/preview_items/simple_overlay_item.cpp
    ${CMAKE_SOURCE_DIR}/common/preview_items/snap_indicator.cpp
    ${CMAKE_SOURCE_DIR}/common/preview_items/two_point_assistant.cpp
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
    widgets/design_block_pane_qt.cpp
    widgets/panel_design_block_chooser_qt.cpp

    background_jobs_list_qt.cpp
    dialog_shim.cpp
    widgets/ui_common_qt.cpp
    widgets/paged_dialog_qt.cpp
    widgets/wx_treebook_qt.cpp
    eda_dde_qt.cpp
    file_history_qt.cpp
    notifications_list_qt.cpp
    pgm_gui_qt.cpp
    ${CMAKE_SOURCE_DIR}/common/background_jobs_monitor.cpp
    ${CMAKE_SOURCE_DIR}/common/notifications_manager.cpp
    ${CMAKE_SOURCE_DIR}/common/pgm_gui.cpp
    dialogs/dialog_color_picker.cpp
    dialogs/dialog_unit_entry.cpp
    dialogs/eda_view_switcher.cpp
    dialogs/hotkey_cycle_popup.cpp
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

# The 3D mice through libspnav (Linux), as in kicommon and common
if( UNIX AND NOT APPLE )
    target_sources( kiqt_common PRIVATE
        ${CMAKE_SOURCE_DIR}/common/spacenav/libspnav_driver.cpp
        ${CMAKE_SOURCE_DIR}/common/spacenav/spnav_2d_plugin.cpp
        )
    target_link_libraries( kiqt_common PUBLIC ${SPNAV_LIBRARIES} )
endif()

set_target_properties( kiqt_common PROPERTIES POSITION_INDEPENDENT_CODE ON )

# The panels of the board settings, the printout and the DXF import shared with eeschema (in common
# in the wxWidgets build) include headers of pcbnew
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/common/dialogs/panel_image_editor.cpp
    ${CMAKE_SOURCE_DIR}/common/dialogs/panel_setup_netclasses.cpp
    ${CMAKE_SOURCE_DIR}/common/printout.cpp
    ${CMAKE_SOURCE_DIR}/common/import_gfx/dxf_import_plugin.cpp
    PROPERTIES INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/pcbnew
    )

# The import of the DXF files (the graphics import of pcbnew and eeschema)
target_include_directories( kiqt_common PRIVATE ${CMAKE_SOURCE_DIR}/thirdparty/dxflib_qcad )
target_link_libraries( kiqt_common PRIVATE dxflib_qcad tinyspline_lib )



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

# The dialogs of common include their headers by name (as in pcbnew and eeschema)
target_include_directories( kiqt_common PRIVATE ${CMAKE_SOURCE_DIR}/common/dialogs )

# The actions of the project manager (as in common)
target_sources( kiqt_common PRIVATE ${CMAKE_SOURCE_DIR}/common/tool/common_control.cpp )
target_include_directories( kiqt_common PRIVATE ${CMAKE_SOURCE_DIR}/kicad )
target_include_directories( kiqt_common SYSTEM PUBLIC ${PYTHON_INCLUDE_DIRS} )
target_link_libraries( kiqt_common PUBLIC ${PYTHON_LIBRARIES} pybind11::embed )

if( KICAD_IPC_API )
    target_sources( kiqt_common PRIVATE
        ${CMAKE_SOURCE_DIR}/common/api/api_handler.cpp
        ${CMAKE_SOURCE_DIR}/common/api/api_handler_common.cpp
        ${CMAKE_SOURCE_DIR}/common/api/api_handler_editor.cpp
        ${CMAKE_SOURCE_DIR}/common/api/api_plugin.cpp
        ${CMAKE_SOURCE_DIR}/common/api/api_plugin_manager.cpp
        ${CMAKE_SOURCE_DIR}/common/api/api_server.cpp
        ${CMAKE_SOURCE_DIR}/scripting/python_manager.cpp
        )

    target_link_libraries( kiqt_common PUBLIC kinng kiapi )
endif()
