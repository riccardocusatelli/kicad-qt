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
    action_plugin.cpp
    api/api_handler_pcb.cpp
    array_pad_number_provider.cpp
    autorouter/ar_autoplacer.cpp
    autorouter/ar_matrix.cpp
    autorouter/autoplace_tool.cpp
    autorouter/spread_footprints.cpp
    board_commit_gui.cpp
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
    connectivity/topo_match.cpp
    convert_shape_list_to_polygon.cpp
    cross-probing.cpp
    drc/drc_interactive_courtyard_clearance.cpp
    drc/drc_test_provider.cpp
    drc/rule_editor/dialog_drc_rule_editor.cpp
    drc/rule_editor/drc_re_abs_length_two_overlay_panel.cpp
    drc/rule_editor/drc_re_allowed_orientation_overlay_panel.cpp
    drc/rule_editor/drc_re_bitmap_overlay_panel.cpp
    drc/rule_editor/drc_re_bool_input_overlay_panel.cpp
    drc/rule_editor/drc_re_condition_group_panel.cpp
    drc/rule_editor/drc_re_condition_row_panel.cpp
    drc/rule_editor/drc_re_custom_rule_panel.cpp
    drc/rule_editor/drc_re_matched_length_diff_pair_overlay_panel.cpp
    drc/rule_editor/drc_re_min_txt_ht_th_overlay_panel.cpp
    drc/rule_editor/drc_re_numeric_input_overlay_panel.cpp
    drc/rule_editor/drc_re_object_selector_panel.cpp
    drc/rule_editor/drc_re_overlay_field.cpp
    drc/rule_editor/drc_re_panel_matcher.cpp
    drc/rule_editor/drc_re_permitted_layers_overlay_panel.cpp
    drc/rule_editor/drc_re_routing_width_overlay_panel.cpp
    drc/rule_editor/drc_re_rtg_diff_pair_overlay_panel.cpp
    drc/rule_editor/drc_re_rule_loader.cpp
    drc/rule_editor/drc_re_rule_saver.cpp
    drc/rule_editor/drc_re_validator_checkbox_list.cpp
    drc/rule_editor/drc_re_validator_combo_ctrl.cpp
    drc/rule_editor/drc_re_validator_min_max_ctrl.cpp
    drc/rule_editor/drc_re_validator_min_preferred_max_ctrl.cpp
    drc/rule_editor/drc_re_validator_numeric_ctrl.cpp
    drc/rule_editor/drc_re_via_style_overlay_panel.cpp
    drc/rule_editor/drc_re_vias_under_smd_overlay_panel.cpp
    drc/rule_editor/drc_rule_editor_utils.cpp
    drc/rule_editor/panel_drc_group_header.cpp
    drc/rule_editor/panel_drc_rule_editor.cpp
    edit_track_width.cpp
    edit_zone_helpers.cpp
    exporters/export_footprint_associations.cpp
    exporters/export_hyperlynx.cpp
    exporters/export_idf.cpp
    files.cpp
    footprint_edit_frame.cpp
    footprint_editor_utils.cpp
    footprint_editor_settings.cpp
    footprint_import_reconciler.cpp
    footprint_libraries_utils.cpp
    footprint_info_impl.cpp
    footprint_preview_panel.cpp
    footprint_wizard.cpp
    fp_tree_model_adapter.cpp
    fp_tree_synchronizing_adapter.cpp
    generate_footprint_info.cpp
    generators/pcb_tuning_pattern.cpp
    generators/pcb_tuning_pattern_edit.cpp
    git/kigit_pcb_merge.cpp
    graphics_cleaner.cpp
    grid_layer_box_helpers.cpp
    import_gfx/dialog_import_graphics.cpp
    import_gfx/graphics_importer_pcbnew.cpp
    initpcb.cpp
    kicad_clipboard_gui.cpp
    layer_pairs.cpp
    load_select_footprint.cpp
    menubar_footprint_editor.cpp
    menubar_pcb_editor.cpp
    microwave/microwave_footprint.cpp
    microwave/microwave_inductor.cpp
    microwave/microwave_polygon.cpp
    microwave/microwave_tool.cpp
    netlist_reader/board_netlist_updater.cpp
    netlist_reader/netlist.cpp
    padstack.cpp
    pcb_base_edit_frame.cpp
    pcb_base_frame.cpp
    pcb_design_block_utils.cpp
    pcb_draw_panel_gal.cpp
    pcb_edit_frame.cpp
    pcb_edit_frame_dialogs.cpp
    pcb_edit_frame_export.cpp
    pcb_fields_grid_table.cpp
    pcb_layer_box_selector.cpp
    pcb_origin_transforms.cpp
    pcb_screen.cpp
    pcbnew_config.cpp
    pcbnew_settings.cpp
    python/scripting/pcb_scripting_tool.cpp
    python/scripting/pcbnew_action_plugins.cpp
    python/scripting/pcbnew_scripting.cpp
    python/scripting/pcbnew_scripting_helpers_gui.cpp
    ratsnest/ratsnest.cpp
    ratsnest/ratsnest_view_item.cpp
    router/pns_kicad_iface.cpp
    router/pns_tool_base.cpp
    router/router_preview_item.cpp
    router/router_tool.cpp
    sel_layer.cpp
    toolbars_footprint_editor.cpp
    toolbars_footprint_wizard.cpp
    toolbars_pcb_editor.cpp
    tools/align_distribute_tool.cpp
    tools/array_tool.cpp
    tools/board_editor_control.cpp
    tools/board_editor_control_export.cpp
    tools/board_inspection_tool.cpp
    tools/board_reannotate_tool.cpp
    tools/convert_tool.cpp
    tools/drawing_tool.cpp
    tools/drc_rule_editor_tool.cpp
    tools/drc_tool.cpp
    tools/edit_tool.cpp
    tools/edit_tool_move_fct.cpp
    tools/footprint_chooser_selection_tool.cpp
    tools/footprint_editor_control.cpp
    tools/generator_tool.cpp
    tools/generator_tool_pns_proxy.cpp
    tools/global_edit_tool.cpp
    tools/item_modification_routine.cpp
    tools/multichannel_tool.cpp
    tools/pad_tool.cpp
    tools/pcb_actions.cpp
    tools/pcb_control.cpp
    tools/pcb_design_block_control.cpp
    tools/pcb_edit_table_tool.cpp
    tools/pcb_editor_conditions.cpp
    tools/pcb_grid_helper.cpp
    tools/pcb_group_tool.cpp
    tools/pcb_picker_tool.cpp
    tools/pcb_point_editor.cpp
    tools/pcb_selection.cpp
    tools/pcb_selection_conditions.cpp
    tools/pcb_selection_tool.cpp
    tools/pcb_tool_base.cpp
    tools/pcb_tool_utils.cpp
    tools/pcb_viewer_tools.cpp
    tools/position_relative_tool.cpp
    tools/tool_event_utils.cpp
    tools/zone_create_helper.cpp
    tools/zone_filler_tool.cpp
    tracks_cleaner.cpp
    undo_redo.cpp
    widgets/appearance_controls.cpp
    widgets/panel_footprint_chooser.cpp
    widgets/pcb_design_block_preview_widget.cpp
    widgets/pcb_net_inspector_panel.cpp
    widgets/pcb_properties_panel.cpp
    widgets/pcb_search_pane.cpp
    widgets/search_handlers.cpp
    zone_layer_properties_grid.cpp
    zone_manager/board_edges_bounding_item.cpp
    zone_manager/dialog_zone_manager.cpp
    zone_manager/model_zones_overview.cpp
    zone_manager/zone_preview_canvas.cpp
    zone_manager/zone_preview_notebook.cpp
    zone_settings_bag.cpp
    )

