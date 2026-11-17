# kitess: the polygon tessellator of GLU (libtess of Mesa GLU 9.0.2, SGI Free Software License B
# 2.0), with the gluTess*() API.  Used instead of libGLU by the build without OpenGL
# (KICAD_HEADLESS_ONLY), for the VRML export (utils/idftools/vrml_layer.cpp).

add_library( kitess STATIC
    src/dict.c
    src/error.c
    src/geom.c
    src/memalloc.c
    src/mesh.c
    src/normal.c
    src/priorityq.c
    src/render.c
    src/sweep.c
    src/tess.c
    src/tessmono.c
    )

set_target_properties( kitess PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    C_VISIBILITY_PRESET hidden
    )

target_include_directories( kitess
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${PROJECT_SOURCE_DIR}/thirdparty/glad/include
    )

# Selects <kitess/glu.h> in <kicad_gl/kiglu.h>
target_compile_definitions( kitess PUBLIC KICAD_GLU_TESS )

if( MSVC )
    target_compile_options( kitess PRIVATE /wd4244 /wd4101 /wd4761 )
else()
    target_compile_options( kitess PRIVATE -w )
endif()
