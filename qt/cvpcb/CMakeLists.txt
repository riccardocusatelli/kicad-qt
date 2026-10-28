# The GUI of cvpcb on Qt (the footprint assignment tool and its footprint viewer, their tools and
# dialogs): the sources of the cvpcb KIFACE of the wxWidgets build, built on the wxWidgets
# compatibility layer (qt/wxcompat).
set( KIQT_CVPCB_GUI_SRCS
    auto_associate.cpp
    cvpcb_mainframe.cpp
    dialogs/dialog_config_equfiles.cpp
    dialogs/fp_conflict_assignment_selector.cpp
    display_footprints_frame.cpp
    footprints_listbox.cpp
    library_listbox.cpp
    listbox_base.cpp
    menubar.cpp
    readwrite_dlgs.cpp
    symbols_listbox.cpp
    toolbars_cvpcb.cpp
    toolbars_display_footprints.cpp
    tools/cvpcb_actions.cpp
    tools/cvpcb_association_tool.cpp
    tools/cvpcb_control.cpp
    tools/cvpcb_fpviewer_selection_tool.cpp
    )

list( TRANSFORM KIQT_CVPCB_GUI_SRCS PREPEND ${CMAKE_SOURCE_DIR}/cvpcb/ )

kiqt_generate_fbp_compat( KIQT_CVPCB_COMPAT_FBP_SRCS
    ${CMAKE_SOURCE_DIR}/cvpcb/dialogs/dialog_config_equfiles_base.fbp
    ${CMAKE_SOURCE_DIR}/cvpcb/dialogs/fp_conflict_assignment_selector_base.fbp
    )

add_library( cvpcb_gui_qt STATIC
    ${KIQT_CVPCB_GUI_SRCS}
    ${KIQT_CVPCB_COMPAT_FBP_SRCS}
    ${CMAKE_SOURCE_DIR}/common/footprint_filter.cpp
    ${CMAKE_SOURCE_DIR}/common/trace_helpers_gui.cpp
    )

set_target_properties( cvpcb_gui_qt PROPERTIES POSITION_INDEPENDENT_CODE ON )

# The code of KiCad includes Python.h, whose structures have members named as the keywords of Qt
target_compile_definitions( cvpcb_gui_qt PRIVATE QT_NO_KEYWORDS )
kiqt_gui_target( cvpcb_gui_qt )

# Compiled as the cvpcb KIFACE (its include directories and definitions), the Qt headers (and the
# bases of the dialogs generated from the wxFormBuilder files) first
target_include_directories( cvpcb_gui_qt BEFORE PRIVATE
    ${CMAKE_SOURCE_DIR}/qt/pcbnew
    ${CMAKE_SOURCE_DIR}/qt/3d-viewer
    ${KIQT_FBP_DIR}/cvpcb/dialogs
    ${KIQT_FBP_DIR}/pcbnew
    ${KIQT_FBP_DIR}/pcbnew/dialogs
    ${KIQT_FBP_DIR}/3d-viewer
    )
target_include_directories( cvpcb_gui_qt PRIVATE
    ${CMAKE_SOURCE_DIR}/cvpcb
    $<TARGET_PROPERTY:cvpcb_kiface,INCLUDE_DIRECTORIES>
    )
target_compile_definitions( cvpcb_gui_qt PRIVATE
    $<TARGET_PROPERTY:cvpcb_kiface,COMPILE_DEFINITIONS>
    )

target_link_libraries( cvpcb_gui_qt
    PRIVATE
        kiqt_common
        nlohmann_json
        Boost::headers
    )


# _cvpcb_qtgui.kiface: the KIFACE of cvpcb with its GUI on Qt, as _cvpcb.kiface of the wxWidgets
# build: the IFACE of cvpcb.cpp and the objects of cvpcb_gui_qt, with the archives of the GUI of
# pcbnew (its base frame, canvas, actions and 3D viewer) and of the headless pcbnew KIFACE.
add_headless_kiface( cvpcb_qtgui_kiface
    OUTPUT_NAME cvpcb_qtgui
    LIKE        pcbnew_cli_kiface
    SOURCES     ${CMAKE_SOURCE_DIR}/cvpcb/cvpcb.cpp
                ${KIQT_KIFACE_SOURCES}
    USAGE       kiqt_common
    ARCHIVES    pcbnew_gui_qt 3d-viewer_qt kiqt_common
    LIBRARIES   kiqt_common kiqt Qt6::Widgets dxflib_qcad markdown_lib nanosvg tinyspline_lib
    )

target_compile_definitions( cvpcb_qtgui_kiface_iface PRIVATE QT_NO_KEYWORDS )
target_include_directories( cvpcb_qtgui_kiface_iface BEFORE PRIVATE
    $<TARGET_PROPERTY:cvpcb_gui_qt,INCLUDE_DIRECTORIES>
    )
target_compile_definitions( cvpcb_qtgui_kiface_iface PRIVATE
    $<TARGET_PROPERTY:cvpcb_gui_qt,COMPILE_DEFINITIONS>
    )

# All the objects of the GUI: the tools and the actions register themselves when they are built
target_sources( cvpcb_qtgui_kiface PRIVATE $<TARGET_OBJECTS:cvpcb_gui_qt> )
add_dependencies( cvpcb_qtgui_kiface cvpcb_gui_qt pcbnew_gui_qt )

if( UNIX AND NOT APPLE )
    target_link_libraries( cvpcb_qtgui_kiface PRIVATE ${SPNAV_LIBRARIES} )
endif()

set_target_properties( cvpcb_qtgui_kiface PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/cvpcb
    )

install( TARGETS cvpcb_qtgui_kiface
    DESTINATION ${KICAD_KIFACE}
    COMPONENT binary
    )
