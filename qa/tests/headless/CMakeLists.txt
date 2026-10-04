#
#  This program source code file is part of KICAD, a free EDA CAD application.
#
#  Copyright The KiCad Developers, see AUTHORS.txt for contributors.
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, you may find one here:
#  http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
#  or you may search the http://www.gnu.org website for the version 2 license,
#  or you may write to the Free Software Foundation, Inc.,
#  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
#

# Programs that use the data model of pcbnew and eeschema directly, linked like the headless
# kifaces of kicad-cli: no GUI toolkit (wxBase only), --no-undefined.

if( NOT TARGET pcbnew_cli_kiface OR NOT TARGET eeschema_cli_kiface )
    return()
endif()

add_headless_executable( qa_headless_pcbnew
    KIFACE  pcbnew_cli_kiface
    SOURCES headless_pcbnew.cpp )

add_headless_executable( qa_headless_eeschema
    KIFACE  eeschema_cli_kiface
    SOURCES headless_eeschema.cpp )

# The tests work on copies: loading a project may write its local settings.
set( HEADLESS_DATA ${CMAKE_CURRENT_BINARY_DIR}/data )
file( COPY ${CMAKE_SOURCE_DIR}/qa/data/pcbnew/api_kitchen_sink.kicad_pcb
           ${CMAKE_SOURCE_DIR}/qa/data/pcbnew/tuning_generators_load_save.kicad_pcb
           ${CMAKE_SOURCE_DIR}/qa/data/eeschema/spice_netlists/legacy_sources
      DESTINATION ${HEADLESS_DATA} )

# Expected violations: the counts of kicad-cli (pcb drc --refill-zones, sch erc).
add_test( NAME qa_headless_pcbnew_kitchen_sink
    COMMAND qa_headless_pcbnew ${HEADLESS_DATA}/api_kitchen_sink.kicad_pcb
            ${CMAKE_CURRENT_BINARY_DIR}/out/kitchen_sink 8 )

add_test( NAME qa_headless_pcbnew_tuning_patterns
    COMMAND qa_headless_pcbnew ${HEADLESS_DATA}/tuning_generators_load_save.kicad_pcb
            ${CMAKE_CURRENT_BINARY_DIR}/out/tuning_patterns 3 )

add_test( NAME qa_headless_eeschema_legacy_sources
    COMMAND qa_headless_eeschema ${HEADLESS_DATA}/legacy_sources/legacy_sources.kicad_sch
            ${CMAKE_CURRENT_BINARY_DIR}/out/legacy_sources 11 )

set_tests_properties( qa_headless_pcbnew_kitchen_sink qa_headless_pcbnew_tuning_patterns
                      qa_headless_eeschema_legacy_sources
    PROPERTIES ENVIRONMENT "KICAD_RUN_FROM_BUILD_DIR=1" )
