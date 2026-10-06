# _eeschema_qt.kiface: the KIFACE of eeschema without GUI (as _eeschema_cli.kiface), with the
# schematic documents of the Qt shell

add_headless_kiface( eeschema_qt_kiface
    OUTPUT_NAME eeschema_qt
    LIKE        eeschema_cli_kiface
    SOURCES     ${CMAKE_SOURCE_DIR}/eeschema/eeschema_cli.cpp
                schematic_document.cpp
                ${KIQT_KIFACE_SOURCES}
    USAGE       kiqt
    LIBRARIES   kiqt Qt6::Widgets
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
