// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "PlaceholderComponent.h"

#include "SceneAPI.h"

#include "Entity.h"
#include "Scene/Scene.h"

#include <Log.h>

namespace Tundra
{

PlaceholderComponent::PlaceholderComponent(Urho3D::Context* context, Scene* scene):
    IComponent(context, scene)
{
}

PlaceholderComponent::~PlaceholderComponent()
{
}

void PlaceholderComponent::SetTypeId(u32 newTypeId)
{
    typeId = newTypeId;
}

void PlaceholderComponent::SetTypeName(const String& newTypeName)
{
    typeName = newTypeName;
}

void PlaceholderComponent::DeserializeFromBinary(kNet::DataDeserializer& /*source*/, AttributeChange::Type /*change*/)
{
    LOGERROR("PlaceholderComponent can not support deserialization from binary");
}

IAttribute *PlaceholderComponent::CreateAttribute(const String &typeName, const String &id, const String &name, AttributeChange::Type /*change*/)
{
    // Check for duplicate
    if (AttributeById(id))
        return IComponent::AttributeById(id);
    if (AttributeByName(name))
        return IComponent::AttributeByName(name);

    IAttribute *attribute = SceneAPI::CreateAttribute(typeName, id);
    if (!attribute)
    {
        LOGERROR("Failed to create new attribute of type \"" + typeName + "\" with ID \"" + id + "\" to placeholder component \"" + GetName() + "\".");
        return 0;
    }

    attribute->SetName(name);
    IComponent::AddAttribute(attribute);
    // Placeholder components are supposed to be similar to static-structured components (attributes filled on construction), so do not signal attribute being added

    return attribute;
}

}
