/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   Color.cpp
    @brief  A 4-component color value, component values are floating-points [0.0, 1.0]. */

#include "StableHeaders.h"
#include "Color.h"

#include <Math/MathFunc.h>

#include <stdio.h>
#include <stdlib.h>

namespace Tundra
{

const Color Color::Red = Color(1.f, 0.f, 0.f, 1.f);
const Color Color::Green = Color(0.f, 1.f, 0.f, 1.f);
const Color Color::Blue = Color(0.f, 0.f, 1.f, 1.f);
const Color Color::White = Color(1.f, 1.f, 1.f, 1.f);
const Color Color::Black = Color(0.f, 0.f, 0.f, 1.f);
const Color Color::Yellow = Color(1.f, 1.f, 0.f, 1.f);
const Color Color::Cyan = Color(0.f, 1.f, 1.f, 1.f);
const Color Color::Magenta = Color(1.f, 0.f, 1.f, 1.f);
const Color Color::Gray = Color(0.5f, 0.5f, 0.5f, 1.f);

Color Color::FromString(const char *str)
{
    assume(str);
    if (!str)
        return Color();
    MATH_SKIP_WORD(str, "Color");
    MATH_SKIP_WORD(str, "(");
    Color c;
    // Use DeserializeFloat() instead of duplicating its code but comply 
    // with the old strtod behavior where 0 is used on conversion failure.
    c.r = DeserializeFloat(str, &str); if (IsNan(c.r)) c.r = 0.f;
    c.g = DeserializeFloat(str, &str); if (IsNan(c.g)) c.g = 0.f;
    c.b = DeserializeFloat(str, &str); if (IsNan(c.b)) c.b = 0.f;
    if (str && *str != '\0') // alpha optional
    {
        c.a = DeserializeFloat(str, &str);
        if (IsNan(c.a)) c.a = 01.f;
    }
    return c;
}

String Color::SerializeToString() const
{
    char str[256];
    sprintf(str, "%.9g,%.9g,%.9g,%.9g", r, g, b, a);
    return str;
}

}
