// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#if defined (_WINDOWS)
#if defined(BULLETPHYSICS_EXPORTS)
#define BULLETPHYSICS_API __declspec(dllexport)
#else
#define BULLETPHYSICS_API __declspec(dllimport)
#endif
#else
#define BULLETPHYSICS_API
#endif

