// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "Math/float4.h"

namespace Tundra
{

/// A 4-component color value, component values are floating-points [0.0, 1.0].
class Color
{
public:
    float r; ///< Red component
    float g; ///< Green component
    float b; ///< Blue component
    float a; ///< Alpha component

    /// The default ctor initializes values to 0.f, 0.f, 0.f, 1.f (opaque black).
    Color() : r(0.0f), g(0.0f), b(0.0f), a(1.0f)
    {
    }

    Color(float nr, float ng, float nb) : r(nr), g(ng), b(nb), a(1.0f)
    {
    }

    Color(float nr, float ng, float nb, float na) : r(nr), g(ng), b(nb), a(na)
    {
    }

    bool operator == (const Color& rhs) const
    {
        return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a; /**< @todo Use epsilon! */
    }

    bool operator != (const Color& rhs) const
    {
        return !(*this == rhs);
    }

    Color operator * (float scalar) const
    {
        return Color(r*scalar, g*scalar, b*scalar, a*scalar);
    }

    Color& operator *= (float scalar)
    {
        r *= scalar;
        g *= scalar;
        b *= scalar;
        a *= scalar;

        return *this;
    }

    /// Parses a string to a new Color.
    /** Accepted formats are: "r,g,b,a" or "(r,g,b,a)" or "(r;g;b;a)" or "r g b" or "r,g,b" or "(r,g,b)" or "(r;g;b)" or "r g b" .
        @sa SerializeToString */
    static Color FromString(const char *str);

    /// Serialize to a string in the format "r,g,b,a"
    String SerializeToString() const;

    /// Implicit conversion to float4.
    operator float4() const;

    /// Returns Color as a float4.
    float4 ToFloat4() const;

    static const Color Red; ///< (1, 0, 0, 1)
    static const Color Green; ///< (0, 1, 0, 1)
    static const Color Blue; ///< (0, 0, 1, 1)
    static const Color White; ///< (1, 1, 1, 1)
    static const Color Black; ///< (0, 0, 0, 1)
    static const Color Yellow; ///< (1, 1, 0, 1)
    static const Color Cyan; ///< (0, 1, 1, 1)
    static const Color Magenta; ///< (1, 0, 1, 1)
    static const Color Gray; ///< (0.5, 0.5, 0.5, 1)
};

}
