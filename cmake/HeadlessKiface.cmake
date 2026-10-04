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

# Link <target> with the archives and libraries of a headless kiface (see below).
function( headless_link target kiface )
    get_target_property( _archive_targets ${kiface} HEADLESS_ARCHIVE_TARGETS )
    get_target_property( _libraries ${kiface} HEADLESS_LIBRARIES )

    set( _archives "" )

    foreach( _lib ${_archive_targets} )
        list( APPEND _archives $<TARGET_FILE:${_lib}> )
        add_dependencies( ${target} ${_lib} )
    endforeach()

    if( CMAKE_SYSTEM_NAME STREQUAL "Linux" )
        # The archives depend on each other: let the linker go through them until all the
        # symbols are resolved.
        target_link_libraries( ${target} PRIVATE
            "-Wl,--start-group" ${_archives} "-Wl,--end-group"
            ${_libraries} )

        target_link_options( ${target} PRIVATE "LINKER:--no-undefined" )
    else()
        target_link_libraries( ${target} PRIVATE ${_archives} ${_archives} ${_libraries} )
    endif()
endfunction()


# add_headless_kiface( <target>
#     OUTPUT_NAME <name>              # builds _<name>.kiface
#     SOURCES <files...>              # the KIFACE without GUI (KIFACE_GETTER, Kiface()), and
#                                     # the headless definitions of editor functions
#     OBJECTS <object library>        # the objects of the editor KIFACE
#     USAGE <targets...>              # targets whose usage requirements SOURCES need
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
# LinkDrcTestProviders()).
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

    set_target_properties( ${target} PROPERTIES
        HEADLESS_ARCHIVE_TARGETS "${target}_archive;${ARG_ARCHIVES}"
        HEADLESS_LIBRARIES       "${ARG_LIBRARIES}"
        HEADLESS_USAGE           "${ARG_USAGE}"
        )

    headless_link( ${target} ${target} )
endfunction()


# add_headless_executable( <target>
#     KIFACE <headless kiface target>
#     SOURCES <files...> )
#
# Builds a program that uses the data model of a kiface directly, without loading it: it is
# linked with the KIFACE source (Kiface()) and the same archives and libraries as the headless
# kiface, so it is checked the same way (no GUI toolkit, --no-undefined).
function( add_headless_executable target )
    cmake_parse_arguments( ARG "" "KIFACE" "SOURCES" ${ARGN} )

    get_target_property( _usage ${ARG_KIFACE} HEADLESS_USAGE )

    # Compiled like the KIFACE source: same usage requirements, include directories and
    # definitions (the editors set some of them for their whole directory).
    add_library( ${target}_objects OBJECT ${ARG_SOURCES} )
    target_link_libraries( ${target}_objects PRIVATE ${_usage} )
    target_include_directories( ${target}_objects PRIVATE
        $<TARGET_PROPERTY:${ARG_KIFACE}_iface,INCLUDE_DIRECTORIES> )
    target_compile_definitions( ${target}_objects PRIVATE
        $<TARGET_PROPERTY:${ARG_KIFACE}_iface,COMPILE_DEFINITIONS> )

    add_executable( ${target}
        $<TARGET_OBJECTS:${target}_objects>
        $<TARGET_OBJECTS:${ARG_KIFACE}_iface> )

    set_target_properties( ${target} PROPERTIES LINKER_LANGUAGE CXX )

    headless_link( ${target} ${ARG_KIFACE} )
endfunction()
