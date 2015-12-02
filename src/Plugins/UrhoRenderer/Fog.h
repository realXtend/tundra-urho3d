// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IComponent.h"
#include "IAttribute.h"
#include "SceneFwd.h"
#include "UrhoRendererApi.h"
#include "UrhoRendererFwd.h"
#include "Math/Color.h"

namespace Tundra
{

/// Defines the overall fog settings for the whole scene.
/** <table class="header">
    <tr>
    <td>
    <h2>Fog</h2>

    Defines the overall fog settings for the whole scene, hence only one component per scene is applicable.
    Sets also the background color of the viewport same as the fog color.

    Registered by UrhoRenderer.

    <b>Attributes</b>:
    <ul>
    <li> int : mode
    <div> @copydoc mode </div>
    <li> Color : color
    <div> @copydoc color </div>
    <li> float : startDistance
    <div> @copydoc startDistance </div>
    <li> float : endDistance
    <div> @copydoc endDistance </div>
    <li> float : expDensity
    <div> @copydoc expDensity </div>
    </ul>
    </table> */
class URHORENDERER_API Fog : public IComponent
{
    COMPONENT_NAME(Fog, 9);

public:
     /// @cond PRIVATE
    /// Do not directly allocate new components using operator new, but use the factory-based SceneAPI::CreateComponent functions instead.
    explicit Fog(Urho3D::Context* context, Scene* scene);
    /// @endcond
    /// Sets fog to None.
    ~Fog();

    /// Fog modes, copied from Ogre::FogMode. Use when setting @c mode attribute.
    ///\todo Urho3D does not support exponential fog
    enum FogMode
    {
        None = 0, ///< No fog
        Exponentially, ///< (Not supported.) Fog density increases exponentially from the camera (fog = 1/e^(distance * density))
        ExponentiallySquare, ///< (Not supported.) Fog density increases at the square of Exponential, i.e. even quicker (fog = 1/e^(distance * density)^2)
        Linear ///< Fog density increases linearly between the start and end distances
    };

    /// Fog mode
    Attribute<int> mode;

    /// Fog color
    Attribute<Color> color;

    /// Fog start distance, Linear only.
    Attribute<float> startDistance;

    /// Fog end distance, Linear only.
    Attribute<float> endDistance;

    /// The density of the fog in Exponentially or ExponentiallySquare mode, as a value between 0 and 1. The default is 0.001.
    /// \todo not used
    Attribute<float> expDensity;

private:
    /// Called when the parent entity has been set.
    void UpdateSignals();

    void AttributesChanged() override;

    /// World ptr
    GraphicsWorldWeakPtr world_;
};

COMPONENT_TYPEDEFS(Fog)

}

