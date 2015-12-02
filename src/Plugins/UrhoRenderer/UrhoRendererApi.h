// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#if defined (_WINDOWS)
#if defined(URHORENDERER_EXPORTS) 
#define URHORENDERER_API __declspec(dllexport)
#else
#define URHORENDERER_API __declspec(dllimport) 
#endif
#else
#define URHORENDERER_API
#endif

