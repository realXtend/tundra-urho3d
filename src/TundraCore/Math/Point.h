// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"
#include "CoreTypes.h"

namespace Tundra
{

/// Two-dimensional integer point.
/** Can be used as an entity-component attribute. */
class TUNDRACORE_API Point
{
public:
    int x;
    int y;

    /// The default ctor initializes values to 0,0 (origin)
    Point() : x(0), y(0)
    {
    }

    Point(int nx, int ny) : x(nx), y(ny)
    {
    }

    bool operator == (const Point& rhs) const
    {
        return x == rhs.x && y == rhs.y;
    }

    bool operator != (const Point& rhs) const
    {
        return !(*this == rhs);
    }

    /// Parses a string to a new Point.
    /** Accepted formats are: "x,y" or "(x,y)" or "x y".
        @sa SerializeToString */
    static Point FromString(const char *str);

    /// Serialize to a string in the format "x,y"
    String SerializeToString() const;
};

}
