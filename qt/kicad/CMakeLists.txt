# kicad-qt: the project manager of KiCad (KICAD_MANAGER_FRAME, on the wxWidgets compatibility
# layer), with the Qt KIFACEs: the editors of KiCad (_eeschema_qtgui.kiface,
# _pcbnew_qtgui.kiface...) run in it


include( ${CMAKE_CURRENT_SOURCE_DIR}/manager.cmake )

add_executable( kicad-qt
    kicad_qt.cpp
    $<TARGET_OBJECTS:kicad_manager_qt>
    )

target_compile_definitions( kicad-qt PRIVATE QT_NO_KEYWORDS KICAD PCM )
kiqt_gui_target( kicad-qt )

# The program classes (BIN_MOD) are in the common archive, linked by file as for kicad-cli
target_include_directories( kicad-qt PRIVATE ${KIQT_MANAGER_INCLUDE_DIRS} )

# The program with the user interface of KiCad (PGM_GUI, KIWAY, the start wizard): kiqt_common,
# as in each KIFACE
target_link_libraries( kicad-qt
    kiqt_common
    kiqt
    "$<TARGET_FILE:common>"
    kicommon_model
    core
    Qt6::Widgets
    nlohmann_json
    Boost::headers
    pads_common
    ${wxWidgets_BASE_LIBRARIES}
    )

add_dependencies( kicad-qt kicad_manager_qt common pcbnew_qtgui_kiface eeschema_qtgui_kiface cvpcb_qtgui_kiface
                  kipython_qtgui_kiface pl_editor_qtgui_kiface gerbview_qtgui_kiface pcb_calculator_qtgui_kiface
                  bitmap2component_qtgui_kiface )

# The JSON schemas of the API and of the plugin and content manager, in the schemas directory of
# the build tree (KICAD_RUN_FROM_BUILD_DIR): their copy targets are not in the wx-free default
foreach( _schema_copy api_schema_build_copy schema_build_copy )
    if( TARGET ${_schema_copy} )
        add_dependencies( kicad-qt ${_schema_copy} )
    endif()
endforeach()

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
