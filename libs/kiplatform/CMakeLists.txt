set( PLATFORM_COMPILE_DEFS "" )

# kiplatform_core: operating system services (paths, environment, IO, policy, secrets...),
# usable without the GUI (wxBase only).  kiplatform: the GUI toolkit ports (window and cursor
# handling, webview, printing), on top of kiplatform_core.

# Add port-specific (GUI) files and definitions
if( KICAD_WX_PORT STREQUAL osx )
    set( PLATFORM_GUI_SRCS
        port/wxosx/ui.mm
        port/wxosx/webview.mm
        )
elseif( KICAD_WX_PORT STREQUAL msw )
    set( PLATFORM_GUI_SRCS
        port/wxmsw/ui.cpp
        port/wxmsw/webview.cpp
        )
elseif( KICAD_WX_PORT STREQUAL gtk )
    set( PLATFORM_GUI_SRCS
        port/wxgtk/ui.cpp
        port/wxgtk/webview.cpp
        )

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

    # Detect GTK3 and configure it
    set( PLATFORM_GUI_LIBS
        ${GTK3_LIBRARIES}
        )
endif()


# Add OS-specific files and definitions
if( APPLE )
    set( PLATFORM_SRCS
        os/apple/app.mm
        os/apple/drivers.mm
        os/apple/environment.mm
        os/apple/io.mm
        os/apple/policy.mm
        os/apple/secrets.mm
        os/apple/sysinfo.cpp
        )

    list( APPEND PLATFORM_GUI_SRCS
        os/apple/printing.mm
        )

    set( PLATFORM_LIBS
        "-framework Cocoa"
        "-framework AppKit"
        "-framework CoreData"
        "-framework Foundation"
        "-framework PDFKit"
        "-framework WebKit"
        "-framework CFNetwork"
        )
elseif( WIN32 )
    set( PLATFORM_SRCS
        os/windows/app.cpp
        os/windows/drivers.cpp
        os/windows/environment.cpp
        os/windows/io.cpp
        os/windows/policy.cpp
        os/windows/secrets.cpp
        os/windows/sysinfo.cpp
        )

    list( APPEND PLATFORM_GUI_SRCS
        os/windows/printing.cpp
        )

    set( PLATFORM_LIBS
        "shlwapi"
        "winhttp"
        "wintrust"
        "imm32"
        "windowsapp"
        )
elseif( UNIX )
    set( PLATFORM_SRCS
        os/unix/app.cpp
        os/unix/drivers.cpp
        os/unix/environment.cpp
        os/unix/io.cpp
        os/unix/policy.cpp
        os/unix/secrets.cpp
        os/unix/sysinfo.cpp
        )

    list( APPEND PLATFORM_GUI_SRCS
        os/unix/printing.cpp
        )

    find_package(PkgConfig REQUIRED)

    # GLib and GIO (not GTK) for the OS services
    pkg_check_modules(GIO REQUIRED gio-2.0)
    set( PLATFORM_INCLUDE_DIRS ${GIO_INCLUDE_DIRS} )
    set( PLATFORM_LIBS ${GIO_LIBRARIES} )

    # Detect the secret library and configure it
    pkg_check_modules(secret REQUIRED libsecret-1)
    list( APPEND PLATFORM_INCLUDE_DIRS ${secret_INCLUDE_DIRS} )
    list( APPEND PLATFORM_LIBS ${secret_LIBRARIES} )

    # Detect Poppler library for printing and configure it
    find_package( Poppler REQUIRED )
    set( PLATFORM_GUI_INCLUDE_DIRS ${POPPLER_INCLUDE_DIRS} )
    list( APPEND PLATFORM_GUI_LIBS ${Poppler_LIBRARIES} )

    if( KICAD_WAYLAND )
        find_package(Wayland 1.20 COMPONENTS Client REQUIRED)

        list( APPEND PLATFORM_GUI_SRCS port/wxgtk/wayland-pointer-constraints-unstable-v1.c )
        list( APPEND PLATFORM_GUI_LIBS Wayland::Client  )
    endif()
endif()

include_directories(
    ${INC_AFTER}
    ${CMAKE_SOURCE_DIR}/include
    )

add_library( kiplatform_core STATIC
    os/common/io.cpp
    ${PLATFORM_SRCS}
    )

target_compile_definitions( kiplatform_core PRIVATE
    ${PLATFORM_COMPILE_DEFS}
    )

target_include_directories( kiplatform_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

target_include_directories( kiplatform_core SYSTEM PRIVATE
    ${PLATFORM_INCLUDE_DIRS}
    )

target_link_libraries( kiplatform_core
    core
    kimath
    nlohmann_json
    ${wxWidgets_BASE_LIBRARIES}
    ${PLATFORM_LIBS}
    )

add_library( kiplatform STATIC
    ${PLATFORM_GUI_SRCS}
    )

target_compile_definitions( kiplatform PRIVATE
    ${PLATFORM_COMPILE_DEFS}
    )

if( KICAD_WX_PORT STREQUAL gtk )
    target_include_directories( kiplatform SYSTEM PRIVATE ${GTK3_INCLUDE_DIRS} )
    target_link_directories( kiplatform PUBLIC ${GTK3_LIBRARY_DIRS} )
    target_compile_options( kiplatform PRIVATE ${GTK3_CFLAGS_OTHER} )
endif()

target_include_directories( kiplatform SYSTEM PRIVATE
    ${PLATFORM_GUI_INCLUDE_DIRS}
    )

target_link_libraries( kiplatform
    kiplatform_core
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_GUI_LIBS}
    )

if( APPLE )
    find_library(COREFOUNDATION_LIBRARY CoreFoundation REQUIRED)
    find_library(SECURITY_LIBRARY Security REQUIRED)

    target_link_libraries( kiplatform_core
            ${COREFOUNDATION_LIBRARY}
            ${SECURITY_LIBRARY}
    )
endif()

if( WIN32 )
add_dependencies( kiplatform_core
    version_header
    )
endif()
