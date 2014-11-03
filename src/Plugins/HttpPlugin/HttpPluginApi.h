// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#if defined (_WINDOWS)
#if defined(TUNDRA_HTTP_EXPORTS) 
#define TUNDRA_HTTP_API __declspec(dllexport)
#else
#define TUNDRA_HTTP_API __declspec(dllimport) 
#endif
#else
#define TUNDRA_HTTP_API
#endif
