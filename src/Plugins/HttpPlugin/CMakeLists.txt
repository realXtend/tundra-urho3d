
macro(configure_curl)
    if ("${CURL_HOME}" STREQUAL "")
        message(FATAL_ERROR "CURL_HOME not set")
    endif()
    set(CURL_INCLUDE_DIRS ${CURL_HOME}/include)
    set(CURL_LIBRARY_DIRS ${CURL_HOME}/lib)
    if (MSVC)
        set(CURL_LIBRARIES optimized libcurl debug libcurl_d general libeay32 ssleay32)
    elseif (ANDROID)
        set(CURL_LIBRARIES curl ssl crypto z)
    else()
        set(CURL_LIBRARIES curl polarssl)
    endif()
endmacro (configure_curl)

###############################################################################

configure_curl()

# Define target name and output directory
init_target(HttpPlugin OUTPUT Plugins)

# Define source files
file(GLOB CPP_FILES *.cpp)
file(GLOB H_FILES *.h)
file(GLOB C_FILES *.c)

AddSourceFolder(HttpAsset)
AddSourceFolder(HttpParser)

set(SOURCE_FILES ${CPP_FILES} ${C_FILES} ${H_FILES})

add_definitions(-DTUNDRA_HTTP_EXPORTS -DCURL_STATICLIB)

UseTundraCore()
use_modules(TundraCore)
use_package(CURL)

build_library(${TARGET_NAME} SHARED ${SOURCE_FILES})

link_modules(TundraCore)
link_package(URHO3D)
link_package(CURL)
if (WIN32)
    target_link_libraries(${TARGET_NAME} ws2_32.lib)
endif()

SetupCompileFlagsWithPCH()

final_target()
