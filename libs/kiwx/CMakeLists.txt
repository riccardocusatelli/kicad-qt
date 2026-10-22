# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright The KiCad Developers, see AUTHORS.txt for contributors.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 3
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# kiwx: the subset of the wxBase 3.2 API that the core of KiCad uses, implemented by KiCad
# (with the wx names, so the core sources build unchanged).  Only used by the KICAD_WX_FREE
# build: the GUI build uses wxWidgets.

find_package( EXPAT REQUIRED )
find_package( ZLIB REQUIRED )
find_package( Threads REQUIRED )

# PCRE2 with the code unit of wchar_t: 16 bit on Windows, 32 bit elsewhere (as wxWidgets)
if( WIN32 )
    set( KIWX_PCRE2_WIDTH 16 )
else()
    set( KIWX_PCRE2_WIDTH 32 )
endif()

# the CMake package (vcpkg), else pkg-config
find_package( PCRE2 CONFIG QUIET COMPONENTS ${KIWX_PCRE2_WIDTH}BIT )

if( TARGET PCRE2::${KIWX_PCRE2_WIDTH}BIT )
    set( KIWX_PCRE2_TARGET PCRE2::${KIWX_PCRE2_WIDTH}BIT )
else()
    find_package( PkgConfig REQUIRED )
    pkg_check_modules( KIWX_PCRE2 REQUIRED IMPORTED_TARGET libpcre2-${KIWX_PCRE2_WIDTH} )
    set( KIWX_PCRE2_TARGET PkgConfig::KIWX_PCRE2 )
endif()

add_library( kiwx SHARED
    src/any.cpp
    src/app.cpp
    src/archive.cpp
    src/arrstr.cpp
    src/base64.cpp
    src/config.cpp
    src/datetime.cpp
    src/dynlib.cpp
    src/fileconf.cpp
    src/filefn.cpp
    src/filename.cpp
    src/files.cpp
    src/format.cpp
    src/gdicmn.cpp
    src/init.cpp
    src/intl.cpp
    src/log.cpp
    src/mimetype.cpp
    src/msgout.cpp
    src/regex.cpp
    src/stdpaths.cpp
    src/strconv.cpp
    src/stream.cpp
    src/string.cpp
    src/sysopt.cpp
    src/uri.cpp
    src/variant.cpp
    src/xml.cpp
    )

# The services of the operating system
if( WIN32 )
    target_sources( kiwx PRIVATE
        src/msw/dynlib_msw.cpp
        src/msw/file_msw.cpp
        src/msw/registry_msw.cpp
        src/msw/stdpaths_msw.cpp
        src/msw/utils_msw.cpp
        )
else()
    target_sources( kiwx PRIVATE
        src/unix/dynlib_unix.cpp
        src/unix/file_unix.cpp
        src/unix/stdpaths_unix.cpp
        src/unix/utils_unix.cpp
        )
endif()

target_include_directories( kiwx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include )

# The whole API is exported, as wxWidgets does (WXDLLIMPEXP_BASE).  On Windows the functions
# are exported by CMake, the data by WXDLLIMPEXP_BASE / WXDLLIMPEXP_DATA_BASE (see wx/defs.h).
set_target_properties( kiwx PROPERTIES
    CXX_VISIBILITY_PRESET default
    VISIBILITY_INLINES_HIDDEN OFF
    WINDOWS_EXPORT_ALL_SYMBOLS ON
    )

target_compile_definitions( kiwx PRIVATE KIWX_BUILDING )

target_link_libraries( kiwx
    PRIVATE
        EXPAT::EXPAT
        ZLIB::ZLIB
        ${KIWX_PCRE2_TARGET}
        ${CMAKE_DL_LIBS}
    PUBLIC
        Threads::Threads
    )

if( WIN32 )
    # the known folders, PathMatchSpec()
    target_link_libraries( kiwx PRIVATE shell32 shlwapi ole32 uuid )
endif()

if( MINGW )
    # WINDOWS_EXPORT_ALL_SYMBOLS is for the MSVC tools; GNU ld exports all only without
    # dllexport
    target_link_options( kiwx PRIVATE "LINKER:--export-all-symbols" )
endif()

if( CMAKE_SYSTEM_NAME STREQUAL "Linux" )
    target_link_options( kiwx PRIVATE "LINKER:--no-undefined" )
endif()

install( TARGETS kiwx
    DESTINATION ${KICAD_LIB}
    COMPONENT binary
    )
