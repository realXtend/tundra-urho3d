// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "Fog.h"
#include "GraphicsWorld.h"
#include "AttributeMetadata.h"
#include "Framework.h"
#include "Scene/Scene.h"
#include "LoggingFunctions.h"

#include <Urho3D/Graphics/GraphicsDefs.h>
#include <Urho3D/Graphics/Zone.h>

namespace Tundra
{

Fog::Fog(Urho3D::Context* context, Scene* scene) :
    IComponent(context, scene),
    INIT_ATTRIBUTE_VALUE(mode, "Mode", 3),
    INIT_ATTRIBUTE_VALUE(color,"Color", Color(0.707792f, 0.770537f, 0.831373f, 1.f)),
    INIT_ATTRIBUTE_VALUE(startDistance, "Start distance", 100.f),
    INIT_ATTRIBUTE_VALUE(endDistance, "End distance", 2000.f),
    INIT_ATTRIBUTE_VALUE(expDensity, "Exponential density", 0.001f)
{
    static AttributeMetadata metadata;
    static bool metadataInitialized = false;
    if (!metadataInitialized)
    {
        metadata.enums[Fog::None] = "NoFog";
        metadata.enums[Fog::Exponentially] = "Exponentially";
        metadata.enums[Fog::ExponentiallySquare] = "ExponentiallySquare";
        metadata.enums[Fog::Linear] = "Linearly";
        metadataInitialized = true;
    }
    mode.SetMetadata(&metadata);

    ParentEntitySet.Connect(this, &Fog::UpdateSignals);
}

Fog::~Fog()
{
}

void Fog::UpdateSignals()
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
        Urho3D::Zone* zone = world_->UrhoZone();
        if (zone)
        {
            zone->SetFogColor(color.Get());
            zone->SetFogStart(startDistance.Get());
            zone->SetFogEnd(endDistance.Get());
        }
    }
}

void Fog::AttributesChanged()
{
    Urho3D::Zone* zone = 0;
    if (world_)
        zone = world_->UrhoZone();

    if (!ViewEnabled() || !zone)
        return;

    if (color.ValueChanged())
        zone->SetFogColor(color.Get());

    if (startDistance.ValueChanged())
        zone->SetFogStart(startDistance.Get());

    if (endDistance.ValueChanged())
        zone->SetFogEnd(endDistance.Get());

    ///\todo Support for exponential fog
}

}
