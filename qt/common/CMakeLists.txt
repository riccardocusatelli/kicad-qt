# libkiqt: the Qt widgets shared by kicad-qt and the Qt KIFACEs, and the GUI services of the Qt
# build that are one for the program (the icons, the cursors, the message dialogs, the event
# loop and the keyboard)

add_library( kiqt SHARED
    gal_canvas.cpp
    property_panel.cpp
    qt_cairo_gal.cpp
    qt_document.cpp
    ui_hooks.cpp

    bitmap_store.cpp
    confirm.cpp
    gal/cursors.cpp
    toolkit/file_watcher_qt.cpp
    toolkit/gui_services_qt.cpp
    toolkit/gui_timer_qt.cpp
    toolkit/qt_keys.cpp
    widgets/msgpanel.cpp
    widgets/wx_infobar.cpp
    ${CMAKE_SOURCE_DIR}/qt/libs/kiplatform/ui.cpp
    )

set_target_properties( kiqt PROPERTIES
    AUTOMOC ON
    OUTPUT_NAME kiqt
    )

target_include_directories( kiqt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )

target_link_libraries( kiqt
    PUBLIC
        Qt6::Widgets
        gal_model
        kicommon_model
        kiplatform_core
        kimath
        core
        ${wxWidgets_BASE_LIBRARIES}
    )

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
    ${CMAKE_SOURCE_DIR}/common/view/view_controls.cpp
    ${CMAKE_SOURCE_DIR}/common/view/zoom_controller.cpp
    ${CMAKE_SOURCE_DIR}/common/widgets/msgpanel_item.cpp

    dialog_shim.cpp
    dialogs/dialog_unit_entry.cpp
    draw_panel_gal.cpp
    eda_base_frame.cpp
    eda_draw_frame.cpp
    kiway.cpp
    kiway_player.cpp
    tool/action_menu.cpp
    tool/action_toolbar.cpp
    tool/tool_dispatcher.cpp
    view/qt_view_controls.cpp
    widgets/unit_binder.cpp
    )

set_target_properties( kiqt_common PROPERTIES POSITION_INDEPENDENT_CODE ON )

target_include_directories( kiqt_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    $<TARGET_PROPERTY:common,INTERFACE_INCLUDE_DIRECTORIES>
    )

target_link_libraries( kiqt_common
    PUBLIC
        kiqt
        libcontext
    )
