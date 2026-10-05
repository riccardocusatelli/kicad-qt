#
#  This program source code file is part of KICAD, a free EDA CAD application.
#
#  Copyright The KiCad Developers, see AUTHORS.txt for contributors.
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 3
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

# KICAD_WX_FREE: the core of KiCad and kicad-cli built without wxWidgets.
#
# The wx API of the core is provided by kiwx (libs/kiwx), and only the targets that kicad-cli
# and its kifaces are made of are built, with the sources they link (cmake/WxFreeSources.cmake,
# written by tools/wxfree_sources.py).  The GUI is not built in this configuration.


# Replaces the wxWidgets lookup: the wx variables of the build refer to kiwx.
macro( kicad_wx_free_setup )
    message( STATUS "KICAD_WX_FREE: core and kicad-cli without wxWidgets (kiwx)" )

    add_compile_definitions( KICAD_WX_FREE )

    # The targets and sources of the wx-free build
    include( WxFreeSources )

    # Before any other include directory: the wx headers are the ones of kiwx
    include_directories( BEFORE ${PROJECT_SOURCE_DIR}/libs/kiwx/include )

    set( wxWidgets_FOUND TRUE )
    set( wxWidgets_LIBRARIES kiwx )
    set( wxWidgets_BASE_LIBRARIES kiwx )
    set( wxWidgets_INCLUDE_DIRS "" )
    set( wxWidgets_VERSION_STRING "3.2.4" )
    set( KICAD_WX_PORT "base" )
endmacro()


# The targets of all the directories under aDir.
function( _kicad_wx_free_all_targets aDir aResult )
    get_property( _targets DIRECTORY ${aDir} PROPERTY BUILDSYSTEM_TARGETS )
    get_property( _subdirs DIRECTORY ${aDir} PROPERTY SUBDIRECTORIES )

    foreach( _subdir ${_subdirs} )
        _kicad_wx_free_all_targets( ${_subdir} _sub )
        list( APPEND _targets ${_sub} )
    endforeach()

    set( ${aResult} ${_targets} PARENT_SCOPE )
endfunction()


# What a link item of a kept target becomes: itself, or for a target that is not built, the
# usage requirements it passes on (the kept targets and header-only libraries it links).
function( _kicad_wx_free_link_item aItem aDropped aVisited aResult )
    set( _name ${aItem} )

    if( _name MATCHES "^\\$<LINK_ONLY:([^>]+)>$" )
        set( _name ${CMAKE_MATCH_1} )
    endif()

    if( NOT _name IN_LIST aDropped )
        set( ${aResult} ${aItem} PARENT_SCOPE )
        return()
    endif()

    set( _result "" )

    if( NOT _name IN_LIST aVisited )
        list( APPEND aVisited ${_name} )
        get_target_property( _libs ${_name} INTERFACE_LINK_LIBRARIES )

        foreach( _lib ${_libs} )
            set( _sub_name ${_lib} )

            if( _sub_name MATCHES "^\\$<LINK_ONLY:([^>]+)>$" )
                set( _sub_name ${CMAKE_MATCH_1} )
            endif()

            if( _sub_name IN_LIST aDropped )
                _kicad_wx_free_link_item( "${_lib}" "${aDropped}" "${aVisited}" _sub )
                list( APPEND _result ${_sub} )
            elseif( TARGET ${_sub_name} )
                get_target_property( _type ${_sub_name} TYPE )
                get_target_property( _imported ${_sub_name} IMPORTED )

                # Header-only libraries and the kept targets; the other libraries of the
                # system are the ones of the GUI
                if( _type STREQUAL "INTERFACE_LIBRARY" OR NOT _imported )
                    # Not a header-only library that brings a target which is not built
                    get_target_property( _sub_libs ${_sub_name} INTERFACE_LINK_LIBRARIES )
                    set( _brings_dropped FALSE )

                    foreach( _sub_lib ${_sub_libs} )
                        if( _sub_lib IN_LIST aDropped )
                            set( _brings_dropped TRUE )
                        endif()
                    endforeach()

                    if( NOT _brings_dropped )
                        list( APPEND _result ${_lib} )
                    endif()
                endif()
            endif()
        endforeach()
    endif()

    set( ${aResult} ${_result} PARENT_SCOPE )
endfunction()


