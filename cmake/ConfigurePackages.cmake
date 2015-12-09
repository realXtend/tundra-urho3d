# =============================================================================
# Configuration macros for global dependencies.
#
# All global dependency configurations should go here.
# All per-module dependency configurations should go in <Module>/CMakeLists.txt.

macro(configure_mathgeolib)
    if ("${MATHGEOLIB_HOME}" STREQUAL "")
        message(FATAL_ERROR "MATHGEOLIB_HOME not set")
    endif()
    set(MATHGEOLIB_INCLUDE_DIRS ${MATHGEOLIB_HOME}/include ${MATHGEOLIB_HOME}/include/MathGeoLib)
    set(MATHGEOLIB_LIBRARY_DIRS ${MATHGEOLIB_HOME}/lib)
    set(MATHGEOLIB_LIBRARIES MathGeoLib)
    if (WIN32)
        set(MATHGEOLIB_DEBUG_LIBRARIES MathGeoLib_d)
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

macro(configure_knet)
    if ("${KNET_HOME}" STREQUAL "")
        message(FATAL_ERROR "KNET_HOME not set")
    endif()
    set(KNET_INCLUDE_DIRS ${KNET_HOME}/include ${KNET_HOME}/include/kNet)
    set(KNET_LIBRARY_DIRS ${KNET_HOME}/lib)
    set(KNET_LIBRARIES kNet)
    if (WIN32)
        set(KNET_LIBRARIES ${KNET_LIBRARIES} ws2_32)
        set(KNET_DEBUG_LIBRARIES kNet_d ws2_32)
    endif()
endmacro (configure_knet)

macro(configure_bullet)
    if ("${BULLET_HOME}" STREQUAL "")
        message(FATAL_ERROR "BULLET_HOME not set")
    endif()
    if (WIN32 OR ANDROID)
        set(BULLET_INCLUDE_DIRS ${BULLET_HOME}/src)
    else()
        set(BULLET_INCLUDE_DIRS ${BULLET_HOME}/include/bullet)
    endif()
    set(BULLET_LIBRARY_DIRS ${BULLET_HOME}/lib)
    set(BULLET_LIBRARIES BulletDynamics BulletCollision LinearMath)
    if (WIN32)
        set(BULLET_DEBUG_LIBRARIES BulletDynamics_d BulletCollision_d LinearMath_d)
    endif()
endmacro (configure_bullet)
