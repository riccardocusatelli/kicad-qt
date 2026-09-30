# Add all the warnings to the files
if( COMPILER_SUPPORTS_WARNINGS )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARN_FLAGS_CXX}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARN_FLAGS_C}")
endif()


# Process-wide state of the low level libraries (assert handler, log backend, geometry
# tuning).  Shared in the KiCad build so kicommon, kigal and every kiface see one instance;
# static for standalone builds (qa/headless_core sets KICORE_RT_SHARED OFF).
option( KICORE_RT_SHARED "Build kicore_rt as a shared library" ON )

if( KICORE_RT_SHARED )
    add_library( kicore_rt SHARED
        ki_diagnostics.cpp
        geometry_tuning.cpp
    )

    target_compile_definitions( kicore_rt PRIVATE KICORE_RT_DLL=1 )
    set_target_properties( kicore_rt PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        OUTPUT_NAME kicore_rt
    )

    if( KICAD_CMAKE_MODULE_PATH )
        include( ${KICAD_CMAKE_MODULE_PATH}/KiCadVersion.cmake )
        string( REGEX MATCH "([0-9]+)\\.([0-9]+)\\.([0-9]+)" KICORE_RT_SOVERSION "${KICAD_VERSION}" )
        set_target_properties( kicore_rt PROPERTIES SOVERSION "${KICORE_RT_SOVERSION}" )
    endif()

    install( TARGETS kicore_rt
        RUNTIME DESTINATION ${KICAD_LIB}
        LIBRARY DESTINATION ${KICAD_LIB}
        COMPONENT binary
    )

    if( APPLE )
        # puts library into the main kicad.app bundle in build tree
        set_target_properties( kicore_rt PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY "${OSX_BUNDLE_BUILD_LIB_DIR}"
            INSTALL_NAME_DIR "${OSX_BUNDLE_BUILD_LIB_DIR}"
            INSTALL_RPATH "@executable_path/../Frameworks"
            BUILD_WITH_INSTALL_RPATH 1
        )
    endif()
else()
    add_library( kicore_rt STATIC
        ki_diagnostics.cpp
        geometry_tuning.cpp
    )

    target_compile_definitions( kicore_rt PUBLIC KICORE_RT_STATIC )
endif()

target_compile_definitions( kicore_rt PRIVATE KICAD_NO_WX )
target_include_directories( kicore_rt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include )


# Toolkit-independent part: no wxWidgets headers or libraries.  Headless tools and
# non-wx front ends (e.g. a Qt UI) link only this.
add_library( core_base STATIC
    base64.cpp
    observable.cpp
    profile.cpp
    utf8.cpp
    version_compare.cpp
)

target_compile_definitions( core_base PRIVATE KICAD_NO_WX )

target_link_libraries( core_base PUBLIC kicore_rt )

target_include_directories( core_base PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
    ${CMAKE_BINARY_DIR} # to get config.h
)

# wxWidgets glue (UTF8 <-> wxString, wx STL helpers).  Existing consumers link "core" and get
# both parts, exactly as before the split.  Skipped when wx is not part of the build
# (qa/headless_core).
if( wxWidgets_FOUND )
    add_library( core STATIC
        utf8_wx.cpp
        wx_stl_compat.cpp
    )

    target_link_libraries( core PUBLIC
        core_base
        ${wxWidgets_LIBRARIES}
    )

    target_include_directories( core PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE
        ${CMAKE_BINARY_DIR} # to get config.h
    )
endif()
