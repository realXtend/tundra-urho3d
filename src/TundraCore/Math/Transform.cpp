/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   Transform.cpp
    @brief  Describes transformation of an object in 3D space. */

#include "StableHeaders.h"

#include "Transform.h"

#include "Math/MathFunc.h"

#include <stdio.h>
#include <stdlib.h>

namespace Tundra
{

Transform::operator String() const
{
    return "Transform(Pos:(" + String(pos.x) + "," + String(pos.y) + "," + String(pos.z) + ") Rot:(" +
        String(rot.x) + "," + String(rot.y) + "," + String(rot.z) + ") Scale:(" +
        String(scale.x) + "," + String(scale.y) + "," + String(scale.z) + "))";
}

String Transform::SerializeToString() const
{
    char str[256];
    sprintf(str, "%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g", pos.x, pos.y, pos.z, rot.x, rot.y, rot.z, scale.x, scale.y, scale.z);
    return str;
}

Transform Transform::FromString(const char *str)
{
    assume(str);
    if (!str)
        return Transform();
    MATH_SKIP_WORD(str, "Transform");
    MATH_SKIP_WORD(str, "(");
    Transform t;
    // Use DeserializeFloat() instead of duplicating its code but comply 
    // with the old strtod behavior where 0 is used on conversion failure.
    t.pos.x = DeserializeFloat(str, &str); if (IsNan(t.pos.x)) t.pos.x = 0.f;
    t.pos.y = DeserializeFloat(str, &str); if (IsNan(t.pos.y)) t.pos.y = 0.f;
    t.pos.z = DeserializeFloat(str, &str); if (IsNan(t.pos.z)) t.pos.z = 0.f;
    t.rot.x = DeserializeFloat(str, &str); if (IsNan(t.rot.x)) t.rot.x = 0.f;
    t.rot.y = DeserializeFloat(str, &str); if (IsNan(t.rot.y)) t.rot.y = 0.f;
    t.rot.z = DeserializeFloat(str, &str); if (IsNan(t.rot.z)) t.rot.z = 0.f;
    t.scale.x = DeserializeFloat(str, &str); if (IsNan(t.scale.x)) t.scale.x = 0.f;
    t.scale.y = DeserializeFloat(str, &str); if (IsNan(t.scale.y)) t.scale.y = 0.f;
    t.scale.z = DeserializeFloat(str, &str); if (IsNan(t.scale.z)) t.scale.z = 0.f;
    return t;
}

}