list( TRANSFORM KIQT_PCBNEW_GUI_SRCS PREPEND ${CMAKE_SOURCE_DIR}/pcbnew/ )

kiqt_generate_fbp( KIQT_PCBNEW_GUI_FBP_SRCS
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_move_exact_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/widgets/appearance_controls_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/widgets/panel_selection_filter_base.fbp
    )

# The dialogs of pcbnew built unchanged on the wxWidgets compatibility layer (qt/wxcompat)
kiqt_generate_fbp_compat( KIQT_PCBNEW_COMPAT_FBP_SRCS
    ${CMAKE_SOURCE_DIR}/common/dialogs/dialog_assign_netclass_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/dialog_HTML_reporter_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/dialog_import_choose_project_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/dialog_rc_job_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/panel_color_settings_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/panel_gal_options_base.fbp
    ${CMAKE_SOURCE_DIR}/common/dialogs/panel_image_editor_base.fbp
    ${CMAKE_SOURCE_DIR}/common/widgets/wx_html_report_panel_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/board_stackup_manager/dialog_dielectric_list_manager_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/board_stackup_manager/panel_board_finish_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/board_stackup_manager/panel_board_stackup_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_barcode_properties_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_board_reannotate_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_board_statistics_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_board_stats_job_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_cleanup_graphics_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_cleanup_tracks_and_vias_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_copper_zones_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_create_array_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_dimension_properties_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_drc_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_enum_pads_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_exchange_footprints_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_export_2581.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_export_idf_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_export_odbpp_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_export_step_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_export_step_process_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_export_vrml_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_filter_selection_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_find_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_footprint_associations_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_footprint_checker_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_footprint_properties_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_footprint_properties_fp_editor_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_footprint_wizard_list_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_fp_edit_pad_table_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_gen_footprint_position_file_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_gendrill_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_generators_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_get_footprint_by_name_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_global_deletion_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_global_edit_teardrops_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_global_edit_text_and_graphics_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_global_edit_tracks_and_vias_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_import_netlist_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_import_settings_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_imported_layers_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_layer_selection_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_migrate_3d_models_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_multichannel_generate_rule_areas_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_multichannel_repeat_layout_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_non_copper_zones_properties_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_offset_item_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_outset_items_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_pad_properties_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_plot_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_pns_diff_pair_dimensions_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_pns_settings_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_position_relative_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_push_pad_properties_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_reference_image_properties_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_render_job_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_rule_area_properties_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_shape_properties_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_swap_layers_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_table_properties_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_tablecell_properties_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_target_properties_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_text_properties_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_textbox_properties_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_track_via_properties_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_track_via_size_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_tuning_pattern_properties_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_unused_pad_layers_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/dialog_update_pcb_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/panel_assign_component_classes_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/panel_display_options_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/panel_edit_options_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/panel_fp_editor_field_defaults_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/panel_fp_editor_graphics_defaults_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/panel_fp_lib_table_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/panel_fp_properties_3d_model_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/panel_fp_user_layer_names_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/panel_pcbnew_action_plugins_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/panel_pcbnew_display_origin_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/panel_rule_area_properties_keepout_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/panel_rule_area_properties_placement_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/panel_setup_constraints_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/panel_setup_defaults_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/panel_setup_dimensions_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/panel_setup_formatting_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/panel_setup_layers_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/panel_setup_mask_and_paste_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/panel_setup_rules_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/panel_setup_teardrops_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/panel_setup_text_and_graphics_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/panel_setup_tracks_and_vias_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/panel_setup_tuning_patterns_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/panel_setup_tuning_profile_info_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/panel_setup_tuning_profiles_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/panel_setup_zone_hatch_offsets_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/panel_setup_zones_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs/panel_zone_properties_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/drc/rule_editor/panel_drc_group_header_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/drc/rule_editor/panel_drc_rule_editor_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/import_gfx/dialog_import_graphics_base.fbp
    ${CMAKE_SOURCE_DIR}/pcbnew/zone_manager/dialog_zone_manager_base.fbp
    )

