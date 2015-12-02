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

typedef WeakPtr<Placeable> PlaceableWeakPtr;

/// Makes the entity a light source.
/** <table class="header">
    <tr>
    <td>
    <h2>EnvironmentLight</h2>
    Gives an access to scene-related environment settings, such as sunlight and ambient light.


    <b>Attributes</b>:
    <ul>
    <li> Color : sunColor.
    <div> @copydoc sunColor </div>
    <li> Color : ambientColor
    <div> @copydoc ambientColor </div>
    <li> float3 : sunDirection
    <div> @copydoc sunDirection </div>
    <li> bool : sunCastShadows
    <div> @copydoc sunCastShadows </div>
    <li> float : brightness
    <div> @copydoc brightness </div>
    </ul>

    <b>Exposes the following scriptable functions:</b>
    <ul>
    <li> None.
    </ul>

    </td>
    </tr>

    Does not emit any actions.

    </table> */
class URHORENDERER_API EnvironmentLight : public IComponent
{
    COMPONENT_NAME(EnvironmentLight, 8)

public:
    /// @cond PRIVATE
    /// Do not directly allocate new components using operator new, but use the factory-based SceneAPI::CreateComponent functions instead.
    explicit EnvironmentLight(Urho3D::Context* context, Scene* scene);
    /// @endcond
    virtual ~EnvironmentLight();

    /// Defines sun (diffuse) color.
    Attribute<Color> sunColor;

    /// Defines ambient light color.
    Attribute<Color> ambientColor;

    /// Defines sun light direction.
    Attribute<float3> sunDirection;

    /// Do we want the sunlight to cast shadows.
    Attribute<bool> sunCastShadows;

    /// Sunlight brightness multiplier.
    Attribute<float> brightness;

private:
    /// Called when the parent entity has been set.
    void UpdateSignals();

    void AttributesChanged() override;

    /// World ptr
    GraphicsWorldWeakPtr world_;

    /// Urho scene node
    SharedPtr<Urho3D::Node> node_;
    /// Urho light
    SharedPtr<Urho3D::Light> light_;
};

COMPONENT_TYPEDEFS(EnvironmentLight)

}