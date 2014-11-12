// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "EnvironmentLight.h"
#include "GraphicsWorld.h"
#include "AttributeMetadata.h"
#include "Placeable.h"
#include "UrhoRenderer.h"
#include "Scene/Scene.h"
#include "LoggingFunctions.h"

#include <Engine/Scene/Node.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Graphics/GraphicsDefs.h>
#include <Engine/Graphics/Light.h>
#include <Engine/Graphics/Zone.h>

namespace Tundra
{

EnvironmentLight::EnvironmentLight(Urho3D::Context* context, Scene* scene) :
    IComponent(context, scene),
    INIT_ATTRIBUTE_VALUE(sunColor, "Sunlight color", Color(0.639f,0.639f,0.639f)),
    INIT_ATTRIBUTE_VALUE(ambientColor, "Ambient light color", GraphicsWorld::DefaultSceneAmbientLightColor()),
    INIT_ATTRIBUTE_VALUE(sunDirection, "Sunlight direction vector", float3(-1.f, -1.f, -1.f)),
    INIT_ATTRIBUTE_VALUE(sunCastShadows, "Sunlight cast shadows", true),
    INIT_ATTRIBUTE_VALUE(brightness, "Brightness", 1.0f)
{
    ParentEntitySet.Connect(this, &EnvironmentLight::UpdateSignals);
}

EnvironmentLight::~EnvironmentLight()
{
    if (world_.Expired())
    {
        if (light_)
            LogError("EnvironmentLight: World has expired, skipping uninitialization!");
        return;
    }

    if (node_)
        node_->Remove();
    light_.Reset();
    node_.Reset();
}

void EnvironmentLight::UpdateSignals()
{
    if (!ViewEnabled())
        return;

    Entity* parent = ParentEntity();
    if (!parent)
        return;

    if (parent->ParentScene())
        world_ = parent->ParentScene()->Subsystem<GraphicsWorld>();

    if (world_)
    {
        Urho3D::Scene* urhoScene = world_->UrhoScene();
        node_ = urhoScene->CreateChild("EnvironmentLight");
        light_ = node_->CreateComponent<Urho3D::Light>();
        light_->SetLightType(Urho3D::LIGHT_DIRECTIONAL);
        
        Urho3D::Zone* zone = world_->UrhoZone();
        if (zone)
            zone->SetAmbientColor(ambientColor.Get());

        node_->SetDirection(sunDirection.Get());
        light_->SetColor(sunColor.Get());
        light_->SetBrightness(brightness.Get());
        light_->SetCastShadows(sunCastShadows.Get());
        // Setup basic shadow cascade for desktops
        #ifndef ANDROID
        light_->SetShadowBias(Urho3D::BiasParameters(0.00025f, 0.5f));
        // Set cascade splits at 10, 50 and 200 world units, fade shadows out at 80% of maximum shadow distance
        light_->SetShadowCascade(Urho3D::CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));
        light_->SetShadowBias(Urho3D::BiasParameters(0.00005f, 0.5f));
        #endif
    }
}

void EnvironmentLight::AttributesChanged()
{
    if (!ViewEnabled() || !light_)
        return;

    if (sunColor.ValueChanged())
        light_->SetColor(sunColor.Get());
    if (ambientColor.ValueChanged() && world_)
    {
        Urho3D::Zone* zone = world_->UrhoZone();
        if (zone)
            zone->SetAmbientColor(ambientColor.Get());
    }
    if (sunDirection.ValueChanged())
        node_->SetDirection(sunDirection.Get());
    if (sunCastShadows.ValueChanged())
        light_->SetCastShadows(sunCastShadows.Get());
    if (brightness.ValueChanged())
        light_->SetBrightness(brightness.Get());
}

}
