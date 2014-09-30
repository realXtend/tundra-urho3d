// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

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

// Urho3D often used types
namespace Urho3D
{
    class String;
    class Variant;
    template <class T> class SharedPtr;
    template <class T> class WeakPtr;
    template <class T> class Vector;
    template <class T, class U> class HashMap;
    template <class T> class List;
    template <class T, class U> class Pair;
    
}

namespace Tundra
{

using Urho3D::String;
using Urho3D::Variant;
using Urho3D::SharedPtr;
using Urho3D::WeakPtr;
using Urho3D::Vector;
using Urho3D::HashMap;
using Urho3D::List;
using Urho3D::Pair;

}

