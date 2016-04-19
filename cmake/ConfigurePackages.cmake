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

# Boost needed by modules utilizing websocketpp
macro(configure_boost)
    if (NOT Boost_FOUND)
        if (MSVC)
            set(Boost_USE_MULTITHREADED TRUE)
            set(Boost_USE_STATIC_LIBS TRUE)
            add_definitions(-DBOOST_ALL_NO_LIB)
        else ()
            set(Boost_USE_MULTITHREADED FALSE)
            set(Boost_USE_STATIC_LIBS FALSE)
        endif ()

        # Boost lookup rules:
        # 1. If a CMake variable BOOST_ROOT was set before calling configure_boost(), that directory is used.
        # 2. Otherwise, if an environment variable BOOST_ROOT was set, use that.
        # 3. Otherwise, use Boost from the Tundra deps directory.

        if ("${BOOST_ROOT}" STREQUAL "")
            file (TO_CMAKE_PATH "$ENV{BOOST_ROOT}" BOOST_ROOT)
        endif()

        if ("${BOOST_ROOT}" STREQUAL "")
            SET(BOOST_ROOT ${ENV_TUNDRA_DEP_PATH}/boost)
        endif()

        message("** Configuring Boost")
        message(STATUS "Using BOOST_ROOT = " ${BOOST_ROOT})

        set(Boost_FIND_REQUIRED TRUE)
        set(Boost_FIND_QUIETLY TRUE)
        set(Boost_DEBUG FALSE)
        set(Boost_USE_MULTITHREADED TRUE)
        set(Boost_DETAILED_FAILURE_MSG TRUE)

        if ("${TUNDRA_BOOST_LIBRARIES}" STREQUAL "")
            set(TUNDRA_BOOST_LIBRARIES system)
        endif()

        find_package(Boost 1.54.0 COMPONENTS ${TUNDRA_BOOST_LIBRARIES})

        if (Boost_FOUND)
            include_directories(${Boost_INCLUDE_DIRS})

            # We are trying to move to absolute lib path linking.
            # This enables us to use Boost for certain functionality
            # without linking Boost to all built libraries and executables.
            # This works cleanly even if TUNDRA_NO_BOOST is defined.
            # Windows uses auto-linking to library names so it will need this dir.
            if (MSVC)
                link_directories(${Boost_LIBRARY_DIRS})
            endif()

            message(STATUS "-- Include Directories")
            foreach(include_dir ${Boost_INCLUDE_DIRS})
                message (STATUS "       " ${include_dir})
            endforeach()
            message(STATUS "-- Library Directories")
            foreach(library_dir ${Boost_LIBRARY_DIRS})
                message (STATUS "       " ${library_dir})
            endforeach()
            message(STATUS "-- Libraries")
            foreach(lib ${Boost_LIBRARIES})
                message (STATUS "       " ${lib})
            endforeach()
            message("")
        else()
            message(FATAL_ERROR "Boost not found!")
        endif()
    endif()
endmacro (configure_boost)

macro(link_boost)
    if (NOT Boost_FOUND)
        message(FATAL_ERROR "Boost has not been found with configure_boost!")
    else ()
        target_link_libraries(${TARGET_NAME} ${Boost_LIBRARIES})
    endif()
endmacro (link_boost)
