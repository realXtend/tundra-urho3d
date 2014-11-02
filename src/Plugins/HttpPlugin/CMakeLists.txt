
configure_curl()

# Define target name and output directory
init_target(HttpPlugin OUTPUT Plugins)

# Define source files
file(GLOB CPP_FILES *.cpp)
file(GLOB H_FILES *.h)

AddSourceFolder(HttpAsset)
AddSourceFolder(HttpParser)

set (SOURCE_FILES ${CPP_FILES} ${H_FILES})

add_definitions(-DTUNDRA_HTTP_EXPORTS -DCURL_STATICLIB)

UseTundraCore()
use_modules(TundraCore)
use_package(CURL)

build_library(${TARGET_NAME} SHARED ${SOURCE_FILES})

link_modules(TundraCore)
link_package(CURL)

SetupCompileFlags()

final_target()