set( KIQT_PCBNEW_COMPAT_DIALOGS
    board_stackup_manager/dialog_dielectric_list_manager.cpp
    board_stackup_manager/panel_board_finish.cpp
    board_stackup_manager/panel_board_stackup.cpp
    board_stackup_manager/stackup_predefined_prms.cpp
    dialogs/dialog_barcode_properties.cpp
    dialogs/dialog_board_reannotate.cpp
    dialogs/dialog_board_setup.cpp
    dialogs/dialog_board_statistics.cpp
    dialogs/dialog_board_stats_job.cpp
    dialogs/dialog_cleanup_graphics.cpp
    dialogs/dialog_cleanup_tracks_and_vias.cpp
    dialogs/dialog_copper_zones.cpp
    dialogs/dialog_create_array.cpp
    dialogs/dialog_dimension_properties.cpp
    dialogs/dialog_drc.cpp
    dialogs/dialog_drc_job_config.cpp
    dialogs/dialog_enum_pads.cpp
    dialogs/dialog_exchange_footprints.cpp
    dialogs/dialog_export_2581.cpp
    dialogs/dialog_export_idf.cpp
    dialogs/dialog_export_odbpp.cpp
    dialogs/dialog_export_step.cpp
    dialogs/dialog_export_vrml.cpp
    dialogs/dialog_filter_selection.cpp
    dialogs/dialog_find.cpp
    dialogs/dialog_footprint_associations.cpp
    dialogs/dialog_footprint_checker.cpp
    dialogs/dialog_footprint_properties.cpp
    dialogs/dialog_footprint_properties_fp_editor.cpp
    dialogs/dialog_fp_edit_pad_table.cpp
    dialogs/dialog_gen_footprint_position.cpp
    dialogs/dialog_gencad_export_options.cpp
    dialogs/dialog_gendrill.cpp
    dialogs/dialog_generators.cpp
    dialogs/dialog_get_footprint_by_name.cpp
    dialogs/dialog_global_deletion.cpp
    dialogs/dialog_global_edit_teardrops.cpp
    dialogs/dialog_global_edit_text_and_graphics.cpp
    dialogs/dialog_global_edit_tracks_and_vias.cpp
    dialogs/dialog_import_netlist.cpp
    dialogs/dialog_import_settings.cpp
    dialogs/dialog_items_list.cpp
    dialogs/dialog_map_layers.cpp
    dialogs/dialog_migrate_3d_models.cpp
    dialogs/dialog_multichannel_generate_rule_areas.cpp
    dialogs/dialog_multichannel_repeat_layout.cpp
    dialogs/dialog_non_copper_zones_properties.cpp
    dialogs/dialog_offset_item.cpp
    dialogs/dialog_outset_items.cpp
    dialogs/dialog_pad_properties.cpp
    dialogs/dialog_plot.cpp
    dialogs/dialog_pns_diff_pair_dimensions.cpp
    dialogs/dialog_pns_settings.cpp
    dialogs/dialog_position_relative.cpp
    dialogs/dialog_push_pad_properties.cpp
    dialogs/dialog_reference_image_properties.cpp
    dialogs/dialog_render_job.cpp
    dialogs/dialog_rule_area_properties.cpp
    dialogs/dialog_shape_properties.cpp
    dialogs/dialog_swap_layers.cpp
    dialogs/dialog_table_properties.cpp
    dialogs/dialog_tablecell_properties.cpp
    dialogs/dialog_target_properties.cpp
    dialogs/dialog_text_properties.cpp
    dialogs/dialog_textbox_properties.cpp
    dialogs/dialog_track_via_properties.cpp
    dialogs/dialog_track_via_size.cpp
    dialogs/dialog_tuning_pattern_properties.cpp
    dialogs/dialog_unused_pad_layers.cpp
    dialogs/dialog_update_pcb.cpp
    dialogs/panel_assign_component_classes.cpp
    dialogs/panel_display_options.cpp
    dialogs/panel_edit_options.cpp
    dialogs/panel_fp_editor_color_settings.cpp
    dialogs/panel_fp_editor_field_defaults.cpp
    dialogs/panel_fp_editor_graphics_defaults.cpp
    dialogs/panel_fp_lib_table.cpp
    dialogs/panel_fp_properties_3d_model.cpp
    dialogs/panel_fp_user_layer_names.cpp
    dialogs/panel_pcbnew_action_plugins.cpp
    dialogs/panel_pcbnew_color_settings.cpp
    dialogs/panel_pcbnew_display_origin.cpp
    dialogs/panel_setup_constraints.cpp
    dialogs/panel_setup_defaults.cpp
    dialogs/panel_setup_dimensions.cpp
    dialogs/panel_setup_formatting.cpp
    dialogs/panel_setup_layers.cpp
    dialogs/panel_setup_mask_and_paste.cpp
    dialogs/panel_setup_rules.cpp
    dialogs/panel_setup_teardrops.cpp
    dialogs/panel_setup_text_and_graphics.cpp
    dialogs/panel_setup_tracks_and_vias.cpp
    dialogs/panel_setup_tuning_patterns.cpp
    dialogs/panel_setup_tuning_profile_info.cpp
    dialogs/panel_setup_tuning_profiles.cpp
    dialogs/panel_setup_zone_hatch_offsets.cpp
    dialogs/panel_setup_zones.cpp
    dialogs/panel_zone_properties.cpp
    dialogs/pcb_text_syntax_help.cpp
    )

