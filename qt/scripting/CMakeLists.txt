# _kipython_qtgui.kiface: the KIFACE of the Python console (FRAME_PYTHON) with its GUI on Qt, as
# _kipython.kiface of the wxWidgets build: its IFACE (kicad_scripting_main.cpp) and the console
# window of the Qt build (the one of the wxWidgets build is the shell of wxPython).
add_library( kipython_gui_qt OBJECT
    kipython_frame_qt.cpp
    ${CMAKE_SOURCE_DIR}/scripting/kipython_settings.cpp
    )

set_target_properties( kipython_gui_qt PROPERTIES POSITION_INDEPENDENT_CODE ON )

# Python.h has structures with members named as the keywords of Qt
target_compile_definitions( kipython_gui_qt PRIVATE QT_NO_KEYWORDS )
kiqt_gui_target( kipython_gui_qt )

# The header of the console of Qt first
target_include_directories( kipython_gui_qt BEFORE PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} )
target_include_directories( kipython_gui_qt PRIVATE
    ${CMAKE_SOURCE_DIR}/scripting
    ${CMAKE_SOURCE_DIR}/thirdparty/pybind11/include
    ${PYTHON_INCLUDE_DIRS}
    )

target_link_libraries( kipython_gui_qt PRIVATE kiqt_common nlohmann_json Boost::headers )

add_headless_kiface( kipython_qtgui_kiface
    OUTPUT_NAME kipython_qtgui
    SOURCES     ${CMAKE_SOURCE_DIR}/scripting/kicad_scripting_main.cpp
                ${KIQT_KIFACE_SOURCES}
    OBJECTS     kipython_gui_qt
    USAGE       kiqt_common
    ARCHIVES    common kiqt_common
    LIBRARIES   kiqt_common kiqt Qt6::Widgets kicommon_model gal_model core Boost::headers
                ${wxWidgets_BASE_LIBRARIES} ${PYTHON_LIBRARIES}
    )

target_compile_definitions( kipython_qtgui_kiface_iface PRIVATE QT_NO_KEYWORDS )
target_include_directories( kipython_qtgui_kiface_iface BEFORE PRIVATE
    $<TARGET_PROPERTY:kipython_gui_qt,INCLUDE_DIRECTORIES>
    )

# Where KIWAY_BASE finds the KIFACE of the console when run from the build tree
set_target_properties( kipython_qtgui_kiface PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/scripting
    )

install( TARGETS kipython_qtgui_kiface
    DESTINATION ${KICAD_KIFACE}
    COMPONENT binary
    )
