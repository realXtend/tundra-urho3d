# Set project name
project(tundra-urho3d)

# CMake version requirement
cmake_minimum_required(VERSION 2.6.0)

# Make our cmake macros to be available for include
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)
include (TundraUtils)

message("") # Newline before any output from the CMake script.

# Declare options and cached variables that
# can be overridden via  cmake cmd line -D<var>=foo

set (URHO3D_HOME         ""      CACHE PATH "Urho3D dependency directory")
set (MATHGEOLIB_HOME     ""      CACHE PATH "MathGeoLib dependency directory")
set (KNET_HOME           ""      CACHE PATH "kNet dependency directory")

# If not passed to cmake, set and cache normalized path for environment variables
if (NOT URHO3D_HOME)
    file (TO_CMAKE_PATH "$ENV{URHO3D_HOME}" URHO3D_HOME)
    set (URHO3D_HOME ${URHO3D_HOME} CACHE PATH "Urho3D dependency directory")
endif()
if (NOT MATHGEOLIB_HOME)
    file (TO_CMAKE_PATH "$ENV{MATHGEOLIB_HOME}" MATHGEOLIB_HOME)
    set (MATHGEOLIB_HOME ${MATHGEOLIB_HOME} CACHE PATH "MathGeoLib dependency directory")
endif()
if (NOT KNET_HOME)
    file (TO_CMAKE_PATH "$ENV{KNET_HOME}" KNET_HOME)
    set (KNET_HOME ${KNET_HOME} CACHE PATH "MathGeoLib dependency directory")
endif()

# If we still haven't resolved dependencies report a fatal error
if (NOT URHO3D_HOME)
    message(FATAL_ERROR "URHO3D_HOME could not be resolved from a environment variable or from cmake cache!")
endif ()
if (NOT MATHGEOLIB_HOME)
    message(FATAL_ERROR "MATHGEOLIB_HOME could not be resolved from a environment variable or from cmake cache!")
endif ()
if (NOT KNET_HOME)
    message(FATAL_ERROR "KNET_HOME could not be resolved from a environment variable or from cmake cache!")
endif ()

# CMake policies
if (COMMAND cmake_policy)
    cmake_policy(SET CMP0005 NEW)
    cmake_policy (SET CMP0003 NEW)
    if (CMAKE_VERSION VERSION_GREATER 2.8.12 OR CMAKE_VERSION VERSION_EQUAL 2.8.12)
        cmake_policy (SET CMP0022 NEW) # INTERFACE_LINK_LIBRARIES defines the link interface
    endif ()
    if (CMAKE_VERSION VERSION_GREATER 3.0.0 OR CMAKE_VERSION VERSION_EQUAL 3.0.0)
        cmake_policy (SET CMP0026 OLD) # Disallow use of the LOCATION target property - therefore we set to OLD as we still need it
        cmake_policy (SET CMP0042 NEW) # MACOSX_RPATH is enabled by default
    endif ()
endif ()

# Compiler-specific C++11 activation. Partial C++11 support is mandatory for Tundra-urho3d.
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
    if (NOT (GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7))
        message(FATAL_ERROR "${PROJECT_NAME} C++11 features require GCC 4.7 or greater.")
    else ()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
        if (APPLE)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
        endif ()
    endif ()
endif ()
if (APPLE)
    if ("${CMAKE_GENERATOR}" MATCHES "Xcode")
        set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++11")
        set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
    endif ()
endif ()
if (MSVC AND MSVC90) # MSVC has partial C++11 support beginning in VC10
    message(FATAL_ERROR "VC9 does not support C++11 features!")
endif()

