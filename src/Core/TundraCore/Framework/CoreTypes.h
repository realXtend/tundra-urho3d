// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

// types

#include <vector>
#include <list>

#if defined(unix) || defined(__APPLE__)
#include <cmath>
#include <limits>
// Gnu GCC has C99-standard macros as an extension but in some system there does not exist them so we define them by ourself.
template <class T> inline bool _finite(const T &f) { return f != std::numeric_limits<T>::infinity(); }
template <class T> inline bool _isnan(const T &f) { return f != f; }
#endif

// Use kNet/Types.h for fixed-width types instead of duplicating the code here.
#include <kNet/Types.h>

// Floating-point types
typedef float f32;
typedef double f64;

// Unsigned types
typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned long ulong;

// Special Tundra identifiers
typedef unsigned int entity_id_t;
typedef unsigned int component_id_t;

