# =============================================================================
# Configuration macros for global dependencies.
#
# All global dependency configurations should go here.
# All per-module dependency configurations should go in <Module>/CMakeLists.txt.

macro(configure_mathgeolib)
    set(MATHGEOLIB_DIR ${ENV_TUNDRA_DEP_PATH}/MathGeoLib)
    set(MATHGEOLIB_INCLUDE_DIRS ${MATHGEOLIB_DIR}/build/include ${MATHGEOLIB_DIR}/build/include/MathGeoLib)
    set(MATHGEOLIB_LIBRARIES ${MATHGEOLIB_DIR}/build/lib/${CMAKE_STATIC_LIBRARY_PREFIX}MathGeoLib${CMAKE_STATIC_LIBRARY_SUFFIX})
    if (WIN32)
        set(MATHGEOLIB_DEBUG_LIBRARIES ${MATHGEOLIB_DIR}/build/lib/${CMAKE_STATIC_LIBRARY_PREFIX}MathGeoLib_d${CMAKE_STATIC_LIBRARY_SUFFIX})
    endif()
endmacro (configure_mathgeolib)

macro(configure_urho3d)
    # Find Urho3D library
    find_package (Urho3D REQUIRED)
    # Set the debug libraries to match our convention
    set (URHO3D_DEBUG_LIBRARIES ${URHO3D_LIBRARIES_DBG})
    # Using just find_package does not set all Urho3D definitions. Set some most important of them manually
    add_definitions(-DURHO3D_LOGGING)
    add_definitions(-DURHO3D_PROFILING)
endmacro(configure_urho3d)
