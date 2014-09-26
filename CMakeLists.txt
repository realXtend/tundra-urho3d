# Set project name
project(tundra-urho3d)

# CMake version requirement
cmake_minimum_required(VERSION 2.6.0)

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
    if (NOT (GCC_VERSION VERSION_GREATER 4.4 OR GCC_VERSION VERSION_EQUAL 4.4))
        message(FATAL_ERROR "${PROJECT_NAME} C++11 features require GCC 4.4 or greater.")
    else ()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
        if (APPLE)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
        endif ()
    endif ()
endif ()
if (APPLE)
    if ("${CMAKE_GENERATOR}" MATCHES "Xcode")
        set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
        set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
    endif ()
endif ()
if (MSVC)
    # MSVC has partial C++11 support beginning in VC10
    if (MSVC90)
        message(FATAL_ERROR "VC9 does not support C++11 features!")
    endif()
endif()

macro(PrintUsedEnvVariables)
    # Print out the resolved variables. Note that this does not print the env variables
    # in most cases as they should be resolved to our CMake variables and get cached.
    # This way this printed list wont lie if ran without env variables and we are still
    # successfully finding all the deps from our cacher (or in bad cases assumed) dirs.
    message("\n============ Used Environment Variables ============\n")
    message(STATUS "TUNDRA_DEP_PATH  = " ${ENV_TUNDRA_DEP_PATH})
    message(STATUS "URHO3D_HOME      = " ${URHO3D_HOME})
endmacro()

macro(PrintUsedBuildConfig)
    message("\n============ Used Build Configuration ==============\n")
    message(STATUS "Install prefix = " ${CMAKE_INSTALL_PREFIX})
    message("")
endmacro()

macro(GetLastElementFromPath dirPath outputParam)
    string (FIND ${dirPath} "/" lastPathSepIndex REVERSE)
    # This path does not have any subdirs in it
    if (${lastPathSepIndex} STREQUAL "-1")
        set (${outputParam} ${dirPath})
    # Path has subdirs, pick the last
    else ()
        math(EXPR lastPathSepIndex "${lastPathSepIndex} + 1")
        string(SUBSTRING ${dirPath} ${lastPathSepIndex} -1 ${outputParam})
    endif ()
endmacro ()

macro(RemoveLastElementFromPath dirPath outputParam)
    string (FIND ${dirPath} "/" lastPathSepIndex REVERSE)
    # This path does not have any subdirs in it
    if (${lastPathSepIndex} STREQUAL "-1")
        set (${outputParam} ${dirPath})
    # Path has subdirs, pick the path in front of it
    else ()
        string(SUBSTRING ${dirPath} 0 ${lastPathSepIndex} ${outputParam})
    endif ()
endmacro ()

# AddProject takes in one or two parameters:
# - One param  : argv0 == the relative directory (to the root CMakeLists.txt) where you project is and that contains the CMakeLists.txt for this project.
# - Two params : argv0 == 'Core'|'Application'. argv1 folder name of the project in src/
# More useful for building the Tundra project when including platform provided projects.
# Note: One param projects or two param "Application" type plugins will always be omitted if BUILD_SDK_ONLY is true!
# Note: Due to the CMake's add_subdirectory restrictions, the directory cannot be outside the Tundra source tree, hopefully we can go around this one day.
# Examples:     AddProject(Core TundraCore)
#               AddProject(Application Plugins/JavascriptModule)
#               AddProject(mysubdir/MyPlugin)
function(AddProject)
    set (PROJECT_TYPE ${ARGV0})
    if (NOT ARGV1)
        if (BUILD_SDK_ONLY)
            message (STATUS "** Omitting Application level project " ${ARGV0} " because BUILD_SDK_ONLY is set to TRUE")
        else ()
            add_subdirectory(${ARGV0})
        endif ()
    else ()
        if (NOT ${ARGV0} STREQUAL "Core" AND NOT ${ARGV0} STREQUAL "Application")
            message (FATAL_ERROR "You are trying to use AddProject macro with invalid project type parameter: " ${ARGV0} " with project name " ${ARGV1})
        endif()
        if (BUILD_SDK_ONLY AND ${ARGV0} STREQUAL "Application")
            message (STATUS "** Omitting Application level project " ${ARGV1} " because BUILD_SDK_ONLY is set to TRUE")
        else ()
            add_subdirectory(src/${ARGV1})
        endif ()
    endif ()
    set (PROJECT_TYPE "")
endfunction()