# Set CMake library search policy
if (COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
    cmake_policy(SET CMP0005 NEW)
endif()

if (APPLE)
    EXEC_PROGRAM(/usr/bin/sw_vers ARGS -productVersion OUTPUT_VARIABLE OSX_MINOR_VERSION_RAW)
    STRING(REGEX REPLACE "10\\.([0-9]).*" "\\1" MACOSX_VERSION "${OSX_MINOR_VERSION_RAW}")
    if(${MACOSX_VERSION} LESS 6)
        message(FATAL_ERROR "Unsupported version of OS X: ${OSX_MINOR_VERSION_RAW}")
        return()
    elseif (${MACOSX_VERSION} LESS 7)
        set(CMAKE_XCODE_ATTRIBUTE_GCC_VERSION "com.apple.compilers.llvm.clang.1_0")
    endif()
endif()

# Detect that a supported Visual Version is used and set up helper variables VC_VER and VS_PLATFORM
if (MSVC)
    if (CMAKE_CL_64)
        set(VS_PLATFORM "x64")
    else()
        SET(VS_PLATFORM "Win32")
    endif()
    if (MSVC10)
        set(VC_VER "vc10")
    elseif(MSVC11)
        set(VC_VER "vc11")
    elseif(MSVC12)
        set(VC_VER "vc12")
    else()
        message(WARNING "-- Warning: Unsupported Visual Studio version detected. Currently Visual Studio 2010 and newer are supported")
    endif()
endif()

# Set compiler warning levels.
if (MSVC)
    add_definitions(/W4)
    if (MSVC10) # We are using 'override' but it's a nonstandard extension in VC10 causing a warning.
        add_definitions(/wd4481)
    endif()
else ()
    add_definitions (-Wall -Wextra -Wno-unknown-pragmas -Wno-unused -Wno-reorder -Wno-switch -Wno-write-strings)
endif()

# Disable unnecessary build types
set(CMAKE_CONFIGURATION_TYPES "Release;RelWithDebInfo;Debug" CACHE STRING "Configurations" FORCE)

if (WIN32)
    add_definitions(-DUNICODE -D_UNICODE)# Always use Unicode character set on Windows.
endif()

if (MSVC)
    # TODO Due to the C++11/C99 requirement, we should use the safe versions of sprintf() et al.
    # and the following should not be used.
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    # TODO This was/is needed for kNet, will not anymore as soon as kNet's master branch is merged to stable.
    add_definitions(-DUNIX)
endif()

# Enable MathGeoLib-Urho3D interoperability by default everywhere.
add_definitions(-DMATH_URHO3D_INTEROP)

# On all platforms, we enable PCH files by default. TODO Not implemented on non-Windows plaforms!
# TODO Make this an option instead?
add_definitions (-DPCH_ENABLED)
SET(PCH_ENABLED 1)

BeginSection("Dependency Variables")
message(STATUS "URHO3D_HOME      = " ${URHO3D_HOME})
message(STATUS "MATHGEOLIB_HOME  = " ${MATHGEOLIB_HOME})
message(STATUS "KNET_HOME        = " ${KNET_HOME})

BeginSection("Configuring Dependencies")

# Set CMake custom module path & include them
# The following CMake modules are required for the project to build.
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${URHO3D_HOME}/Source/CMake/Modules)
include (FindUrho3D)
include (ConfigurePackages)
include (ModuleSetup)
include (TundraInstall)

# Find/configure Urho
configure_urho3d()

# Find/configure MathGeoLib
configure_mathgeolib()

# Find/configure kNet
configure_knet()

EndSection()

# Set global hardcoded install prefix. User cannot change this at the moment, until we figure how we want to use this!
# Call the cleanup step that cleans the install prefix before every installations. This is important as module setups might change between builds.
set (CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install)
set (TUNDRA_BIN ${CMAKE_SOURCE_DIR}/bin)
setup_clean_install_step()

# If the custom optional modules configuration file does not yet
# exist, create it from the template file.
if (NOT EXISTS CMakeBuildConfig.txt)
    execute_process(COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/CMakeBuildConfigTemplate.txt
        ${CMAKE_BINARY_DIR}/CMakeBuildConfig.txt)
endif ()

# Setup library output dir for Android
if (ANDROID)
    file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/src/android/libs)
    file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/src/android/libs/${ANDROID_ABI})
    # Deploy Urho3D library
    # Todo: should rather happen as part of the deps build
    foreach (lib ${URHO3D_LIBRARIES})
        execute_process(COMMAND ${CMAKE_COMMAND} -E copy ${lib} ${CMAKE_SOURCE_DIR}/src/android/libs/${ANDROID_ABI})
    endforeach ()
endif ()

# Read the set of optional modules from another file
# that is kept outside the source control.
# To configure the set of optional modules to add to the build,
# edit CMakeBuildConfig.txt and rerun cmake.
include(${CMAKE_BINARY_DIR}/CMakeBuildConfig.txt)

BeginSection("Used Build Configuration")
message(STATUS "CMAKE_INSTALL_PREFIX        " ${CMAKE_INSTALL_PREFIX})
message(STATUS "ENABLE_BUILD_OPTIMIZATIONS  " ${ENABLE_BUILD_OPTIMIZATIONS})
message(STATUS "ENABLE_TESTS                " ${ENABLE_TESTS})
message(" ")