list( TRANSFORM KIQT_PCBNEW_COMPAT_DIALOGS PREPEND ${CMAKE_SOURCE_DIR}/pcbnew/ )

# The push and shove router (the pnsrouter library of pcbnew, compiled with the Qt headers here)
set( KIQT_PNS_SRCS
    pns_algo_base.cpp
    pns_arc.cpp
    pns_component_dragger.cpp
    pns_diff_pair.cpp
    pns_diff_pair_placer.cpp
    pns_dp_meander_placer.cpp
    pns_dragger.cpp
    pns_hole.cpp
    pns_index.cpp
    pns_item.cpp
    pns_itemset.cpp
    pns_line.cpp
    pns_line_placer.cpp
    pns_logger.cpp
    pns_meander.cpp
    pns_meander_placer.cpp
    pns_meander_placer_base.cpp
    pns_meander_skew_placer.cpp
    pns_mouse_trail_tracer.cpp
    pns_multi_dragger.cpp
    pns_node.cpp
    pns_optimizer.cpp
    pns_router.cpp
    pns_routing_settings.cpp
    pns_shove.cpp
    pns_sizes_settings.cpp
    pns_solid.cpp
    pns_topology.cpp
    pns_utils.cpp
    pns_via.cpp
    pns_walkaround.cpp
    router_status_view_item.cpp
    time_limit.cpp
    )