# Adds the given folder_name into the source files of the current project. Use this macro when your module contains .cpp and .h files in several subdirectories.
macro(AddSourceFolder folder_name)
    file(GLOB H_FILES_IN_FOLDER_${folder_name} ${folder_name}/*.h ${folder_name}/*.inl)
    file(GLOB CPP_FILES_IN_FOLDER_${folder_name} ${folder_name}/*.cpp)
    source_group("Header Files\\${folder_name}" FILES ${H_FILES_IN_FOLDER_${folder_name}})
    source_group("Source Files\\${folder_name}" FILES ${CPP_FILES_IN_FOLDER_${folder_name}})
    set(H_FILES ${H_FILES} ${H_FILES_IN_FOLDER_${folder_name}})
    set(CPP_FILES ${CPP_FILES} ${CPP_FILES_IN_FOLDER_${folder_name}})
endmacro()

# Enables the use of Precompiled Headers in the project this macro is invoked in. Also adds the DEBUG_CPP_NAME to each .cpp file that specifies the name of that compilation unit. MSVC only.
macro(SetupCompileFlagsWithPCH)
    if (MSVC)
        # Label StableHeaders.cpp to create the PCH file and mark all other .cpp files to use that PCH file.
        # Add a #define DEBUG_CPP_NAME "this compilation unit name" to each compilation unit to aid in memory leak checking.
        foreach(src_file ${CPP_FILES})
            if (${src_file} MATCHES "StableHeaders.cpp$")
                set_source_files_properties(${src_file} PROPERTIES COMPILE_FLAGS "/YcStableHeaders.h")        
            else()
                get_filename_component(basename ${src_file} NAME)
                set_source_files_properties(${src_file} PROPERTIES COMPILE_FLAGS "/YuStableHeaders.h -DDEBUG_CPP_NAME=\"\\\"${basename}\"\\\"")
            endif()
        endforeach()
    endif()
endmacro()

# Sets up the compilation flags without PCH. For now just set the DEBUG_CPP_NAME to each compilation unit.
# TODO: The SetupCompileFlags and SetupCompileFlagsWithPCH macros should be merged, and the option to use PCH be passed in as a param. However,
# CMake string ops in PROPERTIES COMPILE_FLAGS gave some problems with this, so these are separate for now.
macro(SetupCompileFlags)
    if (MSVC)
        # Add a #define DEBUG_CPP_NAME "this compilation unit name" to each compilation unit to aid in memory leak checking.
        foreach(src_file ${CPP_FILES})
            if (${src_file} MATCHES "StableHeaders.cpp$")
            else()
                get_filename_component(basename ${src_file} NAME)
                set_source_files_properties(${src_file} PROPERTIES COMPILE_FLAGS "-DDEBUG_CPP_NAME=\"\\\"${basename}\"\\\"")
            endif()
        endforeach()
    endif()
endmacro()

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
else ()
    add_definitions (-Wall -Wextra -Wno-unknown-pragmas -Wno-unused -Wno-reorder -Wno-switch -Wno-write-strings)
endif()

# Disable unnecessary build types
set(CMAKE_CONFIGURATION_TYPES "Release;RelWithDebInfo;Debug" CACHE STRING "Configurations" FORCE)

if (MSVC)
    # In Visual Studio, use Unicode character set
    add_definitions(-DUNICODE -D_UNICODE)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    add_definitions(-DUNIX) # Currently, kNet uses this flag for Unix OSes. (required also for client-side headers). TODO: Clean this up so it's not required.
endif()

# On all platforms, we enable PCH files by default.
add_definitions (-DPCH_ENABLED)
SET(PCH_ENABLED 1)

# Set normalized path for common environment variables
file (TO_CMAKE_PATH "$ENV{TUNDRA_DEP_PATH}" ENV_TUNDRA_DEP_PATH)
file (TO_CMAKE_PATH "$ENV{URHO3D_HOME}" ENV_URHO3D_HOME)

# Utilize CMake's cache for the environment variables. This is needed if e.g.. TUNDRA_DEP_PATH
# was set on the first run of CMake, but once e.g.. Visual Studio auto runs CMake (after changes to any CMakeLists.txt)
# it is no longer set. This will result us in losing all the acquired dependency information that was not found with
# CMake macros find_packade, find_library etc. that automatically cache. This is more flexible for many use cases to remember
# if we once got a valid TUNDRA_DEP_PATH. If env variable TUNDRA_DEP_PATH is present, the cache is never read but instead updated.
if (NOT ENV_TUNDRA_DEP_PATH)
    set (ENV_TUNDRA_DEP_PATH ${CACHED_ENV_TUNDRA_DEP_PATH})
else ()
    set (CACHED_ENV_TUNDRA_DEP_PATH ${ENV_TUNDRA_DEP_PATH} CACHE PATH "Tundra build dependency path" FORCE)
endif ()
if (NOT ENV_URHO3D_HOME)
    set (ENV_URHO3D_HOME ${CACHED_ENV_URHO3D_HOME})
else ()
    set (CACHED_ENV_URHO3D_HOME ${ENV_URHO3D_HOME} CACHE PATH "Tundra build Urho3D path" FORCE)
endif ()

# If we still haven't resolved TUNDRA_DEP_PATH report a fatal error.
if (NOT ENV_TUNDRA_DEP_PATH)
    message(FATAL_ERROR "TUNDRA_DEP_PATH could not be resolved from a environment variable or from cmake cache!")
endif ()

message("\n\n============= Configuring Dependencies =============\n")

# Set CMake custom module path & include them
# The following CMake modules are required for the project to build.
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ENV_URHO3D_HOME}/Source/CMake/Modules ${CMAKE_SOURCE_DIR}/cmake)
include (FindUrho3D)
include (ConfigurePackages)
include (ModuleSetup)
include (TundraInstall)

# Find/configure Urho
configure_urho3d()

# Find/configure MathGeoLib
configure_mathgeolib()

message(STATUS "MathGeoLib debug libs " ${MATHGEOLIB_DEBUG_LIBRARIES})
message(STATUS "MathGeoLib libs " ${MATHGEOLIB_LIBRARIES})

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

# Convenience macro for including all TundraCore subfolders.
macro(UseTundraCore)
    include_directories(${CMAKE_SOURCE_DIR}/src/TundraCore/
        ${CMAKE_SOURCE_DIR}/src/TundraCore/Signals
        ${CMAKE_SOURCE_DIR}/src/TundraCore/Framework
        ${CMAKE_SOURCE_DIR}/src/TundraCore/JSON
    )
    # Using TundraCore requires Urho3D headers as well (SharedPtr etc.)
    include_directories(${URHO3D_INCLUDE_DIRS})
endmacro()

# Read the set of optional modules from another file
# that is kept outside the source control.
# To configure the set of optional modules to add to the build,
# edit CMakeBuildConfig.txt and rerun cmake.
include(${CMAKE_BINARY_DIR}/CMakeBuildConfig.txt)

PrintUsedEnvVariables ()
PrintUsedBuildConfig ()
