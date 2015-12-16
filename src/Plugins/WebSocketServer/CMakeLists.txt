# Define target name and output directory
init_target (WebSocketServer OUTPUT Plugins)

macro(configure_boost)
    if (NOT Boost_FOUND)
        if (MSVC)
            set(Boost_USE_MULTITHREADED TRUE)
            set(Boost_USE_STATIC_LIBS TRUE)
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
        set(Boost_ADDITIONAL_VERSIONS "1.39.0" "1.40.0" "1.41.0" "1.42.0" "1.43.0" "1.44.0" "1.46.1" "1.49.0")

        if ("${TUNDRA_BOOST_LIBRARIES}" STREQUAL "")
            set(TUNDRA_BOOST_LIBRARIES system)
        endif()

        find_package(Boost 1.39.0 COMPONENTS ${TUNDRA_BOOST_LIBRARIES})

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
    # Visual Studio uses library auto-linking via include pragmas.
    if (UNIX OR APPLE)
        if (NOT Boost_FOUND)
            message(FATAL_ERROR "Boost has not been found with configure_boost!")
        else ()
            target_link_libraries(${TARGET_NAME} ${Boost_LIBRARIES})
        endif()
    endif()
endmacro (link_boost)

configure_boost()

# Define source files
file (GLOB CPP_FILES *.cpp)
file (GLOB H_FILES *.h)

add_definitions (-DWEBSOCKETSERVER_EXPORTS)

# Includes
UseTundraCore ()
use_modules(TundraCore Plugins/TundraLogic)

if (WIN32)
include_directories(
    ${ENV_TUNDRA_DEP_PATH}/websocketpp
)
else()
include_directories(
    ${ENV_TUNDRA_DEP_PATH}/include
)
endif()

# Needed due to CMake transitional dependency
use_package(BULLET)

build_library (${TARGET_NAME} SHARED ${CPP_FILES} ${H_FILES} ${MOC_SRCS})

# Linking
link_modules(TundraCore Plugins/TundraLogic)
link_package(KNET)

if (WINDOWS)
    target_link_libraries (${TARGET_NAME}
        ws2_32.lib
    )
else()
    # Visual Studio uses library auto-linking, so this is only necessary for the other platforms.
    target_link_libraries(${TARGET_NAME} ${Boost_LIBRARIES})
endif()

SetupCompileFlags()

final_target()
