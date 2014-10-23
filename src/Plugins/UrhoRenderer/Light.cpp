// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "Light.h"
#include "GraphicsWorld.h"
#include "AttributeMetadata.h"
#include "Placeable.h"
#include "UrhoRenderer.h"
#include "Scene/Scene.h"
#include "Math/MathUtilities.h"
#include "LoggingFunctions.h"

#include <Engine/Scene/Node.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Graphics/Graphics.h>
#include <Engine/Graphics/Light.h>

namespace Tundra
{

Light::Light(Urho3D::Context* context, Scene* scene) :
    IComponent(context, scene),
    INIT_ATTRIBUTE_VALUE(type, "Type", PointLight),
    INIT_ATTRIBUTE_VALUE(diffColor, "Diffuse color", Color::White),
    INIT_ATTRIBUTE_VALUE(specColor, "Specular color", Color::Black),
    INIT_ATTRIBUTE_VALUE(castShadows, "Cast shadows", false),
    INIT_ATTRIBUTE_VALUE(range, "Range", 25),
    INIT_ATTRIBUTE_VALUE(brightness, "Brightness", 1.0f),
    INIT_ATTRIBUTE_VALUE(constAtten, "Constant atten", 0.0f), /**< @todo "Constant attennuation" */
    INIT_ATTRIBUTE_VALUE(linearAtten, "Linear atten", 0.01f), /**< @todo "Linear attennuation" */
    INIT_ATTRIBUTE_VALUE(quadraAtten, "Quadratic atten", 0.01f), /**< @todo "Quadratic attennuation" */
    INIT_ATTRIBUTE_VALUE(innerAngle, "Light inner angle", 30.0f),
    INIT_ATTRIBUTE_VALUE(outerAngle, "Light outer angle", 40.0f)
{
    static AttributeMetadata typeAttrData;
    static bool metadataInitialized = false;
    if(!metadataInitialized)
    {
        typeAttrData.enums[PointLight] = "Point";
        typeAttrData.enums[Spotlight] = "Spot";
        typeAttrData.enums[DirectionalLight] = "Directional";
        metadataInitialized = true;
    }
    type.SetMetadata(&typeAttrData);

    ParentEntitySet.Connect(this, &Light::UpdateSignals);
}

Light::~Light()
{
    if (world_.Expired())
    {
        if (light_)
            LogError("Light: World has expired, skipping uninitialization!");
        return;
    }

    DetachLight();

    if (light_)
        light_.Reset();
}

void Light::UpdateSignals()
{
    if (!ViewEnabled())
        return;

    Entity* parent = ParentEntity();
    if (!parent)
        return;

    parent->ComponentAdded.Connect(this, &Light::OnComponentStructureChanged);
    parent->ComponentRemoved.Connect(this, &Light::OnComponentStructureChanged);

    if (parent->ParentScene())
        world_ = parent->ParentScene()->Subsystem<GraphicsWorld>();

    // Make sure we attach to the Placeable if exists.
    AttachLight();
}

void Light::OnComponentStructureChanged(IComponent*, AttributeChange::Type)
{
    // No-op if attached to the same placeable already
    if (placeable_ == parentEntity->Component<Placeable>())
        return;

    AttachLight(); // Try to attach if placeable is present, otherwise detach
}

void Light::AttributesChanged()
{
    if (!ViewEnabled() || !light_)
        return;

    if (type.ValueChanged())
    {
        int tundraType = type.Get();
        Urho3D::LightType urhoType = Urho3D::LIGHT_DIRECTIONAL;
        if (tundraType == PointLight)
            urhoType = Urho3D::LIGHT_POINT;
        else if (tundraType == Spotlight)
            urhoType = Urho3D::LIGHT_SPOT;
        light_->SetLightType(urhoType);
    }
    if (diffColor.ValueChanged())
        light_->SetColor(diffColor.Get());
    // Specular color value ignored, only use intensity
    if (specColor.ValueChanged())
        light_->SetSpecularIntensity(specColor.Get().ToFloat4().AverageOfElements());
    if (castShadows.ValueChanged())
        light_->SetCastShadows(castShadows.Get());
    if (range.ValueChanged())
        light_->SetRange(range.Get());
    if (brightness.ValueChanged())
        light_->SetBrightness(brightness.Get());
}

void Light::AttachLight()
{
    if (world_.Expired())
        return;

    // Detach first, in case the original placeable no longer exists
    DetachLight();

    Entity *entity = ParentEntity();
    if (!entity)
        return;

    placeable_ = entity->Component<Placeable>();
    if (!placeable_)
        return;

    Urho3D::Node* placeableNode = placeable_->UrhoSceneNode();
    if (!placeableNode)
    {
        LogError("Can not attach light: placeable does not have an Urho3D scene node");
        return;
    }
    
    light_ = placeableNode->CreateComponent<Urho3D::Light>();

    int tundraType = type.Get();
    Urho3D::LightType urhoType = Urho3D::LIGHT_DIRECTIONAL;
    if (tundraType == PointLight)
        urhoType = Urho3D::LIGHT_POINT;
    else if (tundraType == Spotlight)
        urhoType = Urho3D::LIGHT_SPOT;
    light_->SetLightType(urhoType);
    light_->SetColor(diffColor.Get());
    light_->SetSpecularIntensity(specColor.Get().ToFloat4().AverageOfElements());
    light_->SetCastShadows(castShadows.Get());
    light_->SetRange(range.Get());
    light_->SetBrightness(brightness.Get());
}

void Light::DetachLight()
{
    if (!light_ || world_.Expired())
        return;

    if (placeable_)
    {
        Urho3D::Node* placeableNode = placeable_->UrhoSceneNode();
        if (!placeableNode)
        {
            LogError("Can not detach light: placeable does not have an Urho3D scene node");
            return;
        }
        placeableNode->RemoveComponent(light_);
        light_.Reset();
        placeable_.Reset();
    }
}

}
