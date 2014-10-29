// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

// Use kNet/Types.h for fixed-width types instead of duplicating the code here.
#include <kNet/Types.h>

// Urho3D foreach
#include <ForEach.h>

// Urho3D often used types
namespace Urho3D
{
    class RefCounted;
    class Object;
    class String;
    class StringHash;
    class Variant;
    template <class T> class SharedPtr;
    template <class T> class WeakPtr;
    template <class T> class Vector;
    template <class T> class PODVector;
    template <class T, class U> class HashMap;
    template <class T> class HashSet;
    template <class T> class List;
    template <class T, class U> class Pair;
}

namespace Tundra
{
    // Pull in often used Urho3D types.
    using Urho3D::RefCounted;
    using Urho3D::Object;
    using Urho3D::String;
    using Urho3D::StringHash;
    using Urho3D::Variant;
    using Urho3D::SharedPtr;
    using Urho3D::WeakPtr;
    using Urho3D::Vector;
    using Urho3D::PODVector;
    using Urho3D::HashMap;
    using Urho3D::HashSet;
    using Urho3D::List;
    using Urho3D::Pair;
    typedef Vector<String> StringVector;
    typedef Vector<Variant> VariantList;
    typedef HashMap<StringHash, Variant> VariantMap;

    // Floating-point types
    typedef float f32;
    typedef double f64;

    // Unsigned types
    typedef unsigned char uchar;
    typedef unsigned int uint;
    typedef unsigned short ushort;
    typedef unsigned long ulong;
    typedef unsigned long long ulonglong;

    // Special Tundra identifiers
    typedef unsigned int entity_id_t;
    typedef unsigned int component_id_t;
}

/// See http://urho3d.github.io/documentation/HEAD/annotated.html for Urho3D's class reference.
namespace Urho3D {}
/// Contains all of the core %Tundra features.
namespace Tundra {}
/// See http://clb.demon.fi/knet/annotated.html for kNet's class reference.
namespace kNet {}
