/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   Color.cpp
    @brief  A 4-component color value, component values are floating-points [0.0, 1.0]. */

#include "Colour.h"
#include "Math/MathFunc.h"
#include "Math/float4.h"

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
    if (*str == '(')
        ++str;
    Color c;
    c.r = (float)strtod(str, const_cast<char**>(&str));
    if (*str == ',' || *str == ';')
        ++str;
    c.g = (float)strtod(str, const_cast<char**>(&str));
    if (*str == ',' || *str == ';')
        ++str;
    c.b = (float)strtod(str, const_cast<char**>(&str));
    if (*str == ',' || *str == ';')
        ++str;
    if (str && *str != '\0') // alpha optional
        c.a = (float)strtod(str, const_cast<char**>(&str));
    return c;
}

Color::operator float4() const
{
    return float4(r, g, b, a);
}

float4 Color::ToFloat4() const
{
    return (float4)*this;
}

}

