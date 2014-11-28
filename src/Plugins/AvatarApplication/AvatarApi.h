// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#if defined (_WINDOWS)
    #if defined(AVATAR_EXPORTS)
        #define AVATAR_API __declspec(dllexport)
    #else
        #define AVATAR_API __declspec(dllimport)
    #endif
#else
    #define AVATAR_API
#endif

