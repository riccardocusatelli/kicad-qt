# kicad-qt: the Qt shell (project, documents in tabs), with the Qt KIFACEs

add_executable( kicad-qt
    home_widgets.cpp
    kicad_qt.cpp
    main_window.cpp
    )

set_target_properties( kicad-qt PROPERTIES AUTOMOC ON )

# The program classes (BIN_MOD) are in the common archive, linked by file as for kicad-cli
target_include_directories( kicad-qt PRIVATE
    $<TARGET_PROPERTY:common,INTERFACE_INCLUDE_DIRECTORIES>
    )

target_link_libraries( kicad-qt
    kiqt
    "$<TARGET_FILE:common>"
    kicommon_model
    core
    Qt6::Widgets
    ${wxWidgets_BASE_LIBRARIES}
    )

add_dependencies( kicad-qt common pcbnew_qt_kiface eeschema_qt_kiface )

if( CMAKE_SYSTEM_NAME STREQUAL "Linux" )
    target_link_options( kicad-qt PRIVATE "LINKER:--no-undefined" )
endif()

# In the kicad directory of the build tree, as the other programs (KICAD_RUN_FROM_BUILD_DIR)
set_target_properties( kicad-qt PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/kicad
    )

install( TARGETS kicad-qt
    RUNTIME DESTINATION ${KICAD_BIN}
    COMPONENT binary
    )
