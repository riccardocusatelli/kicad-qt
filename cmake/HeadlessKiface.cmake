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

# add_headless_kiface( <target>
#     OUTPUT_NAME <name>              # builds _<name>.kiface
#     SOURCES <files...>              # the KIFACE without GUI (KIFACE_GETTER, Kiface()), and
#                                     # the headless definitions of editor functions
#     OBJECTS <object library>        # the objects of the editor KIFACE
#     USAGE <targets...>              # targets whose usage requirements SOURCE needs
#     ARCHIVES <static libraries...>  # linked by file, without their link dependencies
#     LIBRARIES <libraries...> )      # linked as usual; they must not bring the GUI toolkit
#
# Builds the KIFACE that kicad-cli loads.  The objects of the editor and the archives are
# linked as static archives, so only the objects that the KIFACE needs are pulled in, and the
# link uses --no-undefined against wxBase: an object that needs the GUI toolkit, or GUI code
# of KiCad (kicommon), breaks the link.
#
# An object that is only reached through a static constructor (a self-registering DRC test
# provider, for instance) is not pulled in: something must refer to it (see
# DRC_TEST_PROVIDER_REGISTRY::LinkTestProviders()).
function( add_headless_kiface target )
    cmake_parse_arguments( ARG "" "OUTPUT_NAME;OBJECTS" "SOURCES;USAGE;ARCHIVES;LIBRARIES" ${ARGN} )

    add_library( ${target}_iface OBJECT ${ARG_SOURCES} )
    target_link_libraries( ${target}_iface PRIVATE ${ARG_USAGE} )
    target_compile_definitions( ${target}_iface PRIVATE BUILD_KIWAY_DLL COMPILING_DLL )

    add_library( ${target}_archive STATIC $<TARGET_OBJECTS:${ARG_OBJECTS}> )

    add_library( ${target} MODULE $<TARGET_OBJECTS:${target}_iface> )

    set_target_properties( ${target} PROPERTIES
        OUTPUT_NAME     ${ARG_OUTPUT_NAME}
        PREFIX          ${KIFACE_PREFIX}
        SUFFIX          ${KIFACE_SUFFIX}
        LINKER_LANGUAGE CXX
        )

    set( _archives $<TARGET_FILE:${target}_archive> )

    foreach( _lib ${ARG_ARCHIVES} )
        list( APPEND _archives $<TARGET_FILE:${_lib}> )
        add_dependencies( ${target} ${_lib} )
    endforeach()

    if( CMAKE_SYSTEM_NAME STREQUAL "Linux" )
        # The archives depend on each other: let the linker go through them until all the
        # symbols are resolved.
        target_link_libraries( ${target} PRIVATE
            "-Wl,--start-group" ${_archives} "-Wl,--end-group"
            ${ARG_LIBRARIES} )

        target_link_options( ${target} PRIVATE "LINKER:--no-undefined" )
    else()
        target_link_libraries( ${target} PRIVATE ${_archives} ${_archives} ${ARG_LIBRARIES} )
    endif()
endfunction()
