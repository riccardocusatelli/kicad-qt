# _eeschema_qt.kiface: the KIFACE of eeschema without GUI (as _eeschema_cli.kiface), with the
# schematic documents of the Qt shell

add_headless_kiface( eeschema_qt_kiface
    OUTPUT_NAME eeschema_qt
    LIKE        eeschema_cli_kiface
    SOURCES     ${CMAKE_SOURCE_DIR}/eeschema/eeschema_cli.cpp
                schematic_document.cpp
                schematic_tools.cpp
                ${KIQT_KIFACE_SOURCES}
    USAGE       kiqt_common
    LIBRARIES   kiqt_common kiqt Qt6::Widgets
    )

set_target_properties( eeschema_qt_kiface_iface PROPERTIES AUTOMOC ON )
target_include_directories( eeschema_qt_kiface_iface PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} )
target_compile_definitions( eeschema_qt_kiface_iface PRIVATE KICAD_QT_MODULE )

# Next to the other eeschema KIFACEs (KIWAY_BASE finds them there when run from the build tree)
set_target_properties( eeschema_qt_kiface PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/eeschema
    )

install( TARGETS eeschema_qt_kiface
    DESTINATION ${KICAD_KIFACE}
    COMPONENT binary
    )


# The GUI of eeschema on Qt (the schematic and symbol editors, the symbol viewer and chooser, their
# tools, panels and dialogs): the sources of the eeschema KIFACE with a GUI of the wxWidgets build,
# built on the wxWidgets compatibility layer (qt/wxcompat), with the Qt versions of the panes.  The
# simulator (its plots with mathplot).
set( KIQT_EESCHEMA_GUI_SRCS
    annotate.cpp
    api/api_handler_sch.cpp
    api/api_sch_utils.cpp
    bom_plugins.cpp
    bus-wire-junction.cpp
    cross-probing.cpp
    dialogs/dialog_annotate.cpp
    dialogs/dialog_bom.cpp
    dialogs/dialog_change_symbols.cpp
    dialogs/dialog_database_lib_settings.cpp
    dialogs/dialog_edit_symbols_libid.cpp
    dialogs/dialog_eeschema_page_settings.cpp
    dialogs/dialog_erc.cpp
    dialogs/dialog_erc_job_config.cpp
    dialogs/dialog_export_netlist.cpp
    dialogs/dialog_field_properties.cpp
    dialogs/dialog_global_edit_text_and_graphics.cpp
    dialogs/dialog_ibis_parser_reporter.cpp
    dialogs/dialog_image_properties.cpp
    dialogs/dialog_import_symbol_select.cpp
    dialogs/dialog_junction_props.cpp
    dialogs/dialog_label_properties.cpp
    dialogs/dialog_lib_edit_pin_table.cpp
    dialogs/dialog_lib_fields_table.cpp
    dialogs/dialog_lib_new_symbol.cpp
    dialogs/dialog_lib_symbol_properties.cpp
    dialogs/dialog_line_properties.cpp
    dialogs/dialog_migrate_buses.cpp
    dialogs/dialog_pin_properties.cpp
    dialogs/dialog_plot_schematic.cpp
    dialogs/dialog_remote_symbol_config.cpp
    dialogs/dialog_rescue_each.cpp
    dialogs/dialog_sch_find.cpp
    dialogs/dialog_sch_import_settings.cpp
    dialogs/dialog_schematic_setup.cpp
    dialogs/dialog_shape_properties.cpp
    dialogs/dialog_sheet_pin_properties.cpp
    dialogs/dialog_sheet_properties.cpp
    dialogs/dialog_sim_command.cpp
    dialogs/dialog_sim_format_value.cpp
    dialogs/dialog_sim_model.cpp
    dialogs/dialog_symbol_chooser.cpp
    dialogs/dialog_symbol_fields_table.cpp
    dialogs/dialog_symbol_properties.cpp
    dialogs/dialog_symbol_remap.cpp
    dialogs/dialog_table_properties.cpp
    dialogs/dialog_tablecell_properties.cpp
    dialogs/dialog_text_properties.cpp
    dialogs/dialog_update_from_pcb.cpp
    dialogs/dialog_update_symbol_fields.cpp
    dialogs/dialog_user_defined_signals.cpp
    dialogs/dialog_wire_bus_properties.cpp
    dialogs/panel_bom_presets.cpp
    dialogs/panel_eeschema_annotation_options.cpp
    dialogs/panel_eeschema_color_settings.cpp
    dialogs/panel_eeschema_display_options.cpp
    dialogs/panel_eeschema_editing_options.cpp
    dialogs/panel_setup_buses.cpp
    dialogs/panel_setup_formatting.cpp
    dialogs/panel_setup_pinmap.cpp
    dialogs/panel_simulator_preferences.cpp
    dialogs/panel_sym_color_settings.cpp
    dialogs/panel_sym_display_options.cpp
    dialogs/panel_sym_editing_options.cpp
    dialogs/panel_sym_lib_table.cpp
    dialogs/panel_template_fieldnames.cpp
    dialogs/sch_text_syntax_help.cpp
    eeschema_config.cpp
    eeschema_jobs_handler_gui.cpp
    erc/erc_tree_model.cpp
    fields_editor_grid_data_model.cpp
    fields_grid_table.cpp
    files-io.cpp
    generate_alias_info.cpp
    import_gfx/dialog_import_gfx_sch.cpp
    lib_fields_data_model.cpp
    menubar.cpp
    net_navigator.cpp
    netlist_exporters/netlist_generator.cpp
    picksymbol.cpp
    printing/dialog_print.cpp
    printing/sch_printout.cpp
    project_rescue.cpp
    remote_symbol_download_manager.cpp
    remote_symbol_import_job.cpp
    remote_symbol_import_utils.cpp
    sch_base_frame.cpp
    sch_commit_gui.cpp
    sch_design_block_utils.cpp
    sch_draw_panel.cpp
    sch_edit_frame.cpp
    sch_footprint_field_reconciler.cpp
    sch_item_alignment.cpp
    sch_preview_panel.cpp
    sch_validators.cpp
    schematic_undo_redo.cpp
    sheet.cpp
    sim/legacy_workbook.cpp
    sim/sim_plot_colors.cpp
    sim/sim_plot_tab.cpp
    sim/sim_property.cpp
    sim/sim_tab.cpp
    sim/simulator_frame.cpp
    sim/simulator_frame_ui.cpp
    sim/spice_validator.cpp
    sim/toolbars_simulator_frame.cpp
    symbol_chooser_frame.cpp
    symbol_editor/lib_logger.cpp
    symbol_editor/lib_symbol_library_manager.cpp
    symbol_editor/menubar_symbol_editor.cpp
    symbol_editor/symbol_edit_frame.cpp
    symbol_editor/symbol_editor.cpp
    symbol_editor/symbol_editor_import_export.cpp
    symbol_editor/symbol_editor_plotter.cpp
    symbol_editor/symbol_editor_undo_redo.cpp
    symbol_editor/toolbars_symbol_editor.cpp
    symbol_import_manager.cpp
    symbol_library_manager.cpp
    symbol_tree_model_adapter.cpp
    symbol_tree_synchronizing_adapter.cpp
    symbol_viewer_frame.cpp
    sync_sheet_pin/dialog_sync_sheet_pins.cpp
    sync_sheet_pin/panel_sync_sheet_pins.cpp
    sync_sheet_pin/sheet_synchronization_agent.cpp
    sync_sheet_pin/sheet_synchronization_item.cpp
    sync_sheet_pin/sheet_synchronization_model.cpp
    sync_sheet_pin/sheet_synchronization_notifier.cpp
    toolbars_sch_editor.cpp
    toolbars_symbol_viewer.cpp
    tools/assign_footprints.cpp
    tools/backannotate.cpp
    tools/ee_grid_helper.cpp
    tools/rule_area_create_helper.cpp
    tools/sch_actions.cpp
    tools/sch_align_tool.cpp
    tools/sch_design_block_control.cpp
    tools/sch_drag_net_collision.cpp
    tools/sch_drawing_tools.cpp
    tools/sch_edit_table_tool.cpp
    tools/sch_edit_tool.cpp
    tools/sch_editor_conditions.cpp
    tools/sch_editor_control.cpp
    tools/sch_find_replace_tool.cpp
    tools/sch_group_tool.cpp
    tools/sch_inspection_tool.cpp
    tools/sch_line_wire_bus_tool.cpp
    tools/sch_move_tool.cpp
    tools/sch_navigate_tool.cpp
    tools/sch_point_editor.cpp
    tools/sch_selection_tool.cpp
    tools/sch_tool_utils.cpp
    tools/simulator_control.cpp
    tools/symbol_editor_control.cpp
    tools/symbol_editor_drawing_tools.cpp
    tools/symbol_editor_edit_tool.cpp
    tools/symbol_editor_move_tool.cpp
    tools/symbol_editor_pin_tool.cpp
    widgets/hierarchy_pane.cpp
    widgets/panel_sch_selection_filter.cpp
    widgets/panel_symbol_chooser.cpp
    widgets/pinshape_combobox.cpp
    widgets/pintype_combobox.cpp
    widgets/sch_design_block_pane.cpp
    widgets/sch_design_block_preview_widget.cpp
    widgets/sch_field_autocomplete.cpp
    widgets/sch_properties_panel.cpp
    widgets/sch_search_pane.cpp
    widgets/search_handlers.cpp
    widgets/symbol_diff_widget.cpp
    widgets/symbol_filter_combobox.cpp
    widgets/symbol_preview_widget.cpp
    widgets/tuner_slider.cpp
    )

