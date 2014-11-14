// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#if defined (_WINDOWS)
#if defined(TUNDRA_ZIP_EXPORTS) 
#define TUNDRA_ZIP_API __declspec(dllexport)
#else
#define TUNDRA_ZIP_API __declspec(dllimport) 
#endif
#else
#define TUNDRA_ZIP_API
#endif
