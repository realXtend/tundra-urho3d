# =============================================================================
# Configuration macros for global dependencies.
#
# All global dependency configurations should go here.
# All per-module dependency configurations should go in <Module>/CMakeLists.txt.

macro(configure_mathgeolib)
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
    # Add Urho root include directory to allow disambiguation between Tundra/MathGeoLib/Urho includes
    set (URHO3D_INCLUDE_DIRS ${URHO3D_INCLUDE_DIRS} ${URHO3D_HOME}/Source)
    # Using just find_package does not set all Urho3D definitions. Set some most important of them manually
    add_definitions(-DURHO3D_LOGGING)
    add_definitions(-DURHO3D_PROFILING)
endmacro(configure_urho3d)

macro(configure_knet)
    set(KNET_INCLUDE_DIRS ${KNET_HOME}/include ${KNET_HOME}/include/kNet)
    set(KNET_LIBRARY_DIRS ${KNET_HOME}/lib)
    set(KNET_LIBRARIES kNet)
    if (WIN32)
        set(KNET_LIBRARIES ${KNET_LIBRARIES} ws2_32)
        set(KNET_DEBUG_LIBRARIES kNet_d ws2_32)
    endif()
endmacro (configure_knet)

macro(configure_gtest)
    set(GTEST_INCLUDE_DIRS ${GTEST_HOME}/include)
    set(GTEST_LIBRARY_DIRS ${GTEST_HOME}/build)
    set(GTEST_LIBRARIES gtest gtest_main)
endmacro (configure_gtest)
