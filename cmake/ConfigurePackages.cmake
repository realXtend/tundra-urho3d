# =============================================================================
# Configuration macros for global dependencies.
#
# All global dependency configurations should go here.
# All per-module dependency configurations should go in <Module>/CMakeLists.txt.

macro(configure_mathgeolib)
    set(MATHGEOLIB_DIR ${ENV_TUNDRA_DEP_PATH}/MathGeoLib)
    set(MATHGEOLIB_INCLUDE_DIRS ${MATHGEOLIB_DIR}/build/include)
    set(MATHGEOLIB_LIBRARY_DIRS ${MATHGEOLIB_DIR}/build/lib)
    set(MATHGEOLIB_DEBUG_LIBRARIES MathGeoLib)
    set(MATHGEOLIB_RELEASE_LIBRARIES MathGeoLib_d)
endmacro (configure_mathgeolib)
