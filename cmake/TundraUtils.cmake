
# Start section. Prints a title that must be passed.
macro(BeginSection title)
    EndSection()
    string(TOUPPER ${title} title_upper)
    string(REGEX REPLACE "." "-" title_line ${title_upper})
    message("\n${title_upper}")
    message("${title_line}\n")
    set(_SECTION_ENDED FALSE)
endmacro()

# Ends a section. Calling this is optional, the previous
# section is automatically ended in BeginSection.
macro(EndSection)
    if (NOT _SECTION_ENDED)
        message(" ")
    endif()
    set(_SECTION_ENDED TRUE)
endmacro()

# Init section macro state
set(_SECTION_ENDED TRUE) 

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

# Lists sub directories
# Get sub directory names from directory of current CMakeLists.txt
#   GetSubDirectories(MY_RESULT_VAR)
# Get absolute directort paths
#   GetSubDirectories(MY_RESULT_VAR "/my/path", TRUE)
# Get absolute sub directory paths from directory of current CMakeLists.txt
#   GetSubDirectories(MY_RESULT_VAR ${CMAKE_CURRENT_SOURCE_DIR} TRUE)
macro (GetSubDirectories result)
    # If second param is not defined use currently being processed CMakeLists.txt directory
    if ("${ARGV1}" STREQUAL "")
        set(current_dir ${CMAKE_CURRENT_SOURCE_DIR})
    else()
        set(current_dir "${ARGV1}")
    endif()
    set (use_abs_path ${ARGV2})
    file (GLOB children RELATIVE ${current_dir} ${current_dir}/*)
    set (dirlist "")
    foreach (child ${children})
        if (IS_DIRECTORY ${current_dir}/${child})
            # Absolute path if extra argument is truthy
            if (use_abs_path)
                list (APPEND dirlist ${current_dir}/${child})
            else()
                list (APPEND dirlist ${child})
            endif ()
        endif()
    endforeach()
    set (${result} ${dirlist})
endmacro()