# The GUI of the drawing sheet editor on Qt: the sources of the pl_editor KIFACE of the wxWidgets
# build, built on the wxWidgets compatibility layer (qt/wxcompat).
set( KIQT_PL_EDITOR_GUI_SRCS
    dialogs/properties_frame.cpp
    dialogs/dialogs_for_printing.cpp
    dialogs/design_inspector.cpp
    dialogs/panel_pl_editor_color_settings.cpp
    dialogs/panel_pl_editor_display_options.cpp
    pl_editor_layout.cpp
    files.cpp
    pl_editor_frame.cpp
    pl_editor_settings.cpp
    pl_draw_panel_gal.cpp
    pl_editor_undo_redo.cpp
    menubar.cpp
    toolbars_pl_editor.cpp
    tools/pl_actions.cpp
    tools/pl_selection.cpp
    tools/pl_selection_tool.cpp
    tools/pl_drawing_tools.cpp
    tools/pl_edit_tool.cpp
    tools/pl_editor_control.cpp
    tools/pl_point_editor.cpp
    )

list( TRANSFORM KIQT_PL_EDITOR_GUI_SRCS PREPEND ${CMAKE_SOURCE_DIR}/pagelayout_editor/ )

kiqt_generate_fbp_compat( KIQT_PL_EDITOR_COMPAT_FBP_SRCS
    ${CMAKE_SOURCE_DIR}/pagelayout_editor/dialogs/dialog_design_inspector_base.fbp
    ${CMAKE_SOURCE_DIR}/pagelayout_editor/dialogs/panel_pl_editor_color_settings_base.fbp
    ${CMAKE_SOURCE_DIR}/pagelayout_editor/dialogs/properties_frame_base.fbp
    )

add_library( pl_editor_gui_qt OBJECT
    pl_editor_frame_qt.cpp
    ${KIQT_PL_EDITOR_GUI_SRCS}
    ${KIQT_PL_EDITOR_COMPAT_FBP_SRCS}
    )

set_target_properties( pl_editor_gui_qt PROPERTIES POSITION_INDEPENDENT_CODE ON )

target_compile_definitions( pl_editor_gui_qt PRIVATE QT_NO_KEYWORDS PL_EDITOR )
kiqt_gui_target( pl_editor_gui_qt )

# Compiled as the pl_editor KIFACE (its include directories), the bases of the dialogs generated
# from the wxFormBuilder files first
target_include_directories( pl_editor_gui_qt BEFORE PRIVATE
    ${KIQT_FBP_DIR}/pagelayout_editor/dialogs
    )
target_include_directories( pl_editor_gui_qt PRIVATE
    ${CMAKE_SOURCE_DIR}/pagelayout_editor
    ${CMAKE_SOURCE_DIR}/pagelayout_editor/dialogs
    ${CMAKE_SOURCE_DIR}/common/dialogs
    )

target_link_libraries( pl_editor_gui_qt PRIVATE kiqt_common nlohmann_json Boost::headers )


# _pl_editor_qtgui.kiface: the KIFACE of the drawing sheet editor with its GUI on Qt, as
# _pl_editor.kiface of the wxWidgets build.
add_headless_kiface( pl_editor_qtgui_kiface
    OUTPUT_NAME pl_editor_qtgui
    SOURCES     ${CMAKE_SOURCE_DIR}/pagelayout_editor/pl_editor.cpp
                ${KIQT_KIFACE_SOURCES}
    OBJECTS     pl_editor_gui_qt
    USAGE       kiqt_common
    ARCHIVES    common kiqt_common
    LIBRARIES   kiqt_common kiqt Qt6::Widgets kicommon_model gal_model core Boost::headers
                ${wxWidgets_BASE_LIBRARIES}
    )

target_compile_definitions( pl_editor_qtgui_kiface_iface PRIVATE QT_NO_KEYWORDS PL_EDITOR )
target_include_directories( pl_editor_qtgui_kiface_iface BEFORE PRIVATE
    $<TARGET_PROPERTY:pl_editor_gui_qt,INCLUDE_DIRECTORIES>
    )

# Where KIWAY_BASE finds the KIFACE when run from the build tree
set_target_properties( pl_editor_qtgui_kiface PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/pagelayout_editor
    )

install( TARGETS pl_editor_qtgui_kiface
    DESTINATION ${KICAD_KIFACE}
    COMPONENT binary
    )
