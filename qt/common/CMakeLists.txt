# libkiqt: the Qt widgets shared by kicad-qt and the Qt KIFACEs

add_library( kiqt SHARED
    gal_canvas.cpp
    property_panel.cpp
    qt_cairo_gal.cpp
    qt_document.h
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