list( TRANSFORM KIQT_EESCHEMA_GUI_SRCS PREPEND ${CMAKE_SOURCE_DIR}/eeschema/ )

kiqt_generate_fbp_compat( KIQT_EESCHEMA_COMPAT_FBP_SRCS
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_annotate_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_bom_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_change_symbols_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_database_lib_settings_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_edit_symbols_libid_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_erc_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_export_netlist_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_field_properties_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_global_edit_text_and_graphics_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_ibis_parser_reporter_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_image_properties_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_import_symbol_select_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_increment_annotations.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_junction_props_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_label_properties_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_lib_edit_pin_table_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_lib_fields_table_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_lib_new_symbol_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_lib_symbol_properties_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_line_properties_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_migrate_buses_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_pin_properties_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_plot_schematic_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_rescue_each_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_sch_find_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_sch_import_settings_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_shape_properties_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_sheet_pin_properties_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_sheet_properties_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_sim_command_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_sim_format_value_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_sim_model_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_symbol_fields_table_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_symbol_properties_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_symbol_remap_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_table_properties_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_tablecell_properties_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_text_properties_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_update_from_pcb_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_update_symbol_fields_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_user_defined_signals_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/dialog_wire_bus_properties_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/panel_bom_presets.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/panel_eeschema_annotation_options_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/panel_eeschema_display_options_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/panel_eeschema_editing_options_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/panel_setup_buses_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/panel_setup_formatting_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/panel_setup_pinmap_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/panel_simulator_preferences.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/panel_sym_color_settings_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/panel_sym_display_options_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/panel_sym_editing_options_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/panel_sym_lib_table_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs/panel_template_fieldnames_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/import_gfx/dialog_import_gfx_sch_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/printing/dialog_print_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/sim/simulator_frame_ui_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/sync_sheet_pin/dialog_sync_sheet_pins_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/sync_sheet_pin/panel_sync_sheet_pins_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/widgets/panel_sch_selection_filter_base.fbp
    ${CMAKE_SOURCE_DIR}/eeschema/widgets/tuner_slider_base.fbp
    )

