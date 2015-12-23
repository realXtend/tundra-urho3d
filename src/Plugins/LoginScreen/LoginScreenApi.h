// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#if defined (_WINDOWS)
    #if defined(LOGINSCREEN_EXPORTS)
        #define LOGINSCREEN_API __declspec(dllexport)
    #else
        #define LOGINSCREEN_API __declspec(dllimport)
    #endif
#else
    #define LOGINSCREEN_API
#endif

