# Set project name
project(tundra-urho3d)

# CMake version requirement
cmake_minimum_required(VERSION 2.6.0)

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

macro(PrintUsedDependencyVariables)
    BeginSection("Dependency Variables")
    message(STATUS "URHO3D_HOME      = " ${URHO3D_HOME})
    message(STATUS "MATHGEOLIB_HOME  = " ${MATHGEOLIB_HOME})
    message(STATUS "KNET_HOME        = " ${KNET_HOME})
endmacro()

macro(PrintUsedBuildConfig)
    BeginSection("Used Build Configuration")
    message(STATUS "Install prefix = " ${CMAKE_INSTALL_PREFIX})
endmacro()

macro(BeginSection title)
    EndSection()
    string(TOUPPER ${title} title_upper)
    string(REGEX REPLACE "." "-" title_line ${title_upper})
    message("\n${title_upper}")
    message("${title_line}\n")
    set(_SECTION_ENDED FALSE)
endmacro()

macro(EndSection)
    if (NOT _SECTION_ENDED)
        message(" ")
    endif()
    set(_SECTION_ENDED TRUE)
endmacro()
set(_SECTION_ENDED TRUE) # init section macro state

# AddProject takes in one or two parameters:
# - One param:  argv0 == the relative directory (to the root CMakeLists.txt) where you project is and that contains the CMakeLists.txt for this project.
# - Two params: argv0 == project type/category (used also for the solution folder's name), wrap in double quotes if the name contains spaces
#               argv1 == same as in one arg situation.
# More useful for building the Tundra project when including platform provided projects.
# Note: Due to the CMake's add_subdirectory restrictions, the directory cannot be outside the Tundra source tree, hopefully we can go around this one day.
# Examples:     AddProject(TundraCore)
#               AddProject("Application Plugins" Plugins/JavascriptModule)
#               AddProject(mysubdir/MyPlugin)
function(AddProject)
    set(PROJECT_TYPE ${ARGV0})
    if (NOT ARGV1)
        add_subdirectory(${ARGV0})
    else()
        add_subdirectory(src/${ARGV1})
    endif()
    set(PROJECT_TYPE "")
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
    # TODO PCH for other platforms
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

# On all platforms, we enable PCH files by default. TODO Not implemented on non-Windows plaforms!
# TODO Make this an option instead?
add_definitions (-DPCH_ENABLED)
SET(PCH_ENABLED 1)

PrintUsedDependencyVariables ()

BeginSection("Configuring Dependencies")

# Set CMake custom module path & include them
# The following CMake modules are required for the project to build.
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${URHO3D_HOME}/Source/CMake/Modules ${CMAKE_SOURCE_DIR}/cmake)
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

# Convenience macro for including all TundraCore subfolders.
macro(UseTundraCore)
    include_directories(${CMAKE_SOURCE_DIR}/src/TundraCore/
        ${CMAKE_SOURCE_DIR}/src/TundraCore/Asset
        ${CMAKE_SOURCE_DIR}/src/TundraCore/Framework
        ${CMAKE_SOURCE_DIR}/src/TundraCore/JSON
        ${CMAKE_SOURCE_DIR}/src/TundraCore/Scene
        ${CMAKE_SOURCE_DIR}/src/TundraCore/Signals
    )
    # Due to automatic CMake dependency transfer, we will need kNet and MathGeoLib link directories
    # Urho3D include directories are also needed throughout the project
    use_package(URHO3D)
    use_package(MATHGEOLIB)
    use_package(KNET)
endmacro()

# Read the set of optional modules from another file
# that is kept outside the source control.
# To configure the set of optional modules to add to the build,
# edit CMakeBuildConfig.txt and rerun cmake.
include(${CMAKE_BINARY_DIR}/CMakeBuildConfig.txt)

PrintUsedBuildConfig ()