add_library( eeschema_gui_qt STATIC
    ${KIQT_EESCHEMA_GUI_SRCS}
    ${KIQT_EESCHEMA_COMPAT_FBP_SRCS}
    widgets/panel_remote_symbol_qt.cpp
    widgets/symbol_tree_pane_qt.cpp
    sch_edit_frame_qt.cpp
    symbol_edit_frame_qt.cpp
    symbol_viewer_frame_qt.cpp
    widgets/sch_design_block_pane_qt.cpp
    widgets/sch_design_block_preview_widget_qt.cpp
    widgets/sch_properties_panel_qt.cpp
    ${CMAKE_SOURCE_DIR}/common/widgets/mathplot.cpp
    )

set_target_properties( eeschema_gui_qt PROPERTIES POSITION_INDEPENDENT_CODE ON )

# The code of KiCad includes Python.h, whose structures have members named as the keywords of Qt
target_compile_definitions( eeschema_gui_qt PRIVATE QT_NO_KEYWORDS )
kiqt_gui_target( eeschema_gui_qt )

# Compiled as the eeschema KIFACE (its include directories and definitions), the Qt headers (and the
# bases of the dialogs generated from the wxFormBuilder files) first
target_include_directories( eeschema_gui_qt BEFORE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs
    ${KIQT_FBP_DIR}/eeschema
    ${KIQT_FBP_DIR}/eeschema/dialogs
    ${KIQT_FBP_DIR}/eeschema/import_gfx
    ${KIQT_FBP_DIR}/eeschema/printing
    ${KIQT_FBP_DIR}/eeschema/sim
    ${KIQT_FBP_DIR}/eeschema/sync_sheet_pin
    ${KIQT_FBP_DIR}/eeschema/widgets
    )
