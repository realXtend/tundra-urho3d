# Define target name and output directory
if (WIN32)

init_target (TundraConsole OUTPUT ./)

# Define source files
file (GLOB CPP_FILES main.cpp)
file (GLOB H_FILES *.h)

set (SOURCE_FILES ${CPP_FILES} ${H_FILES})

SetupCompileFlags()

UseTundraCore()
use_core_modules(TundraCore)

build_executable(${TARGET_NAME} ${SOURCE_FILES} ${RESOURCE_FILES})

link_modules(TundraCore)

final_target ()

endif ()