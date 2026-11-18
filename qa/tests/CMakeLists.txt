# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2014-2019 KiCad Developers, see AUTHORS.txt for contributors.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

## Unit tests
<<<<<<< b2e17e0a
# Without wxWidgets (KICAD_WX_FREE): the library suites, the headless programs and the tests of
# kicad-cli; the other suites use the GUI
if( NOT KICAD_WX_FREE )
    add_subdirectory( api )
    add_subdirectory( common )
    add_subdirectory( gerbview )
    add_subdirectory( eeschema )
    add_subdirectory( pcbnew )
endif()
||||||| 1304b76aad
add_subdirectory( api )
add_subdirectory( common )
add_subdirectory( gerbview )
add_subdirectory( eeschema )
add_subdirectory( libs )
add_subdirectory( pcbnew )
=======
add_subdirectory( api )
add_subdirectory( common )
add_subdirectory( diff_merge )
add_subdirectory( gerbview )
add_subdirectory( eeschema )
add_subdirectory( libs )
add_subdirectory( pcbnew )
>>>>>>> ed8c16703e

add_subdirectory( libs )
add_subdirectory( headless )

if( KICAD_SPICE_QA AND NOT KICAD_WX_FREE )
    add_subdirectory( spice )
endif()

if( KICAD_TEST_XML_OUTPUT )
    # To do this, you will need xmlrunner
    set( PYTEST_ARGS_QAPYTHON --junitxml=${CMAKE_CURRENT_BINARY_DIR}/qapython.junit-results.xml )
    set( PYTEST_ARGS_QACLI --junitxml=${CMAKE_CURRENT_BINARY_DIR}/qacli.junit-results.xml )
endif()

set( EXTRA_PYTEST_ENVIRONMENT "" )

if( APPLE )
    # We need to use the installed kicad-cli not the build dir to run these tests
    get_filename_component( CAIRO_LIB_BASE "${CAIRO_LIBRARY}" DIRECTORY )
    set( EXTRA_PYTEST_ENVIRONMENT "DYLD_FALLBACK_LIBRARY_PATH=${CAIRO_LIB_BASE}" )

elseif ( MSVC )
    set( EXTRA_PYTEST_ENVIRONMENT "FONTCONFIG_PATH=${CMAKE_SOURCE_DIR}/qa/resources/windows/fonts/;PYTHONUTF8=1;PYTHONIOENCODING=utf-8" )
endif()

<<<<<<< b2e17e0a
# qa_python needs the pcbnew Python module, which is not built without wxWidgets
if( NOT KICAD_WX_FREE AND NOT (MSVC AND CMAKE_BUILD_TYPE STREQUAL "Debug") )
# qa_python will never work under MSVC + Debug because we only have the Release mode python.exe
# but the kicad binaries are linked to the debug mode python
# Test that runs the QA tests through scripting
add_test(NAME qa_python
    COMMAND ${PYTHON_EXECUTABLE} -m pytest ${CMAKE_CURRENT_SOURCE_DIR}/pcbnewswig ${PYTEST_ARGS_QAPYTHON}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
||||||| 1304b76aad
if( NOT (MSVC AND CMAKE_BUILD_TYPE STREQUAL "Debug") )
# qa_python will never work under MSVC + Debug because we only have the Release mode python.exe
# but the kicad binaries are linked to the debug mode python
# Test that runs the QA tests through scripting
add_test(NAME qa_python
    COMMAND ${PYTHON_EXECUTABLE} -m pytest ${CMAKE_CURRENT_SOURCE_DIR}/pcbnewswig ${PYTEST_ARGS_QAPYTHON}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
=======
# Use pytest-xdist for parallel CLI test execution when available
execute_process(
    COMMAND ${PYTHON_EXECUTABLE} -c "import xdist"
    RESULT_VARIABLE PYTEST_XDIST_RESULT
    OUTPUT_QUIET
    ERROR_QUIET
>>>>>>> ed8c16703e
)

if( PYTEST_XDIST_RESULT EQUAL 0 )
    set( PYTEST_PARALLEL_ARGS -n auto )
    message( STATUS "pytest-xdist found, qa_cli tests will run in parallel" )
else()
    set( PYTEST_PARALLEL_ARGS "" )
    message( STATUS "pytest-xdist not found, qa_cli tests will run sequentially" )
endif()

add_test(NAME qa_cli
    COMMAND ${PYTHON_EXECUTABLE} -m pytest ${PYTEST_PARALLEL_ARGS}
            ${CMAKE_CURRENT_SOURCE_DIR}/cli ${PYTEST_ARGS_QACLI}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

set_tests_properties( qa_cli
    PROPERTIES
        ENVIRONMENT "KICAD_CLI=$<TARGET_FILE:kicad-cli>;PYTHONPATH=${CMAKE_BINARY_DIR}/pcbnew;${PYTHON_QA_PATH};${EXTRA_PYTEST_ENVIRONMENT}"
)

setup_qa_env( qa_cli )

# Target to install python dependencies for QA via pip
add_custom_target( qa_python_deps
        COMMAND ${PYTHON_EXECUTABLE} -m ensurepip
        COMMAND ${PYTHON_EXECUTABLE} -m pip install -r ${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt )