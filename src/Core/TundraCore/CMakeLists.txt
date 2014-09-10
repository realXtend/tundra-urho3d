# Define target name and output directory
if (TUNDRACORE_SHARED)
    init_target(TundraCore OUTPUT ./)
else()
    init_target(TundraCore)
endif()

# Define source files
file(GLOB CPP_FILES *.cpp)
file(GLOB H_FILES *.h)

set(SOURCE_FILES ${CPP_FILES} ${H_FILES})

UseTundraCore() # Include all subfolders within the project
use_core_modules(Math)
use_package(QJSON)

add_definitions(-DTUNDRACORE_EXPORTS)

if (TUNDRACORE_SHARED)
    build_library(${TARGET_NAME} SHARED ${SOURCE_FILES})
else()
    build_library(${TARGET_NAME} STATIC ${SOURCE_FILES})
endif()

# link_modules(Math)

SetupCompileFlagsWithPCH()

final_target()

