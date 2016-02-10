// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#if defined (_WINDOWS)
    #if defined(ECEDITOR_EXPORTS)
        #define ECEDITOR_API __declspec(dllexport)
    #else
        #define ECEDITOR_API __declspec(dllimport)
    #endif
#else
    #define ECEDITOR_API
#endif

