# The triplet of the Qt build of KiCad on Windows (CI): the x64-windows libraries in release only
# (the Qt build is a release build), without compiler tracking, for the binary cache of the CI.
include( "${VCPKG_ROOT_DIR}/triplets/x64-windows.cmake" )

set( VCPKG_BUILD_TYPE release )
set( VCPKG_DISABLE_COMPILER_TRACKING ON )
