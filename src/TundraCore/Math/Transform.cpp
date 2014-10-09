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
    if (*str == '(')
        ++str;
    Transform t;
    t.pos.x = (float)strtod(str, const_cast<char**>(&str));
    if (*str == ',' || *str == ';')
        ++str;
    t.pos.y = (float)strtod(str, const_cast<char**>(&str));
    if (*str == ',' || *str == ';')
        ++str;
    t.pos.z = (float)strtod(str, const_cast<char**>(&str));
    if (*str == ',' || *str == ';')
        ++str;
    t.rot.x = (float)strtod(str, const_cast<char**>(&str));
    if (*str == ',' || *str == ';')
        ++str;
    t.rot.y = (float)strtod(str, const_cast<char**>(&str));
    if (*str == ',' || *str == ';')
        ++str;
    t.rot.z = (float)strtod(str, const_cast<char**>(&str));
    if (*str == ',' || *str == ';')
        ++str;
    t.scale.x = (float)strtod(str, const_cast<char**>(&str));
    if (*str == ',' || *str == ';')
        ++str;
    t.scale.y = (float)strtod(str, const_cast<char**>(&str));
    if (*str == ',' || *str == ';')
        ++str;
    t.scale.z = (float)strtod(str, const_cast<char**>(&str));
    if (*str == ',' || *str == ';')
        ++str;
    return t;
}

}
