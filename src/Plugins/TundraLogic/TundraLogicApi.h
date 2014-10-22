// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#if defined (_WINDOWS)
#if defined(TUNDRALOGIC_EXPORTS) 
#define TUNDRALOGIC_API __declspec(dllexport)
#else
#define TUNDRALOGIC_API __declspec(dllimport) 
#endif
#else
#define TUNDRALOGIC_API
#endif

