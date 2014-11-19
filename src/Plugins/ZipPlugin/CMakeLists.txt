
macro(configure_zziplib)
    if ("${ZZIPLIB_HOME}" STREQUAL "")
        # Make fatal once linux dep build is implemented
        message(FATAL_ERROR "ZZIPLIB_HOME not set")
    endif()
    if (MSVC OR ANDROID)
        set(ZZIPLIB_INCLUDE_DIRS ${ZZIPLIB_HOME})
        set(ZZIPLIB_LIBRARY_DIRS ${ZZIPLIB_HOME}/build)
    endif()
    if (MSVC)
        list(APPEND ZZIPLIB_LIBRARY_DIRS ${ZZIPLIB_HOME}/../zlib/build/lib)
        set(ZZIPLIB_LIBRARIES optimized zziplibstatic optimized zlibstatic debug zziplibstaticd debug zlibstaticd)
    elseif(ANDROID)
        set(ZZIPLIB_LIBRARIES zziplibstatic z)
    else()
        # Make sure to statically link to our dependency build
        # zlib is pretty much guaranteed to be picked from the system if not .a postfixed
        # and the exact directory given to ZZIPLIB_LIBRARY_DIRS
        set(ZZIPLIB_INCLUDE_DIRS ${ZZIPLIB_HOME}/include)
        set(ZZIPLIB_LIBRARY_DIRS ${ZZIPLIB_HOME}/lib ${ZZIPLIB_HOME}/../../zlib/build/lib)
        set(ZZIPLIB_LIBRARIES zzip.a z.a)
    endif()
endmacro (configure_zziplib)

###############################################################################

configure_zziplib()

# Define target name and output directory
init_target(ZipPlugin OUTPUT Plugins)

# Define source files
file(GLOB CPP_FILES *.cpp)
file(GLOB H_FILES *.h)

set(SOURCE_FILES ${CPP_FILES} ${H_FILES})

add_definitions(-DTUNDRA_ZIP_EXPORTS)

UseTundraCore()
use_package(ZZIPLIB)

build_library(${TARGET_NAME} SHARED ${SOURCE_FILES})

link_modules(TundraCore)
link_package(URHO3D)
link_package(ZZIPLIB)

SetupCompileFlagsWithPCH()

final_target()
