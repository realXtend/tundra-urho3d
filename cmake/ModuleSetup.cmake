# Tundra build macros
#
# Generally used as follows:
# 1. call configure_${PACKAGE}() once from the main CMakeLists.txt
# 2. call init_target (${NAME}) once per build target in the module CMakeLists.txt
# 3. call use_package (${PACKAGE}) once per build target
# 3. call use_modules() with a list of local module names for includes
# 4. call build_library/executable() on the source files
# 5. call link_package (${PACKAGE}) once per build target
# 6. call link_modules() with a list of local module names libraries
# 7. call SetupCompileFlags/SetupCompileFlagsWithPCH
# 8. call final_target () at the end of build target's CMakeLists.txt
#    (not needed for lib targets, only exe's & modules)

# =============================================================================

# reusable macros

# define target name, and directory, if it should be output
# ARGV1 is directive to output, and ARGV2 is where to
macro (init_target NAME)

    # Define target name and output directory.
    # Skip ARGV1 that is the keyword OUTPUT.
    set (TARGET_NAME ${NAME})
    set (TARGET_OUTPUT ${ARGV2})
    
    message ("** " ${TARGET_NAME})

    # Include our own module path. This makes #include "x.h"
    # work in project subfolders to include the main directory headers
    # note: CMAKE_INCLUDE_CURRENT_DIR could automate this
    include_directories (${CMAKE_CURRENT_SOURCE_DIR})
    
    # Add the SDK static libs build location for linking
    link_directories (${PROJECT_BINARY_DIR}/lib)

    # set TARGET_DIR
    if (NOT "${TARGET_OUTPUT}" STREQUAL "")
        if (NOT ANDROID)
            set (TARGET_DIR ${CMAKE_SOURCE_DIR}/bin/${TARGET_OUTPUT})
            if (MSVC)
                # export symbols, copy needs to be added via copy_target
                add_definitions (-DMODULE_EXPORTS)
            endif ()
        else ()
            set (TARGET_DIR ${CMAKE_SOURCE_DIR}/src/android/libs/${ANDROID_ABI})
        endif ()
    endif ()
endmacro (init_target)

