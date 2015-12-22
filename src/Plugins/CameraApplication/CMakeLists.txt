# Define target name and output directory
init_target(CameraApplication OUTPUT Plugins)

# Define source files
file(GLOB CPP_FILES *.cpp)
file(GLOB H_FILES *.h)

set (SOURCE_FILES ${CPP_FILES} ${H_FILES})

UseTundraCore()
use_modules(TundraCore Plugins/TundraLogic Plugins/UrhoRenderer)
use_package(BULLET)

add_definitions(-DCAMERA_EXPORTS)

build_library(${TARGET_NAME} SHARED ${SOURCE_FILES})

link_modules(TundraCore Plugins/TundraLogic Plugins/UrhoRenderer)
link_package(URHO3D)
link_package(MATHGEOLIB)

SetupCompileFlagsWithPCH()

final_target()
