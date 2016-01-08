// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#if defined (_WINDOWS)
#if defined(JAVASCRIPT_EXPORTS) 
#define JAVASCRIPT_API __declspec(dllexport)
#else
#define JAVASCRIPT_API __declspec(dllimport) 
#endif
#else
#define JAVASCRIPT_API
#endif

