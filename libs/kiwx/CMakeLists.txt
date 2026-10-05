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
find_package( PkgConfig REQUIRED )
pkg_check_modules( PCRE2_32 REQUIRED IMPORTED_TARGET libpcre2-32 )

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
    src/filename.cpp
    src/files.cpp
    src/format.cpp
    src/gdicmn.cpp
    src/intl.cpp
    src/log.cpp
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

target_include_directories( kiwx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include )

# The whole API is exported, as wxWidgets does (WXDLLIMPEXP_BASE)
set_target_properties( kiwx PROPERTIES
    CXX_VISIBILITY_PRESET default
    VISIBILITY_INLINES_HIDDEN OFF
    )

target_link_libraries( kiwx
    PRIVATE
        EXPAT::EXPAT
        ZLIB::ZLIB
        PkgConfig::PCRE2_32
        ${CMAKE_DL_LIBS}
    PUBLIC
        Threads::Threads
    )

if( CMAKE_SYSTEM_NAME STREQUAL "Linux" )
    target_link_options( kiwx PRIVATE "LINKER:--no-undefined" )
endif()

install( TARGETS kiwx
    DESTINATION ${KICAD_LIB}
    COMPONENT binary
    )
