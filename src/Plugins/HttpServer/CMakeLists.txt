# Define target name and output directory
init_target (HttpServer OUTPUT Plugins)

configure_boost()

# Define source files
file (GLOB CPP_FILES *.cpp)
file (GLOB H_FILES *.h)

AddSourceFolder(HttpServerBindings)

set (SOURCE_FILES ${CPP_FILES} ${H_FILES})

add_definitions (-DHTTPSERVER_EXPORTS)

# Includes
UseTundraCore ()
use_modules(TundraCore Plugins/JavaScript)

if (WIN32)
include_directories(
    ${ENV_TUNDRA_DEP_PATH}/websocketpp
)
else()
include_directories(
    ${ENV_TUNDRA_DEP_PATH}/include
)
endif()

build_library (${TARGET_NAME} SHARED ${SOURCE_FILES})

# Linking
link_modules(TundraCore Plugins/JavaScript)
link_package(KNET)

if (WINDOWS)
    target_link_libraries (${TARGET_NAME}
        ws2_32.lib
    )
else()
    # Visual Studio uses library auto-linking, so this is only necessary for the other platforms.
    link_boost()
endif()

SetupCompileFlags()

final_target()
