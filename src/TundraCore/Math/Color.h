// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"
#include "Math/float4.h"

#include <Urho3D/Math/Color.h>

namespace Tundra
{

/// A 4-component color value, component values are floating-points [0.0, 1.0].
/** Can be used as an entity-component attribute. */
class TUNDRACORE_API Color
{
public:
    float r; ///< Red component
    float g; ///< Green component
    float b; ///< Blue component
    float a; ///< Alpha component

    /// The default ctor initializes values to 0.f, 0.f, 0.f, 1.f (opaque black).
    Color() : r(0.0f), g(0.0f), b(0.0f), a(1.0f) {}
    /// Alpha is initialized to 1.
    Color(float nr, float ng, float nb) : r(nr), g(ng), b(nb), a(1.0f) {}
    Color(float nr, float ng, float nb, float na) : r(nr), g(ng), b(nb), a(na) {}
    Color(const Urho3D::Color &c) : r(c.r_), g(c.g_), b(c.b_), a(c.a_) {}
    Color(const float4 &c) : r(c.x), g(c.y), b(c.z), a(c.w) {}

    /// @todo Bring back if needed, but use Equals().
    /// bool operator == (const Color& rhs) const { return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a; }
    /// bool operator != (const Color& rhs) const { return !(*this == rhs); }

    /// Returns true if this vector is equal to the given vector, up to given per-element epsilon.
    bool Equals(const Color &other, float epsilon = 1e-3f) const;

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
    /** @todo "r g b a" instead so that this would be consistent with float2/3/4/Quat etc.
        @todo Apparently MGL has switched to using ',' instead of ' '  as separator. Which one we want to use? */
    String SerializeToString() const;

    /// Returns "Color(r, g, b, a)".
    String SerializeToCodeString() const;

    /// Returns "(r, g, b, a)" (limited precision).
    String ToString() const;

    /// Set component values.
    void Set(float r_, float g_, float b_) { r = r_; g = g_; b = b_; }

    /// Implicit conversion to float4.
    operator float4() const { return float4(r, g, b, a); }
    /// Explicit conversion to float4.
    float4 ToFloat4() const { return static_cast<float4>(*this); }

    /// Implicit conversion to Urho3D::Color.
    operator Urho3D::Color() const { return Urho3D::Color(r, g, b, a);  }
    /// Explicit conversion to Urho3D::Color.
    Urho3D::Color ToUrhoColor() const { return static_cast<Urho3D::Color>(*this); }

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