target_include_directories( eeschema_gui_qt PRIVATE
    $<TARGET_PROPERTY:eeschema_kiface_objects,INCLUDE_DIRECTORIES>
    )
target_compile_definitions( eeschema_gui_qt PRIVATE
    $<TARGET_PROPERTY:eeschema_kiface_objects,COMPILE_DEFINITIONS>
    )

target_link_libraries( eeschema_gui_qt
    PRIVATE
        kiqt_common
        markdown_lib
        nanosvg
        nlohmann_json
        Boost::headers
    )


# _eeschema_qtgui.kiface: the KIFACE of eeschema with its GUI on Qt, as _eeschema.kiface of the
# wxWidgets build: the IFACE of eeschema.cpp (its frames, panels and dialogs) and all the objects of
# eeschema_gui_qt, with the archives of the headless KIFACE.
add_headless_kiface( eeschema_qtgui_kiface
    OUTPUT_NAME eeschema_qtgui
    LIKE        eeschema_cli_kiface
    SOURCES     ${CMAKE_SOURCE_DIR}/eeschema/eeschema.cpp
                ${KIQT_KIFACE_SOURCES}
    USAGE       kiqt_common
    ARCHIVES    kiqt_common
    LIBRARIES   kiqt_common kiqt Qt6::Widgets dxflib_qcad markdown_lib nanosvg tinyspline_lib
    )

# eeschema.cpp is compiled as the GUI of eeschema (its Qt headers first)
target_compile_definitions( eeschema_qtgui_kiface_iface PRIVATE QT_NO_KEYWORDS )
target_include_directories( eeschema_qtgui_kiface_iface BEFORE PRIVATE
    $<TARGET_PROPERTY:eeschema_gui_qt,INCLUDE_DIRECTORIES>
    )
target_compile_definitions( eeschema_qtgui_kiface_iface PRIVATE
    $<TARGET_PROPERTY:eeschema_gui_qt,COMPILE_DEFINITIONS>
    )

# All the objects of the GUI: the tools and the actions register themselves when they are built
target_sources( eeschema_qtgui_kiface PRIVATE $<TARGET_OBJECTS:eeschema_gui_qt> )
add_dependencies( eeschema_qtgui_kiface eeschema_gui_qt )

if( UNIX AND NOT APPLE )
    target_link_libraries( eeschema_qtgui_kiface PRIVATE ${SPNAV_LIBRARIES} )
endif()

set_target_properties( eeschema_qtgui_kiface PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/eeschema
    )

install( TARGETS eeschema_qtgui_kiface
    DESTINATION ${KICAD_KIFACE}
    COMPONENT binary
    )
