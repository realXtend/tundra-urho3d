// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#if defined (_WINDOWS)
#if defined(URHO_MODULE_EXPORTS) 
#define URHO_MODULE_API __declspec(dllexport)
#else
#define URHO_MODULE_API __declspec(dllimport) 
#endif
#else
#define URHO_MODULE_API
#endif

