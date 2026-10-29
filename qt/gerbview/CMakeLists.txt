# The GUI of the Gerber viewer on Qt: the sources of the gerbview KIFACE of the wxWidgets build,
# built on the wxWidgets compatibility layer (qt/wxcompat).
set( KIQT_GERBVIEW_GUI_SRCS
    dialogs/panel_gerbview_color_settings.cpp
    dialogs/panel_gerbview_display_options.cpp
    dialogs/panel_gerbview_excellon_settings.cpp
    dialogs/dialog_draw_layers_settings.cpp
    dialogs/dialog_map_gerber_layers_to_pcb.cpp
    dialogs/dialog_print_gerbview.cpp
    dialogs/dialog_select_one_pcb_layer.cpp
    widgets/dcode_selection_box.cpp
    widgets/gbr_layer_box_selector.cpp
    widgets/gerbview_layer_widget.cpp
    widgets/layer_widget.cpp
    am_param.cpp
    am_primitive.cpp
    aperture_macro.cpp
    gbr_layout.cpp
    gerber_file_image.cpp
    gerber_file_image_list.cpp
    gerber_draw_item.cpp
    gerbview_printout.cpp
    X2_gerber_attributes.cpp
    clear_gbr_drawlayers.cpp
    dcode.cpp
    evaluate.cpp
    events_called_functions.cpp
    excellon_read_drill_file.cpp
    export_to_pcbnew.cpp
    files.cpp
    gerbview_settings.cpp
    gerbview_frame.cpp
    gerbview_frame_load.cpp
    job_file_reader.cpp
    menubar.cpp
    readgerb.cpp
    rs274_read_XY_and_IJ_coordinates.cpp
    rs274d.cpp
    rs274x.cpp
    toolbars_gerber.cpp
    gerbview_draw_panel_gal.cpp
    gerbview_painter.cpp
    tools/gerbview_actions.cpp
    tools/gerbview_inspection_tool.cpp
    tools/gerbview_selection.cpp
    tools/gerbview_selection_tool.cpp
    tools/gerbview_control.cpp
    gerber_collectors.cpp
    )

list( TRANSFORM KIQT_GERBVIEW_GUI_SRCS PREPEND ${CMAKE_SOURCE_DIR}/gerbview/ )

kiqt_generate_fbp_compat( KIQT_GERBVIEW_COMPAT_FBP_SRCS
    ${CMAKE_SOURCE_DIR}/gerbview/dialogs/dialog_draw_layers_settings_base.fbp
    ${CMAKE_SOURCE_DIR}/gerbview/dialogs/dialog_map_gerber_layers_to_pcb_base.fbp
    ${CMAKE_SOURCE_DIR}/gerbview/dialogs/panel_gerbview_display_options_base.fbp
    ${CMAKE_SOURCE_DIR}/gerbview/dialogs/panel_gerbview_excellon_settings_base.fbp
    )

add_library( gerbview_gui_qt OBJECT
    ${KIQT_GERBVIEW_GUI_SRCS}
    ${CMAKE_SOURCE_DIR}/common/board_printout.cpp
    ${CMAKE_SOURCE_DIR}/common/widgets/wx_ellipsized_static_text.cpp
    ${KIQT_GERBVIEW_COMPAT_FBP_SRCS}
    )

set_target_properties( gerbview_gui_qt PROPERTIES POSITION_INDEPENDENT_CODE ON )

# The printout of the boards (common in the wxWidgets build) reads the plot settings of pcbnew
set_source_files_properties( ${CMAKE_SOURCE_DIR}/common/board_printout.cpp PROPERTIES
    INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/pcbnew
    )

target_compile_definitions( gerbview_gui_qt PRIVATE QT_NO_KEYWORDS GERBVIEW )
kiqt_gui_target( gerbview_gui_qt )

# Compiled as the gerbview KIFACE (its include directories), the bases of the dialogs generated
# from the wxFormBuilder files first
target_include_directories( gerbview_gui_qt BEFORE PRIVATE
    ${KIQT_FBP_DIR}/gerbview/dialogs
    ${KIQT_FBP_DIR}/gerbview
    )
target_include_directories( gerbview_gui_qt PRIVATE
    ${CMAKE_SOURCE_DIR}/gerbview
    ${CMAKE_SOURCE_DIR}/gerbview/dialogs
    ${CMAKE_SOURCE_DIR}/common/dialogs
    ${CMAKE_SOURCE_DIR}/3d-viewer
    )

target_link_libraries( gerbview_gui_qt PRIVATE kiqt_common nlohmann_json Boost::headers )


# _gerbview_qtgui.kiface: the KIFACE of the Gerber viewer with its GUI on Qt, as
# _gerbview.kiface of the wxWidgets build.
add_headless_kiface( gerbview_qtgui_kiface
    OUTPUT_NAME gerbview_qtgui
    SOURCES     ${CMAKE_SOURCE_DIR}/gerbview/gerbview.cpp
                ${KIQT_KIFACE_SOURCES}
    OBJECTS     gerbview_gui_qt
    USAGE       kiqt_common
    ARCHIVES    common kiqt_common
    LIBRARIES   kiqt_common kiqt Qt6::Widgets kicommon_model gal_model core Boost::headers
                ${wxWidgets_BASE_LIBRARIES}
    )

target_compile_definitions( gerbview_qtgui_kiface_iface PRIVATE QT_NO_KEYWORDS GERBVIEW )
target_include_directories( gerbview_qtgui_kiface_iface BEFORE PRIVATE
    $<TARGET_PROPERTY:gerbview_gui_qt,INCLUDE_DIRECTORIES>
    )

# Where KIWAY_BASE finds the KIFACE when run from the build tree
set_target_properties( gerbview_qtgui_kiface PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/gerbview
    )

install( TARGETS gerbview_qtgui_kiface
    DESTINATION ${KICAD_KIFACE}
    COMPONENT binary
    )