macro (final_target)
    # set TARGET_DIR
    if (TARGET_DIR)
        if (MSVC)
            # copy to target directory
            add_custom_command (TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} ARGS -E make_directory ${TARGET_DIR})
            add_custom_command (TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different \"$(TargetPath)\" ${TARGET_DIR})
        else ()
            # set target directory
            set_target_properties (${TARGET_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${TARGET_DIR})
            set_target_properties (${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${TARGET_DIR})
            if (APPLE)
                # Avoid putting built targets in ./bin/[RelWithDebInfo | Release | Debug] and ./bin/plugins/[RelWithDebInfo | Release | Debug]
                # Only one rule should be in place here, to avoid a bug (which is reported as fixed) in CMake where different build setups are used
                # which results already built libs to be deleted and thus causing build errors in further modules.
                # See  http://public.kitware.com/Bug/view.php?id=11844 for more info

                if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
                    set_target_properties (${TARGET_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG ${TARGET_DIR})
                    set_target_properties (${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${TARGET_DIR})
                elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
                    set_target_properties (${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${TARGET_DIR})
                    set_target_properties (${TARGET_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${TARGET_DIR})
                elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
                    set_target_properties (${TARGET_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE ${TARGET_DIR})
                    set_target_properties (${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${TARGET_DIR})
                endif()

                set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -flat_namespace")
            endif()
        endif ()
    endif ()

    if (NOT "${PROJECT_TYPE}" STREQUAL "")
        # message (STATUS "project type: " ${PROJECT_TYPE})
        set_target_properties (${TARGET_NAME} PROPERTIES FOLDER ${PROJECT_TYPE})
    endif ()

    message("") # newline after each project

    # run the setup install macro for everything included in this build
    setup_install_target ()
    
endmacro (final_target)

# build a library from internal sources
macro (build_library TARGET_NAME LIB_TYPE)

    set (TARGET_LIB_TYPE ${LIB_TYPE})

    message (STATUS "build type: " ${TARGET_LIB_TYPE} " library")
   
    # *unix add -fPIC for static libraries
    if (UNIX AND ${TARGET_LIB_TYPE} STREQUAL "STATIC")
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
    endif ()

    # Version information for shared libraries. Disabled for now
    #if (${TARGET_LIB_TYPE} STREQUAL "SHARED")
    #    if (WIN32)
    #        set(RESOURCE_FILES ${RESOURCE_FILES}
    #            ${PROJECT_SOURCE_DIR}/src/TundraCore/Resources/resource.h
    #            ${PROJECT_SOURCE_DIR}/src/TundraCore/Resources/TundraPlugin.rc)
    #    endif()
    #    # TODO non-Windows platforms.
    #endif()

    add_library(${TARGET_NAME} ${TARGET_LIB_TYPE} ${ARGN} ${RESOURCE_FILES})

    if (MSVC AND ENABLE_BUILD_OPTIMIZATIONS)
        if (${TARGET_LIB_TYPE} STREQUAL "SHARED")
            set_target_properties (${TARGET_NAME} PROPERTIES LINK_FLAGS_RELEASE ${CMAKE_SHARED_LINKER_FLAGS_RELEASE})
            set_target_properties (${TARGET_NAME} PROPERTIES LINK_FLAGS_RELWITHDEBINFO ${CMAKE_SHARED_LINKER_FLAGS_RELEASE})
        else ()
            set_target_properties (${TARGET_NAME} PROPERTIES STATIC_LIBRARY_FLAGS_RELEASE "/LTCG")
            set_target_properties (${TARGET_NAME} PROPERTIES STATIC_LIBRARY_FLAGS_RELWITHDEBINFO "/LTCG")
        endif ()
    endif ()

    # build static libraries to /lib
    if (${TARGET_LIB_TYPE} STREQUAL "STATIC" AND NOT TARGET_DIR)
        message (STATUS "       " ${PROJECT_BINARY_DIR}/lib)
        set_target_properties (${TARGET_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
    endif ()

    # internal library naming convention
    set_target_properties (${TARGET_NAME} PROPERTIES DEBUG_POSTFIX _d)
    # Android always needs lib prefix
    if (NOT ANDROID AND NOT (${TARGET_NAME} STREQUAL "Tundra"))
        set_target_properties (${TARGET_NAME} PROPERTIES PREFIX "")
    endif ()
    set_target_properties (${TARGET_NAME} PROPERTIES LINK_INTERFACE_LIBRARIES "")
    
    # Add common system library dependencies for Android
    if (ANDROID)
        target_link_libraries (${TARGET_NAME} log)
    endif ()
endmacro (build_library)

# build an executable from internal sources 
macro (build_executable TARGET_NAME)
    set (TARGET_LIB_TYPE "EXECUTABLE")
    message(STATUS "building executable: " ${TARGET_NAME})

    add_executable (${TARGET_NAME} ${ARGN})
    if (MSVC)
        target_link_libraries (${TARGET_NAME} optimized dbghelp.lib)
    endif ()

    set_target_properties (${TARGET_NAME} PROPERTIES DEBUG_POSTFIX _d)
endmacro()

# include and lib directories, and definitions
macro (use_package PREFIX)
    message (STATUS "using " ${PREFIX})
    add_definitions (${${PREFIX}_DEFINITIONS})
    include_directories (${${PREFIX}_INCLUDE_DIRS})
    link_directories (${${PREFIX}_LIBRARY_DIRS})
endmacro (use_package)

# link directories
macro (link_package PREFIX)
    if (${PREFIX}_DEBUG_LIBRARIES)
        foreach (releaselib_  ${${PREFIX}_LIBRARIES})
            target_link_libraries (${TARGET_NAME} optimized ${releaselib_})
        endforeach ()
        foreach (debuglib_ ${${PREFIX}_DEBUG_LIBRARIES})
            target_link_libraries (${TARGET_NAME} debug ${debuglib_})
        endforeach ()
    else ()
        target_link_libraries (${TARGET_NAME} ${${PREFIX}_LIBRARIES})
    endif ()
endmacro (link_package)

# Macro for using modules relative to the src directory
# Example: use_modules(TundraCore Plugins/UrhoRenderer)
macro(use_modules)
    set(moduleList "")
    foreach(moduleName_ ${ARGN})
        include_directories(${CMAKE_SOURCE_DIR}/src/${moduleName_})
        if (moduleList STREQUAL "")
            set(moduleList ${moduleName_})
        else()
            set(moduleList "${moduleList}, ${moduleName_}")
        endif()
    endforeach()
    message(STATUS "using modules: " ${moduleList})
endmacro()

# include local module libraries
macro (link_modules)
    foreach (module_ ${ARGN})
        get_filename_component(libraryName ${module_} NAME)
        target_link_libraries (${TARGET_NAME} ${libraryName})
    endforeach ()
endmacro (link_modules)

# manually find the debug libraries
macro (find_debug_libraries PREFIX DEBUG_POSTFIX)
    foreach (lib_ ${${PREFIX}_LIBRARIES})
        set (${PREFIX}_DEBUG_LIBRARIES ${${PREFIX}_DEBUG_LIBRARIES}
            ${lib_}${DEBUG_POSTFIX})
    endforeach ()
endmacro ()

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
    # TODO PCH for other platforms/compilers
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

# Convenience macro for including all TundraCore subfolders.
macro(UseTundraCore)
    include_directories(${CMAKE_SOURCE_DIR}/src/TundraCore/
        ${CMAKE_SOURCE_DIR}/src/TundraCore/Asset
        ${CMAKE_SOURCE_DIR}/src/TundraCore/Framework
        ${CMAKE_SOURCE_DIR}/src/TundraCore/Input
        ${CMAKE_SOURCE_DIR}/src/TundraCore/JSON
        ${CMAKE_SOURCE_DIR}/src/TundraCore/Scene
        ${CMAKE_SOURCE_DIR}/src/TundraCore/Console
        ${CMAKE_SOURCE_DIR}/src/TundraCore/Debug
        ${CMAKE_SOURCE_DIR}/src/TundraCore/Signals
        ${CMAKE_SOURCE_DIR}/src/TundraCore/Script
    )
    # Due to automatic CMake dependency transfer, we will need kNet and MathGeoLib link directories
    # Urho3D include directories are also needed throughout the project
    use_package(URHO3D)
    use_package(MATHGEOLIB)
    use_package(KNET)
endmacro()
