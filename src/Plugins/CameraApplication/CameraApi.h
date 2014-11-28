// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#if defined (_WINDOWS)
    #if defined(CAMERA_EXPORTS)
        #define CAMERA_API __declspec(dllexport)
    #else
        #define CAMERA_API __declspec(dllimport)
    #endif
#else
    #define CAMERA_API
#endif

