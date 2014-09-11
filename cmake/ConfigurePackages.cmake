# =============================================================================
# Configuration macros for global dependencies.
#
# All global dependency configurations should go here.
# All per-module dependency configurations should go in <Module>/CMakeLists.txt.

macro(configure_mathgeolib)
    set(MATHGEOLIB_DIR ${ENV_TUNDRA_DEP_PATH}/MathGeoLib)
    set(MATHGEOLIB_INCLUDE_DIRS ${MATHGEOLIB_DIR}/build/include ${MATHGEOLIB_DIR}/build/include/MathGeoLib)
    set(MATHGEOLIB_LIBRARIES ${MATHGEOLIB_DIR}/build/lib/MathGeoLib${CMAKE_STATIC_LIBRARY_SUFFIX})
    set(MATHGEOLIB_DEBUG_LIBRARIES ${MATHGEOLIB_DIR}/build/lib/MathGeoLib_d${CMAKE_STATIC_LIBRARY_SUFFIX})
endmacro (configure_mathgeolib)
