# Define target name and output directory
init_target(UrhoRenderer OUTPUT Plugins)

# Define source files
file(GLOB CPP_FILES *.cpp)
file(GLOB H_FILES *.h)

AddSourceFolder(Ogre)
AddSourceFolder(UrhoRendererBindings)

set (SOURCE_FILES ${CPP_FILES} ${H_FILES})

add_definitions(-DURHORENDERER_EXPORTS)

UseTundraCore()
use_modules(TundraCore Plugins/JavaScript)

build_library(${TARGET_NAME} SHARED ${SOURCE_FILES})

link_modules(TundraCore JavaScript)
link_package(URHO3D)
link_package(MATHGEOLIB)

SetupCompileFlagsWithPCH()

final_target()