list( TRANSFORM KIQT_PNS_SRCS PREPEND ${CMAKE_SOURCE_DIR}/pcbnew/router/ )

add_library( pcbnew_gui_qt STATIC
    ${KIQT_PNS_SRCS}
    ${KIQT_PCBNEW_COMPAT_FBP_SRCS}
    ${KIQT_PCBNEW_COMPAT_DIALOGS}
    ${KIQT_PCBNEW_GUI_SRCS}
    ${KIQT_PCBNEW_GUI_FBP_SRCS}
    ${CMAKE_SOURCE_DIR}/common/dialogs/dialog_book_reporter.cpp
    ${CMAKE_SOURCE_DIR}/common/dialogs/dialog_group_properties.cpp
    ${CMAKE_SOURCE_DIR}/common/dialogs/dialog_assign_netclass.cpp
    ${CMAKE_SOURCE_DIR}/common/dialogs/dialog_import_choose_project.cpp
    ${CMAKE_SOURCE_DIR}/common/dialogs/dialog_multi_unit_entry.cpp
    ${CMAKE_SOURCE_DIR}/common/dialogs/dialog_paste_special.cpp
    ${CMAKE_SOURCE_DIR}/common/dialogs/dialog_rc_job.cpp
    ${CMAKE_SOURCE_DIR}/common/dialogs/panel_color_settings.cpp
    ${CMAKE_SOURCE_DIR}/common/dialogs/panel_gal_options.cpp
    ${CMAKE_SOURCE_DIR}/common/dialogs/panel_image_editor.cpp
    ${CMAKE_SOURCE_DIR}/common/dialogs/panel_setup_netclasses.cpp
    ${CMAKE_SOURCE_DIR}/common/dialogs/rule_editor_dialog_base.cpp
    ${CMAKE_SOURCE_DIR}/common/drawing_sheet/ds_data_model_io.cpp
    ${CMAKE_SOURCE_DIR}/common/drawing_sheet/ds_proxy_undo_item.cpp
    ${CMAKE_SOURCE_DIR}/common/import_gfx/dxf_import_plugin.cpp
    ${CMAKE_SOURCE_DIR}/common/import_gfx/graphics_import_mgr.cpp
    ${CMAKE_SOURCE_DIR}/common/libraries/lib_table_grid_data_model.cpp
    ${CMAKE_SOURCE_DIR}/common/pin_numbers.cpp
    ${CMAKE_SOURCE_DIR}/common/status_popup.cpp
    ${CMAKE_SOURCE_DIR}/common/tool/design_block_control.cpp
    ${CMAKE_SOURCE_DIR}/common/tool/embed_tool.cpp
    ${CMAKE_SOURCE_DIR}/common/tool/group_tool.cpp
    ${CMAKE_SOURCE_DIR}/common/tool/properties_tool.cpp
    ${CMAKE_SOURCE_DIR}/common/transline_calculations/coupled_microstrip.cpp
    ${CMAKE_SOURCE_DIR}/common/transline_calculations/coupled_stripline.cpp
    ${CMAKE_SOURCE_DIR}/common/transline_calculations/microstrip.cpp
    ${CMAKE_SOURCE_DIR}/common/transline_calculations/stripline.cpp
    ${CMAKE_SOURCE_DIR}/common/transline_calculations/transline_calculation_base.cpp
    ${CMAKE_SOURCE_DIR}/common/widgets/area_selector.cpp
    ${CMAKE_SOURCE_DIR}/common/widgets/footprint_diff_widget.cpp
    ${CMAKE_SOURCE_DIR}/common/widgets/footprint_preview_widget.cpp
    ${CMAKE_SOURCE_DIR}/common/widgets/margin_offset_binder.cpp
    ${CMAKE_SOURCE_DIR}/common/widgets/netclass_selector.cpp
    ${CMAKE_SOURCE_DIR}/common/widgets/net_selector.cpp
    ${CMAKE_SOURCE_DIR}/common/widgets/rc_tree_model.cpp
    ${CMAKE_SOURCE_DIR}/common/widgets/widget_save_restore.cpp
    ${CMAKE_SOURCE_DIR}/common/widgets/wx_html_report_panel.cpp
    dialogs/dialog_export_step_process_qt.cpp
    dialogs/dialog_move_exact.cpp
    pcb_base_edit_frame_qt.cpp
    footprint_edit_frame_qt.cpp
    footprint_tree_pane_qt.cpp
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

# The 3D mice through libspnav (Linux), as in kicommon and common
if( UNIX AND NOT APPLE )
    target_sources( pcbnew_gui_qt PRIVATE
        ${CMAKE_SOURCE_DIR}/common/spacenav/libspnav_driver.cpp
        ${CMAKE_SOURCE_DIR}/common/spacenav/spnav_2d_plugin.cpp
        )
    target_link_libraries( pcbnew_gui_qt PRIVATE ${SPNAV_LIBRARIES} )
endif()

set_target_properties( pcbnew_gui_qt PROPERTIES POSITION_INDEPENDENT_CODE ON )

# The code of KiCad includes Python.h, whose structures have members named as the keywords of Qt
target_compile_definitions( pcbnew_gui_qt PRIVATE QT_NO_KEYWORDS )
kiqt_gui_target( pcbnew_gui_qt )

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
    ${CMAKE_SOURCE_DIR}/thirdparty/dxflib_qcad
    ${CMAKE_SOURCE_DIR}/3d-viewer
    )
target_compile_definitions( pcbnew_gui_qt PRIVATE
    $<TARGET_PROPERTY:pcbnew_kiface_objects,COMPILE_DEFINITIONS>
    )

target_link_libraries( pcbnew_gui_qt
    PRIVATE
        kiqt_common
        dxflib_qcad
        markdown_lib
        nanosvg
        tinyspline_lib
        nlohmann_json
        Boost::headers
    )
