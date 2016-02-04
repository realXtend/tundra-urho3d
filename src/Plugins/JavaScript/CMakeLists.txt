# Define target name and output directory
init_target(JavaScript OUTPUT Plugins)

# Define source files
file(GLOB CPP_FILES *.cpp)
file(GLOB C_FILES *.c)
file(GLOB H_FILES *.h)

AddSourceFolder(MathBindings)

set (SOURCE_FILES ${CPP_FILES} ${C_FILES} ${H_FILES})

add_definitions(-DJAVASCRIPT_EXPORTS)
add_definitions(-DDUK_COMPILING_DUKTAPE)

# Specify TundraCore includes manually instead of the full set, as compiling Duktape would clash with the Windows includes
# due to Tundra's DebugAPI.h
include_directories(
    .
    ../../TundraCore/Framework
    ../../TundraCore/Asset
    ../../TundraCore/Scene
    ../../TundraCore/Script
    ../../TundraCore/Signals)
use_package(URHO3D)
use_package(MATHGEOLIB)
use_package(KNET)

use_modules(TundraCore)

build_library(${TARGET_NAME} SHARED ${SOURCE_FILES})

link_modules(TundraCore)
link_package(URHO3D)
link_package(MATHGEOLIB)

SetupCompileFlagsWithPCH()

final_target()
