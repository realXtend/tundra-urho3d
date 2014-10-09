// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

#include "EntityReference.h"
#include "Entity.h"
#include "Scene/Scene.h"
#include "Name.h"
#include "SceneAPI.h"

#include "Framework.h"
#include "IComponent.h"

#include <StringUtils.h>

namespace Tundra
{

void EntityReference::Set(EntityPtr entity)
{
    Set(entity.Get());
}

void EntityReference::Set(Entity* entity)
{
    if (!entity)
    {
        ref.Clear();
        return;
    }
    
    String name = entity->Name();
    Scene* scene = entity->ParentScene();
    if (!scene)
    {
        // If entity is not in scene, set directly by ID
        ref = String(entity->Id());
        return;
    }
    if (scene->IsUniqueName(name))
        ref = name;
    else
        ref = String(entity->Id());
}

bool EntityReference::IsEmpty() const
{
    String trim = ref.Trimmed();
    if (trim.Empty())
        return true;
    // 0 is not a valid Entity id
    if (trim[0] == '0')
        return true;
    return false;
}

EntityPtr EntityReference::Lookup(Scene* scene) const
{
    if (!scene || ref.Empty())
        return EntityPtr();
    // If ref looks like an ID, lookup by ID first
    entity_id_t id = Urho3D::ToInt(ref);
    if (id != 0)
    {
        EntityPtr entity = scene->EntityById(id);
        if (entity)
            return entity;
    }
    // Then get by name
    return scene->EntityByName(ref.Trimmed());
}

EntityPtr EntityReference::LookupParent(Entity* entity) const
{
    if (!entity)
        return EntityPtr();
    else if (ref.Empty())
        return entity->Parent();
    else return Lookup(entity->ParentScene());
}

bool EntityReference::Matches(Entity *entity) const
{
    if (!entity || ref.Empty())
        return false;
    // If ref looks like an ID, lookup by ID first
    entity_id_t id = Urho3D::ToInt(ref);
    if (id)
        return (entity->Id() == id);
    return (entity->Name().Compare(ref, false) == 0);
}

}
