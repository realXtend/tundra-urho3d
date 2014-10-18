// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IComponent.h"
#include "IAttribute.h"
#include "SceneFwd.h"
#include "UrhoModuleApi.h"
#include "UrhoModuleFwd.h"
#include "Math/Color.h"

namespace Tundra
{

typedef WeakPtr<Placeable> PlaceableWeakPtr;

/// Makes the entity a light source.
/** <table class="header">
    <tr>
    <td>
    <h2>Light</h2>
    Makes the entity a light source.

    <b>Attributes</b>:
    <ul>
    <li>enum: type
    <div> @copydoc type </div>
    <li>Color: diffColor
    <div> @copydoc diffColor </div>
    <li>Color: specColor
    <div> @copydoc specColor </div>
    <li>bool: castShadows
    <div> @copydoc castShadows </div>
    <li>float: range
    <div> @copydoc range </div>
    <li>float: brightness
    <div> @copydoc brightness </div>
    <li>float: constAtten
    <div> @copydoc constAtten </div>
    <li>float: linearAtten
    <div>@copydoc linearAtten </div>
    <li>float: quadraAtten
    <div> @copydoc quadraAtten </div>
    <li>float: innerAngle
    <div> @copydoc innerAngle </div>
    <li>float: outerAngle
    <div>@copydoc outerAngle</div>
    </ul>

    <b>Exposes the following scriptable functions:</b>
    <ul>
    <li> None.
    </ul>

    <b>Reacts on the following (Placeable) actions:</b>
    <ul>
    <li>"Hide": Disables the light from affecting the scene.
    <li>"Show": Enables the light in the scene.
    <li>"ToggleVisibility": Toggles between the enabled and disabled states.
    </ul>
    </td>
    </tr>

    Does not emit any actions.

    <b>Depends on the Placeable component. The position in the Placeable component specifies the position in the world space where this light is placed at.
    </table> */
class URHO_MODULE_API Light : public IComponent
{
    COMPONENT_NAME(Light, 16)

public:
    /// @cond PRIVATE
    /// Do not directly allocate new components using operator new, but use the factory-based SceneAPI::CreateComponent functions instead.
    explicit Light(Urho3D::Context* context, Scene* scene);
    /// @endcond
    virtual ~Light();

    /// light type enumeration
    enum Type
    {
        PointLight,
        Spotlight,
        DirectionalLight,
    };

    /// Light type, see Type.
    Attribute<int> type;
    
    /// Light diffuse color, specifies the color the light casts.
    Attribute<Color> diffColor;
    
    /// Light specular color, specifies the color of the reflections the light casts.
    Attribute<Color> specColor;

    /// If true, this light casts dynamically calculated shadows on the scene.
    Attribute<bool> castShadows;

    /// Specifies how far in world space units the light reaches.
    Attribute<float> range;
    
    /// Light brightness, specifies the numerator of the light attenuation equation.
    Attribute<float> brightness;
    
    /// Light constant attenuation, specifies the constant term of the light attenuation equation.
    Attribute<float> constAtten;
    
    /// Light linear attenuation, specifies the linear term of the light attenuation equation.
    Attribute<float> linearAtten;
    
    /// Light quadratic attenuation, specifies the quadratic term of the light attenuation equation.
    Attribute<float> quadraAtten;
    
    /// Specifies inner umbra angle of the light. Only applicable for spotlights.
    Attribute<float> innerAngle;

    /// Specifies outer penumbra angle of the light. Only applicable for spotlights.
    Attribute<float> outerAngle;

private:
    /// Called when the parent entity has been set.
    void UpdateSignals();

    /// Called when component has been added or removed from the parent entity. Checks the existence of the Placeable component, and attaches this camera to it.
    void OnComponentStructureChanged(IComponent*, AttributeChange::Type);

    void AttributesChanged() override;

    /// Sets placeable component
    /** set a null placeable to detach the camera, otherwise will attach
        @param placeable placeable component */
    void SetPlaceable(const ComponentPtr &placeable);

     /// Attaches light to placeable
    void AttachLight();

    /// Detaches light from placeable
    void DetachLight();

    /// placeable component 
    PlaceableWeakPtr placeable_;

    /// World ptr
    GraphicsWorldWeakPtr world_;

    /// Urho light
    SharedPtr<Urho3D::Light> light_;
};

COMPONENT_TYPEDEFS(Light)

}