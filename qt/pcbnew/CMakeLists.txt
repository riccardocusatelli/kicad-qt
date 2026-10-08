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
