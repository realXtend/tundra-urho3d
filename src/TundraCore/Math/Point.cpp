// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "Point.h"
#include "Math/MathFunc.h"

#include <stdio.h>
#include <stdlib.h>

namespace Tundra
{

Point Point::FromString(const char *str)
{
    assume(str);
    if (!str)
        return Point();
    if (*str == '(')
        ++str;
    Point p;
    p.x = strtol(str, const_cast<char**>(&str), 10);
    if (*str == ',' || *str == ';')
        ++str;
    p.y = strtol(str, const_cast<char**>(&str), 10);
    return p;
}

String Point::SerializeToString() const
{
    char str[256];
    sprintf(str, "%d,%d", x, y);
    return str;
}

}
