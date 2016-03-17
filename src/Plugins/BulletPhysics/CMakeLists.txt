# Define target name and output directory
init_target(BulletPhysics OUTPUT Plugins)

# Define source files
file(GLOB CPP_FILES *.cpp)
file(GLOB H_FILES *.h)

AddSourceFolder(BulletPhysicsBindings)

set (SOURCE_FILES ${CPP_FILES} ${H_FILES})

add_definitions(-DBULLETPHYSICS_EXPORTS)

UseTundraCore()
use_modules(TundraCore Plugins/UrhoRenderer Plugins/JavaScript)
use_package(BULLET)

build_library(${TARGET_NAME} SHARED ${SOURCE_FILES})

link_modules(TundraCore UrhoRenderer JavaScript)
link_package(URHO3D)
link_package(BULLET)
link_package(MATHGEOLIB)

SetupCompileFlagsWithPCH()

final_target()
