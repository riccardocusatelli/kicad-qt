# _pcbnew_qt.kiface: the KIFACE of pcbnew without GUI (as _pcbnew_cli.kiface), with the board
# documents of the Qt shell

add_headless_kiface( pcbnew_qt_kiface
    OUTPUT_NAME pcbnew_qt
    LIKE        pcbnew_cli_kiface
    SOURCES     ${CMAKE_SOURCE_DIR}/pcbnew/pcbnew_cli.cpp
                ${CMAKE_SOURCE_DIR}/pcbnew/generators/pcb_tuning_pattern_headless.cpp
                board_document.cpp
                board_tools.cpp
                ${KIQT_KIFACE_SOURCES}
    USAGE       kiqt_common
    ARCHIVES    3d-viewer_qt kiqt_common
    LIBRARIES   kiqt_common kiqt Qt6::Widgets
    )

set_target_properties( pcbnew_qt_kiface_iface PROPERTIES AUTOMOC ON )
target_include_directories( pcbnew_qt_kiface_iface PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} )
target_compile_definitions( pcbnew_qt_kiface_iface PRIVATE KICAD_QT_MODULE )

# Next to the other pcbnew KIFACEs (KIWAY_BASE finds them there when run from the build tree)
set_target_properties( pcbnew_qt_kiface PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/pcbnew
    )

install( TARGETS pcbnew_qt_kiface
    DESTINATION ${KICAD_KIFACE}
    COMPONENT binary
    )


# The GUI of pcbnew on Qt (frames, tools, panels, dialogs): the sources of the pcbnew KIFACE with a
# GUI of the wxWidgets build (pcbnew_kiface_objects, and the GUI parts of pcbcommon) that build on
# Qt, with the Qt versions of the panels and dialogs.  The KIFACE links it once all of pcbnew is
# ported (phase 3B.5).
set( KIQT_PCBNEW_GUI_SRCS
    array_pad_number_provider.cpp
    autorouter/ar_autoplacer.cpp
    autorouter/ar_matrix.cpp
    autorouter/spread_footprints.cpp
    board_stackup_manager/board_stackup_reporter.cpp
    board_stackup_manager/dielectric_material.cpp
    board_tables/board_characteristics_table.cpp
    board_tables/board_stackup_table.cpp
    build_BOM_from_board.cpp
    cleanup_item.cpp
    component_classes/component_class.cpp
    component_classes/component_class_assignment_rule.cpp
    component_classes/component_class_cache_proxy.cpp
    component_classes/component_class_manager.cpp
    convert_shape_list_to_polygon.cpp
    cross-probing.cpp
    drc/drc_interactive_courtyard_clearance.cpp
    drc/drc_test_provider.cpp
    drc/rule_editor/drc_re_panel_matcher.cpp
    drc/rule_editor/drc_re_rule_saver.cpp
    edit_track_width.cpp
    exporters/export_footprint_associations.cpp
    exporters/export_hyperlynx.cpp
    exporters/export_idf.cpp
    footprint_editor_settings.cpp
    footprint_import_reconciler.cpp
    footprint_info_impl.cpp
    footprint_wizard.cpp
    fp_tree_model_adapter.cpp
    generate_footprint_info.cpp
    git/kigit_pcb_merge.cpp
    graphics_cleaner.cpp
    import_gfx/graphics_importer_pcbnew.cpp
    layer_pairs.cpp
    microwave/microwave_footprint.cpp
    microwave/microwave_inductor.cpp
    microwave/microwave_tool.cpp
    netlist_reader/board_netlist_updater.cpp
    netlist_reader/netlist.cpp
    padstack.cpp
    pcb_base_frame.cpp
    pcb_base_edit_frame.cpp
    pcb_draw_panel_gal.cpp
    pcb_edit_frame.cpp
    pcb_layer_box_selector.cpp
    pcb_origin_transforms.cpp
    pcb_screen.cpp
    pcbnew_settings.cpp
    ratsnest/ratsnest.cpp
    ratsnest/ratsnest_view_item.cpp
    toolbars_footprint_wizard.cpp
    tools/align_distribute_tool.cpp
    tools/footprint_chooser_selection_tool.cpp
    tools/generator_tool_pns_proxy.cpp
    tools/item_modification_routine.cpp
    tools/pcb_editor_conditions.cpp
    tools/pcb_grid_helper.cpp
    tools/pcb_selection.cpp
    tools/pcb_selection_conditions.cpp
    tools/pcb_selection_tool.cpp
    tools/pcb_tool_base.cpp
    tools/pcb_tool_utils.cpp
    tools/tool_event_utils.cpp
    tools/zone_create_helper.cpp
    tracks_cleaner.cpp
    widgets/appearance_controls.cpp
    widgets/pcb_design_block_preview_widget.cpp
    widgets/pcb_net_inspector_panel.cpp
    widgets/pcb_properties_panel.cpp
    widgets/pcb_search_pane.cpp
    widgets/search_handlers.cpp
    zone_manager/board_edges_bounding_item.cpp
    zone_settings_bag.cpp
    )

list( TRANSFORM KIQT_PCBNEW_GUI_SRCS PREPEND ${CMAKE_SOURCE_DIR}/pcbnew/ )

kiqt_generate_fbp( KIQT_PCBNEW_GUI_FBP_SRCS
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_filter_selection_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_move_exact_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/widgets/appearance_controls_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/widgets/panel_selection_filter_base.fbp
    )

add_library( pcbnew_gui_qt STATIC
    ${KIQT_PCBNEW_GUI_SRCS}
    ${KIQT_PCBNEW_GUI_FBP_SRCS}
    ${CMAKE_SOURCE_DIR}/common/widgets/net_selector.cpp
    dialogs/dialog_filter_selection.cpp
    dialogs/dialog_move_exact.cpp
    pcb_base_edit_frame_qt.cpp
    pcb_edit_frame_qt.cpp
    pcb_layer_box_selector_qt.cpp
    widgets/appearance_controls_qt.cpp
    widgets/panel_selection_filter.cpp
    widgets/pcb_design_block_pane_qt.cpp
    widgets/pcb_design_block_preview_widget_qt.cpp
    widgets/pcb_net_inspector_panel_qt.cpp
    widgets/pcb_properties_panel_qt.cpp
    widgets/vertex_editor_pane.cpp
    )

set_target_properties( pcbnew_gui_qt PROPERTIES POSITION_INDEPENDENT_CODE ON )

# The code of KiCad includes Python.h, whose structures have members named as the keywords of Qt
target_compile_definitions( pcbnew_gui_qt PRIVATE QT_NO_KEYWORDS )

# Compiled as the pcbnew KIFACE (its include directories and definitions), the Qt headers (and the
# bases of the panels generated from the wxFormBuilder files) first
target_include_directories( pcbnew_gui_qt BEFORE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs
    ${CMAKE_SOURCE_DIR}/qt/3d-viewer
    ${KIQT_FBP_DIR}/pcbnew
    ${KIQT_FBP_DIR}/pcbnew/dialogs
    ${KIQT_FBP_DIR}/3d-viewer
    )
target_include_directories( pcbnew_gui_qt PRIVATE
    $<TARGET_PROPERTY:pcbnew_kiface_objects,INCLUDE_DIRECTORIES>
    ${CMAKE_SOURCE_DIR}/3d-viewer
    )
target_compile_definitions( pcbnew_gui_qt PRIVATE
    $<TARGET_PROPERTY:pcbnew_kiface_objects,COMPILE_DEFINITIONS>
    )

target_link_libraries( pcbnew_gui_qt
    PRIVATE
        kiqt_common
        nlohmann_json
        Boost::headers
    )
