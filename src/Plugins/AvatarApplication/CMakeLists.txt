# Define target name and output directory
init_target(AvatarApplication OUTPUT Plugins)

# Define source files
file(GLOB CPP_FILES *.cpp)
file(GLOB H_FILES *.h)

AddSourceFolder(AvatarApplicationBindings)

set (SOURCE_FILES ${CPP_FILES} ${H_FILES})

UseTundraCore()
use_modules(TundraCore Plugins/UrhoRenderer Plugins/JavaScript)

add_definitions(-DAVATAR_EXPORTS)

build_library(${TARGET_NAME} SHARED ${SOURCE_FILES})

link_modules(TundraCore UrhoRenderer JavaScript)
link_package(URHO3D)
link_package(MATHGEOLIB)

SetupCompileFlagsWithPCH()

final_target()
