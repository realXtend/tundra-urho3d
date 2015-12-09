# Define target name and output directory
init_target(AvatarApplication OUTPUT Plugins)

# Define source files
file(GLOB CPP_FILES *.cpp)
file(GLOB H_FILES *.h)

set (SOURCE_FILES ${CPP_FILES} ${H_FILES})

UseTundraCore()
use_modules(TundraCore Plugins/UrhoRenderer Plugins/TundraLogic Plugins/CameraApplication)

add_definitions(-DAVATAR_EXPORTS)
use_package(BULLET)

build_library(${TARGET_NAME} SHARED ${SOURCE_FILES})

link_modules(TundraCore Plugins/UrhoRenderer Plugins/TundraLogic Plugins/CameraApplication)
link_package(URHO3D)
link_package(MATHGEOLIB)

SetupCompileFlagsWithPCH()

final_target()
