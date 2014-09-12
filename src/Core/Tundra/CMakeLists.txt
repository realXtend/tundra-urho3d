# Define target name and output directory
init_target (Tundra OUTPUT ./)

# Define source files
file (GLOB CPP_FILES main.cpp)
file (GLOB H_FILES *.h)

set (SOURCE_FILES ${CPP_FILES} ${H_FILES})

SetupCompileFlags()

UseTundraCore()
use_core_modules(TundraCore)


if (MSVC)
   build_executable(${TARGET_NAME} WIN32 ${SOURCE_FILES} ${RESOURCE_FILES})
else()
   if (ANDROID)
       build_library(${TARGET_NAME} SHARED ${SOURCE_FILES})
   else()
       build_executable(${TARGET_NAME} ${SOURCE_FILES} ${RESOURCE_FILES})
   endif()
endif()

link_modules(TundraCore)

final_target ()

# Install files
setup_install_files (${TUNDRA_BIN}/../README.md)       # Readme
setup_install_files (${TUNDRA_BIN}/../LICENSE)         # Licence