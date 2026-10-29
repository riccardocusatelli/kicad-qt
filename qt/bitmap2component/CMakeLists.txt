# The image converter (bitmap2component) with its GUI on Qt: its sources compiled on the
# wxWidgets compatibility layer, as a KIFACE of the Qt shell.

set( KIQT_BITMAP2COMPONENT_GUI_SRCS
    bitmap2cmp_control.cpp
    bitmap2cmp_settings.cpp
    bitmap2component.cpp
    bitmap2cmp_frame.cpp
    bitmap2cmp_panel.cpp
    )

list( TRANSFORM KIQT_BITMAP2COMPONENT_GUI_SRCS PREPEND ${CMAKE_SOURCE_DIR}/bitmap2component/ )

kiqt_generate_fbp_compat( KIQT_BITMAP2COMPONENT_COMPAT_FBP_SRCS
    ${CMAKE_SOURCE_DIR}/bitmap2component/bitmap2cmp_panel_base.fbp
    )

add_library( bitmap2component_gui_qt OBJECT
    ${KIQT_BITMAP2COMPONENT_GUI_SRCS}
    ${KIQT_BITMAP2COMPONENT_COMPAT_FBP_SRCS}
    )

set_target_properties( bitmap2component_gui_qt PROPERTIES POSITION_INDEPENDENT_CODE ON )

target_compile_definitions( bitmap2component_gui_qt PRIVATE QT_NO_KEYWORDS BITMAP_2_CMP )
kiqt_gui_target( bitmap2component_gui_qt )

# Compiled as bitmap2component (its include directories), the base of the panel generated
# from the wxFormBuilder file first
target_include_directories( bitmap2component_gui_qt BEFORE PRIVATE
    ${KIQT_FBP_DIR}/bitmap2component
    )
target_include_directories( bitmap2component_gui_qt PRIVATE
    ${CMAKE_SOURCE_DIR}/bitmap2component
    )

target_link_libraries( bitmap2component_gui_qt PRIVATE kiqt_common potrace nlohmann_json
                       Boost::headers )


# _bitmap2component_qtgui.kiface: the KIFACE of the image converter with its GUI on Qt (the
# wxWidgets build links it in the bitmap2component program).
add_headless_kiface( bitmap2component_qtgui_kiface
    OUTPUT_NAME bitmap2component_qtgui
    SOURCES     ${CMAKE_SOURCE_DIR}/bitmap2component/bitmap2cmp_main.cpp
                ${KIQT_KIFACE_SOURCES}
    OBJECTS     bitmap2component_gui_qt
    USAGE       kiqt_common
    ARCHIVES    common kiqt_common
    LIBRARIES   kiqt_common kiqt Qt6::Widgets kicommon_model gal_model core Boost::headers
                potrace ${wxWidgets_BASE_LIBRARIES}
    )

target_compile_definitions( bitmap2component_qtgui_kiface_iface PRIVATE QT_NO_KEYWORDS BITMAP_2_CMP )
target_include_directories( bitmap2component_qtgui_kiface_iface BEFORE PRIVATE
    $<TARGET_PROPERTY:bitmap2component_gui_qt,INCLUDE_DIRECTORIES>
    )

# Where KIWAY_BASE finds the KIFACE when run from the build tree
set_target_properties( bitmap2component_qtgui_kiface PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bitmap2component
    )

install( TARGETS bitmap2component_qtgui_kiface
    DESTINATION ${KICAD_KIFACE}
    COMPONENT binary
    )