# Keeps the targets of the wx-free build with their sources, and takes the others out of
# the build (and out of the link interfaces of the kept ones).
function( kicad_wx_free_restrict_targets )
    # Targets kept whole, besides the ones of the source lists
    set( _kept_whole
        kiwx
        kicore_rt
        kiapi
        kiplatform_core
        kimath
        fmt
        sexpr
        nlohmann_json_schema_validator
        glad                # GL types for the GLU tessellator of the VRML export (idf3)
        idf3                # also linked whole by the IDF 3D plugin
        kicad-cli
        kicommon_model
        eeschema_cli_kiface eeschema_cli_kiface_archive
        pcbnew_cli_kiface pcbnew_cli_kiface_archive
        s3d_plugin_idf s3d_plugin_oce s3d_plugin_vrml     # 3D models (VRML export, render)

        # The QA without the GUI (KICAD_BUILD_QA_TESTS, see qa/CMakeLists.txt)
        qa_utils qa_kimath qa_sexpr
        qa_headless_pcbnew qa_headless_pcbnew_objects
        qa_headless_eeschema qa_headless_eeschema_objects
        ${KICAD_WX_FREE_EXTRA_TARGETS}
        )

    set( _kept ${KICAD_WX_FREE_TARGETS} ${_kept_whole} )

    _kicad_wx_free_all_targets( ${PROJECT_SOURCE_DIR} _all )

    # Targets of KiCad that are not kept (the link interfaces are cleaned of them)
    set( _dropped "" )

    foreach( _target ${_all} )
        get_target_property( _type ${_target} TYPE )

        if( _type STREQUAL "INTERFACE_LIBRARY" )
            continue()
        endif()

        # The custom targets are built when a kept target needs them, not by default
        if( _type STREQUAL "UTILITY" )
            set_target_properties( ${_target} PROPERTIES EXCLUDE_FROM_ALL TRUE )
            continue()
        endif()

        if( NOT _target IN_LIST _kept )
            list( APPEND _dropped ${_target} )
            set_target_properties( ${_target} PROPERTIES EXCLUDE_FROM_ALL TRUE )
        endif()
    endforeach()

    foreach( _target ${_kept} )
        if( NOT TARGET ${_target} )
            continue()
        endif()

        # The wx headers of the precompiled headers would be the GUI ones
        set_target_properties( ${_target} PROPERTIES DISABLE_PRECOMPILE_HEADERS ON )

        foreach( _property LINK_LIBRARIES INTERFACE_LINK_LIBRARIES )
            get_target_property( _libs ${_target} ${_property} )

            if( NOT _libs )
                continue()
            endif()

            set( _new_libs "" )

            foreach( _lib ${_libs} )
                _kicad_wx_free_link_item( "${_lib}" "${_dropped}" "" _items )
                list( APPEND _new_libs ${_items} )
            endforeach()

            # A dropped target can link back the kept one
            foreach( _self ${_target} "$<LINK_ONLY:${_target}>" )
                list( REMOVE_ITEM _new_libs ${_self} )
            endforeach()

            list( REMOVE_DUPLICATES _new_libs )
            set_target_properties( ${_target} PROPERTIES ${_property} "${_new_libs}" )
        endforeach()

        string( REPLACE "-" "_" _var ${_target} )

        if( _target IN_LIST _kept_whole OR NOT DEFINED KICAD_WX_FREE_SOURCES_${_var} )
            continue()
        endif()

        # The allowed sources, as absolute paths
        set( _allowed "" )

        foreach( _src ${KICAD_WX_FREE_SOURCES_${_var}} )
            if( _src MATCHES "^BUILD/(.*)" )
                list( APPEND _allowed ${PROJECT_BINARY_DIR}/${CMAKE_MATCH_1} )
            else()
                list( APPEND _allowed ${PROJECT_SOURCE_DIR}/${_src} )
            endif()
        endforeach()

        get_target_property( _sources ${_target} SOURCES )
        get_target_property( _source_dir ${_target} SOURCE_DIR )
        get_target_property( _binary_dir ${_target} BINARY_DIR )
        set( _kept_sources "" )

        foreach( _src ${_sources} )
            # The objects of a target that is not built go away
            if( _src MATCHES "^\\$<TARGET_OBJECTS:([^>]+)>$" )
                if( NOT CMAKE_MATCH_1 IN_LIST _dropped )
                    list( APPEND _kept_sources ${_src} )
                endif()

                continue()
            endif()

            # Other generator expressions and headers stay
            if( _src MATCHES "^\\$<" OR NOT _src MATCHES "\\.(c|cpp|cxx|cc)$" )
                list( APPEND _kept_sources ${_src} )
                continue()
            endif()

            get_filename_component( _abs ${_src} ABSOLUTE BASE_DIR ${_source_dir} )

            if( NOT _abs IN_LIST _allowed )
                get_filename_component( _abs ${_src} ABSOLUTE BASE_DIR ${_binary_dir} )
            endif()

            if( _abs IN_LIST _allowed )
                list( APPEND _kept_sources ${_src} )
            endif()
        endforeach()

        set_target_properties( ${_target} PROPERTIES SOURCES "${_kept_sources}" )
    endforeach()
endfunction()
