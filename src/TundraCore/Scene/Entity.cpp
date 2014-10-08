// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

#include "Entity.h"
#include "Scene/Scene.h"
#include "Name.h"
#include "SceneAPI.h"

#include "Framework.h"
#include "IComponent.h"

#include <Log.h>
#include <XMLFile.h>
#include <ForEach.h>

#include <kNet/DataSerializer.h>
#include <kNet/DataDeserializer.h>
#include <Profiler.h>
#include <algorithm>

/// \todo Find out where these are getting defined
#undef min
#undef max

namespace Tundra
{

Entity::Entity(Framework* framework, entity_id_t id, bool temporary, Scene* scene) :
    Object(framework->GetContext()),
    framework_(framework),
    id_(id),
    scene_(scene),
    temporary_(temporary)
{
}

Entity::~Entity()
{
    // If components still alive, they become free-floating
    for (ComponentMap::ConstIterator i = components_.Begin(); i != components_.End(); ++i)
        i->second_->SetParentEntity(0);
   
    components_.Clear();
}

void Entity::ChangeComponentId(component_id_t old_id, component_id_t new_id)
{
    if (old_id == new_id)
        return;
    
    ComponentPtr old_comp = ComponentById(old_id);
    if (!old_comp)
        return;
    
    if (ComponentById(new_id))
    {
        LOGWARNING("Purged component " + String(new_id) + " to make room for a ChangeComponentId request. This should not happen.");
        RemoveComponentById(new_id, AttributeChange::LocalOnly);
    }
    
    old_comp->SetNewId(new_id);
    components_.Erase(old_id);
    components_[new_id] = old_comp;
}

void Entity::AddComponent(const ComponentPtr &component, AttributeChange::Type change)
{
    AddComponent(0, component, change);
}

void Entity::AddComponent(component_id_t id, const ComponentPtr &component, AttributeChange::Type change)
{
    // Must exist and be free
    if (component && component->ParentEntity() == 0)
    {
        if (!id)
        {
            bool authority = true;
            if (scene_)
                authority = scene_->IsAuthority();
            // Loop until we find a free ID
            for (;;)
            {
                if (authority)
                    id = component->IsReplicated() ? idGenerator_.AllocateReplicated() : idGenerator_.AllocateLocal();
                else
                    id = component->IsReplicated() ? idGenerator_.AllocateUnacked() : idGenerator_.AllocateLocal();
                if (components_.Find(id) == components_.End())
                    break;
            }
        }
        else
        {
            component->SetReplicated(id < UniqueIdGenerator::FIRST_LOCAL_ID);
            // If component ID is specified manually, but it already exists, it is an error. Do not add the component in that case.
            if (components_.Find(id) != components_.End())
            {
                LOGERROR("Can not add component: a component with id " + String(id) + " already exists in entity " + ToString());
                return;
            }
            // Whenever a manual replicated ID is assigned, reset the ID generator to the highest value to avoid unnecessary free ID probing in the future
            if (id < UniqueIdGenerator::FIRST_LOCAL_ID)
                idGenerator_.ResetReplicatedId(std::max(id, idGenerator_.id));
        }

        component->SetNewId(id);
        component->SetParentEntity(this);
        components_[id] = component;
        
        if (change != AttributeChange::Disconnected)
            ComponentAdded.Emit(component.Get(), change == AttributeChange::Default ? component->UpdateMode() : change);
        if (scene_)
            scene_->EmitComponentAdded(this, component.Get(), change);
    }
}

void Entity::RemoveComponent(const ComponentPtr &component, AttributeChange::Type change)
{
    if (component)
    {
        for(ComponentMap::Iterator it = components_.Begin(); it != components_.End(); ++it)
            if (it->second_ == component)
            {
                RemoveComponent(it, change);
                return;
            }

        LOGWARNINGF("Entity::RemoveComponent: Failed to find %s \"%s\" from %s.", component->TypeName().CString(), component->GetName().CString(), ToString().CString());
    }
}

void Entity::RemoveAllComponents(AttributeChange::Type change)
{
    while(!components_.Empty())
    {
        if (components_.Begin()->first_ != components_.Begin()->second_->Id())
            LOGWARNING("Component ID mismatch on RemoveAllComponents: map key " + String(components_.Begin()->first_) + " component ID " + String(components_.Begin()->second_->Id()));
        RemoveComponent(components_.Begin(), change);
    }
}

void Entity::RemoveComponent(ComponentMap::Iterator iter, AttributeChange::Type change)
{
    const ComponentPtr& component = iter->second_;

    if (change != AttributeChange::Disconnected)
        ComponentRemoved(iter->second_.Get(), change == AttributeChange::Default ? component->UpdateMode() : change);
    if (scene_)
        scene_->EmitComponentRemoved(this, iter->second_.Get(), change);

    iter->second_->SetParentEntity(0);
    components_.Erase(iter);
}


void Entity::RemoveComponentById(component_id_t id, AttributeChange::Type change)
{
    ComponentPtr comp = ComponentById(id);
    if (comp)
        RemoveComponent(comp, change);
}

size_t Entity::RemoveComponents(const String &typeName, AttributeChange::Type change)
{
    Vector<component_id_t> removeIds;
    for(ComponentMap::ConstIterator it = components_.Begin(); it != components_.End(); ++it)
        if (it->second_->TypeName().Compare(typeName) == 0)
            removeIds.Push(it->first_);
            
    foreach(component_id_t id, removeIds)
        RemoveComponentById(id, change);
    
    return removeIds.Size();
}

size_t Entity::RemoveComponents(u32 typeId, AttributeChange::Type change)
{
    Vector<component_id_t> removeIds;
    for(ComponentMap::ConstIterator it = components_.Begin(); it != components_.End(); ++it)
        if (it->second_->TypeId() == typeId)
            removeIds.Push(it->first_);

    foreach(component_id_t id, removeIds)
        RemoveComponentById(id, change);

    return removeIds.Size();
}

ComponentPtr Entity::GetOrCreateComponent(const String &type_name, AttributeChange::Type change, bool replicated)
{
    ComponentPtr existing = Component(type_name);
    if (existing)
        return existing;

    return CreateComponent(type_name, change, replicated);
}

ComponentPtr Entity::GetOrCreateComponent(const String &type_name, const String &name, AttributeChange::Type change, bool replicated)
{
    ComponentPtr existing = Component(type_name, name);
    if (existing)
        return existing;

    return CreateComponent(type_name, name, change, replicated);
}

ComponentPtr Entity::GetOrCreateComponent(u32 typeId, AttributeChange::Type change, bool replicated)
{
    ComponentPtr existing = Component(typeId);
    if (existing)
        return existing;

    return CreateComponent(typeId, change, replicated);
}

ComponentPtr Entity::GetOrCreateComponent(u32 typeId, const String &name, AttributeChange::Type change, bool replicated)
{
    ComponentPtr new_comp = Component(typeId, name);
    if (new_comp)
        return new_comp;

    return CreateComponent(typeId, name, change, replicated);
}

ComponentPtr Entity::GetOrCreateLocalComponent(const String& type_name)
{
    return GetOrCreateComponent(type_name, AttributeChange::LocalOnly, false);
}

ComponentPtr Entity::GetOrCreateLocalComponent(const String& type_name, const String& name)
{
    return GetOrCreateComponent(type_name, name, AttributeChange::LocalOnly, false);
}

ComponentPtr Entity::CreateComponent(const String &type_name, AttributeChange::Type change, bool replicated)
{
    ComponentPtr new_comp = framework_->Scene()->CreateComponentByName(scene_, type_name);
    if (!new_comp)
    {
        LOGERROR("Failed to create a component of type \"" + type_name + "\" to " + ToString());
        return ComponentPtr();
    }

    // If changemode is default, and new component requests to not be replicated by default, honor that
    if (change != AttributeChange::Default || new_comp->IsReplicated())
        new_comp->SetReplicated(replicated);
    
    AddComponent(new_comp, change);
    return new_comp;
}

ComponentPtr Entity::CreateComponent(const String &type_name, const String &name, AttributeChange::Type change, bool replicated)
{
    ComponentPtr new_comp = framework_->Scene()->CreateComponentByName(scene_, type_name, name);
    if (!new_comp)
    {
        LOGERROR("Failed to create a component of type \"" + type_name + "\" and name \"" + name + "\" to " + ToString());
        return ComponentPtr();
    }

    // If changemode is default, and new component requests to not be replicated by default, honor that
    if (change != AttributeChange::Default || new_comp->IsReplicated())
        new_comp->SetReplicated(replicated);
    
    AddComponent(new_comp, change);
    return new_comp;
}

ComponentPtr Entity::CreateComponent(u32 typeId, AttributeChange::Type change, bool replicated)
{
    ComponentPtr new_comp = framework_->Scene()->CreateComponentById(scene_, typeId);
    if (!new_comp)
    {
        LOGERROR("Failed to create a component of type id " + String(typeId) + " to " + ToString());
        return ComponentPtr();
    }

    // If changemode is default, and new component requests to not be replicated by default, honor that
    if (change != AttributeChange::Default || new_comp->IsReplicated())
        new_comp->SetReplicated(replicated);
    
    AddComponent(new_comp, change);
    return new_comp;
}

ComponentPtr Entity::CreateComponent(u32 typeId, const String &name, AttributeChange::Type change, bool replicated)
{
    ComponentPtr new_comp = framework_->Scene()->CreateComponentById(scene_, typeId, name);
    if (!new_comp)
    {
        LOGERROR("Failed to create a component of type id " + String(typeId) + " and name \"" + name + "\" to " + ToString());
        return ComponentPtr();
    }

    // If changemode is default, and new component requests to not be replicated by default, honor that
    if (change != AttributeChange::Default || new_comp->IsReplicated())
        new_comp->SetReplicated(replicated);
    
    AddComponent(new_comp, change);
    return new_comp;
}

ComponentPtr Entity::CreateComponentWithId(component_id_t compId, u32 typeId, const String &name, AttributeChange::Type change)
{
    ComponentPtr new_comp = framework_->Scene()->CreateComponentById(scene_, typeId, name);
    if (!new_comp)
    {
        LOGERROR("Failed to create a component of type id " + String(typeId) + " and name \"" + name + "\" to " + ToString());
        return ComponentPtr();
    }

    // If this overload is called with id 0, it must come from SyncManager (server). In that case make sure we do not allow the component to be created as local
    if (!compId)
        new_comp->SetReplicated(true);

    AddComponent(compId, new_comp, change);
    return new_comp;
}

ComponentPtr Entity::CreateLocalComponent(const String &type_name)
{
    return CreateComponent(type_name, AttributeChange::LocalOnly, false);
}

ComponentPtr Entity::CreateLocalComponent(const String &type_name, const String &name)
{
    return CreateComponent(type_name, name, AttributeChange::LocalOnly, false);
}

ComponentPtr Entity::ComponentById(component_id_t id) const
{
    ComponentMap::ConstIterator i = components_.Find(id);
    return (i != components_.End() ? i->second_ : ComponentPtr());
}

ComponentPtr Entity::Component(const String &typeName) const
{
    const String cTypeName = IComponent::EnsureTypeNameWithoutPrefix(typeName);
    for (ComponentMap::ConstIterator i = components_.Begin(); i != components_.End(); ++i)
        if (i->second_->TypeName() == cTypeName)
            return i->second_;

    return ComponentPtr();
}

ComponentPtr Entity::Component(u32 typeId) const
{
    for (ComponentMap::ConstIterator i = components_.Begin(); i != components_.End(); ++i)
        if (i->second_->TypeId() == typeId)
            return i->second_;

    return ComponentPtr();
}

Entity::ComponentVector Entity::ComponentsOfType(const String &typeName) const
{
    return ComponentsOfType(framework_->Scene()->ComponentTypeIdForTypeName(typeName));
}

Entity::ComponentVector Entity::ComponentsOfType(u32 typeId) const
{
    ComponentVector ret;
    for (ComponentMap::ConstIterator i = components_.Begin(); i != components_.End(); ++i)
        if (i->second_->TypeId() == typeId)
            ret.Push(i->second_);
    return ret;
}

ComponentPtr Entity::Component(const String &type_name, const String& name) const
{
    const String cTypeName = IComponent::EnsureTypeNameWithoutPrefix(type_name);
    for (ComponentMap::ConstIterator i = components_.Begin(); i != components_.End(); ++i)
        if (i->second_->TypeName() == cTypeName && i->second_->GetName() == name)
            return i->second_;

    return ComponentPtr();
}

ComponentPtr Entity::Component(u32 typeId, const String& name) const
{
    for (ComponentMap::ConstIterator i = components_.Begin(); i != components_.End(); ++i)
        if (i->second_->TypeId() == typeId && i->second_->GetName() == name)
            return i->second_;

    return ComponentPtr();
}

void Entity::SerializeToBinary(kNet::DataSerializer &dst, bool serializeTemporary, bool serializeLocal, bool serializeChildren) const
{
    dst.Add<u32>(Id());
    dst.Add<u8>(IsReplicated() ? 1 : 0);
    Vector<ComponentPtr> serializable;
    Vector<EntityPtr> serializableChildren;
    for (ComponentMap::ConstIterator i = components_.Begin(); i != components_.End(); ++i)
        if (i->second_->ShouldBeSerialized(serializeTemporary, serializeLocal))
            serializable.Push(i->second_);

    if (serializeChildren)
    {
        foreach(const EntityWeakPtr &childWeak, children_)
        {
            const EntityPtr child = childWeak.Lock();
            if (child && child->ShouldBeSerialized(serializeTemporary, serializeLocal, serializeChildren))
                serializableChildren.Push(child);
        }
    }

    /// \hack Retain binary compatibility with earlier scene format, at the cost of max. 65535 components or child entities
    if (serializable.Size() > 0xffff)
        LOGERROR("Entity::SerializeToBinary: entity contains more than 65535 components, binary save will be erroneous");
    if (serializableChildren.Size() > 0xffff)
        LOGERROR("Entity::SerializeToBinary: entity contains more than 65535 child entities, binary save will be erroneous");

    dst.Add<u32>(serializable.Size() | (serializableChildren.Size() << 16));
    foreach(const ComponentPtr &comp, serializable)
    {
        dst.Add<u32>(comp->TypeId()); ///\todo VLE this!
        dst.AddString(comp->GetName().CString());
        dst.Add<u8>(comp->IsReplicated() ? 1 : 0);

        // Write each component to a separate buffer, then write out its size first, so we can skip unknown components
        PODVector<unsigned char> comp_bytes;
        // Assume 64KB max per component for now
        comp_bytes.Resize(64 * 1024);
        kNet::DataSerializer comp_dest((char*)&comp_bytes[0], comp_bytes.Size());
        comp->SerializeToBinary(comp_dest);
        comp_bytes.Resize(comp_dest.BytesFilled());
        
        dst.Add<u32>(comp_bytes.Size());
        if (comp_bytes.Size())
            dst.AddArray<u8>((const u8*)&comp_bytes[0], comp_bytes.Size());
    }

    // Serialize child entities
    if (serializeChildren)
    {
        foreach(const EntityPtr child, serializableChildren)
            child->SerializeToBinary(dst, serializeTemporary, true);
    }
}

/* Disabled for now, since have to decide how entityID conflicts are handled.
void Entity::DeserializeFromBinary(kNet::DataDeserializer &src, AttributeChange::Type change)
{
}*/

void Entity::SerializeToXML(Urho3D::XMLFile &doc, Urho3D::XMLElement &base_element, bool serializeTemporary, bool serializeLocal, bool serializeChildren) const
{
    Urho3D::XMLElement entity_elem;

    if (base_element)
        entity_elem = base_element.CreateChild("entity");
    else
        entity_elem = doc.CreateRoot("entity");

    entity_elem.SetUInt("id", Id());
    entity_elem.SetBool("sync", IsReplicated());
    if (serializeTemporary)
        entity_elem.SetBool("temporary", IsTemporary());

    for (ComponentMap::ConstIterator i = components_.Begin(); i != components_.End(); ++i)
        if (i->second_->ShouldBeSerialized(serializeTemporary, serializeLocal))
            i->second_->SerializeTo(doc, entity_elem, serializeTemporary);

    // Serialize child entities
    if (serializeChildren)
    {
        for(ChildEntityVector::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
        {
            const EntityPtr child = i->Lock();
            if (child && child->ShouldBeSerialized(serializeTemporary, serializeLocal, serializeChildren))
                child->SerializeToXML(doc, entity_elem, serializeTemporary, serializeLocal);
        }
    }
}

/* Disabled for now, since have to decide how entityID conflicts are handled.
void Entity::DeserializeFromXML(Urho3D::XMLElement& element, AttributeChange::Type change)
{
}*/

String Entity::SerializeToXMLString(bool serializeTemporary, bool serializeLocal, bool serializeChildren, bool createSceneElement) const
{
    if (createSceneElement)
    {
        Urho3D::XMLFile sceneDoc(context_);
        Urho3D::XMLElement sceneElem = sceneDoc.CreateRoot("scene");
        SerializeToXML(sceneDoc, sceneElem, serializeTemporary, serializeLocal, serializeChildren);
        return sceneDoc.ToString();
    }
    else
    {
        Urho3D::XMLFile entity_doc(context_);
        Urho3D::XMLElement null_elem;
        SerializeToXML(entity_doc, null_elem, serializeTemporary, serializeLocal, serializeChildren);
        return entity_doc.ToString();
    }
}

/* Disabled for now, since have to decide how entityID conflicts are handled.
bool Entity::DeserializeFromXMLString(const String &src, AttributeChange::Type change)
{
    Urho3D::XMLFile entityDocument("Entity");
    if (!entityDocument.setContent(src))
    {
        LogError("Parsing entity XML from text failed.");
        return false;
    }

    return CreateContentFromXml(entityDocument, replaceOnConflict, change);
}*/

EntityPtr Entity::Clone(bool local, bool temporary, const String &cloneName, AttributeChange::Type changeType) const
{
    Urho3D::XMLFile doc(context_);
    Urho3D::XMLElement sceneElem = doc.CreateRoot("scene");
    Urho3D::XMLElement entityElem = sceneElem.CreateChild("entity");
    entityElem.SetBool("sync", IsReplicated());
    entityElem.SetUInt("id", local ? scene_->NextFreeIdLocal() : scene_->NextFreeId());
    // Set the temporary status in advance so it's valid when Scene::CreateContentFromXml signals changes in the scene
    entityElem.SetBool("temporary", temporary);
    // Setting of a new name for the clone is a bit clumsy, but this is the best way to do it currently.
    const bool setNameForClone = !cloneName.Empty();
    bool cloneNameWritten = false;
    for(ComponentMap::ConstIterator i = components_.Begin(); i != components_.End(); ++i)
    {
        i->second_->SerializeTo(doc, entityElem);
        if (setNameForClone && !cloneNameWritten && i->second_->TypeId() == Name::ComponentTypeId)
        {
            // Now that we've serialized the Name component, overwrite value of the "name" attribute.
            Urho3D::XMLElement nameComponentElem = entityElem.GetChild("component");
            while (nameComponentElem.GetAttribute("type") != Name::TypeNameStatic())
                nameComponentElem = nameComponentElem.GetNext("component");
            nameComponentElem.GetChild("attribute").SetAttribute("value", cloneName);
            cloneNameWritten = true;
        }
    }
    // Serialize child entities
    for(ChildEntityVector::ConstIterator i = children_.Begin(); i != children_.End(); ++i)
        if (!i->Expired())
            i->Lock()->SerializeToXML(doc, entityElem, true, true);

    Vector<Entity *> newEntities = scene_->CreateContentFromXml(doc, true, changeType);
    // Set same parent for the new entity as the original has
    if (newEntities.Size())
        newEntities[0]->SetParent(Parent(), changeType);

    return (!newEntities.Empty() ? EntityPtr(newEntities[0]) : EntityPtr());
}

void Entity::SetName(const String &name)
{
    SharedPtr<Name> comp = GetOrCreateComponent<Name>();
    assert(comp);
    comp->name.Set(name, AttributeChange::Default);
}

String Entity::GetName() const
{
    SharedPtr<Name> name = Component<Name>();
    return name ? name->name.Get() : "";
}

void Entity::SetDescription(const String &desc)
{
    SharedPtr<Name> comp = GetOrCreateComponent<Name>();
    assert(comp);
    comp->description.Set(desc, AttributeChange::Default);
}

String Entity::Description() const
{
    SharedPtr<Name> name = Component<Name>();
    return name ? name->description.Get() : "";
}

void Entity::SetGroup(const String &groupName)
{
    SharedPtr<Name> comp = GetOrCreateComponent<Name>();
    comp->group.Set(groupName, AttributeChange::Default);
}

String Entity::Group() const
{
    SharedPtr<Name> comp = Component<Name>();
    return comp ? comp->group.Get() : "";
}

EntityAction *Entity::Action(const String &name)
{
    for (ActionMap::Iterator it = actions_.Begin(); it != actions_.End(); ++it)
    {
        if (it->second_->GetName().Compare(name, false) == 0)
            return it->second_;
    }

    EntityAction *action = new EntityAction(name);
    actions_.Insert(MakePair(name, action));
    return action;
}

void Entity::RemoveAction(const String &name)
{
    ActionMap::Iterator iter = actions_.Find(name);
    if (iter != actions_.End())
    {
        delete (iter->second_);
        actions_.Erase(iter);
    }
}

void Entity::Exec(EntityAction::ExecTypeField type, const String &action, const String &p1, const String &p2, const String &p3)
{
    StringVector params;
    if (p1.Length())
        params.Push(p1);
    if (p2.Length())
        params.Push(p2);
    if (p3.Length())
        params.Push(p3);
    Exec(type, action, params);
}

void Entity::Exec(EntityAction::ExecTypeField type, const String &action, const StringVector &params)
{
    PROFILE(Entity_ExecEntityAction);
    
    EntityAction *act = Action(action);
    if ((type & EntityAction::Local) != 0)
    {
        if (params.Size() == 0)
            act->Trigger();
        else if (params.Size() == 1)
            act->Trigger(params[0]);
        else if (params.Size() == 2)
            act->Trigger(params[0], params[1]);
        else if (params.Size() == 3)
            act->Trigger(params[0], params[1], params[2]);
        else if (params.Size() >= 4)
        {
            StringVector rest;
            rest.Reserve(params.Size() - 3);
            for (u32 i = 3; i < params.Size(); ++i)
                rest.Push(params[i]);
            act->Trigger(params[0], params[1], params[2], rest);
        }
    }

    if (ParentScene())
        ParentScene()->EmitActionTriggered(this, action, params, type);
}

void Entity::Exec(EntityAction::ExecTypeField type, const String &action, const VariantList &params)
{
    StringVector stringParams;
    foreach(Variant var, params)
        stringParams.Push(var.ToString());
    Exec(type, action, stringParams);
}

void Entity::EmitEntityRemoved(AttributeChange::Type change)
{
    if (change == AttributeChange::Disconnected)
        return;
    if (change == AttributeChange::Default)
        change = AttributeChange::Replicate;
    EntityRemoved.Emit(this, change);
}

void Entity::EmitEnterView(IComponent* camera)
{
    EnterView.Emit(camera);
}

void Entity::EmitLeaveView(IComponent* camera)
{
    LeaveView.Emit(camera);
}

void Entity::SetTemporary(bool enable, AttributeChange::Type change)
{
    if (enable != temporary_)
    {
        temporary_ = enable;
        if (change == AttributeChange::Default)
            change = AttributeChange::Replicate;
        if (change != AttributeChange::Disconnected)
            TemporaryStateToggled.Emit(this, change);
    }
}

String Entity::ToString() const
{
    String name = GetName();
    if (name.Trimmed().Empty())
        return String("Entity ID ") + String(Id());
    else
        return String("Entity \"") + name + "\" (ID: " + String(Id()) + ")";
}

void Entity::AddChild(EntityPtr child, AttributeChange::Type change)
{
    if (!child)
    {
        LOGWARNING("Entity::AddChild: null child entity specified");
        return;
    }

    child->SetParent(EntityPtr(this), change);
}

void Entity::RemoveChild(EntityPtr child, AttributeChange::Type change)
{
    if (child->Parent().Get() != this)
    {
        LOGWARNING("Entity::RemoveChild: the specified entity is not parented to this entity");
        return;
    }

    // RemoveEntity does a silent SetParent(0) to remove the child from the parent's child vector
    if (scene_)
        scene_->RemoveEntity(child->Id(), change);
    else
        LOGERROR("Entity::RemoveChild: null parent scene, can not remove the entity from scene");
}

void Entity::RemoveAllChildren(AttributeChange::Type change)
{
    while (children_.Size())
        RemoveChild(children_[0].Lock(), change);
}

void Entity::DetachChild(EntityPtr child, AttributeChange::Type change)
{
    if (!child)
    {
        LOGWARNING("Entity::DetachChild: null child entity specified");
        return;
    }
    if (child->Parent().Get() != this)
    {
        LOGWARNING("Entity::DetachChild: the specified entity is not parented to this entity");
        return;
    }

    child->SetParent(EntityPtr(), change);
}

void Entity::SetParent(EntityPtr parent, AttributeChange::Type change)
{
    EntityPtr oldParent = parent_.Lock();
    if (oldParent == parent)
        return; // Nothing to do

    // Prevent self assignment
    if (parent.Get() == this)
    {
        LOGERROR("Entity::SetParent: self parenting attempted.");
        return;
    }

    // Prevent cyclic assignment
    if (parent)
    {
        Entity* parentCheck = parent.Get();
        while (parentCheck)
        {
            if (parentCheck == this)
            {
                LOGERROR("Entity::SetParent: Cyclic parenting attempted.");
                return;
            }
            parentCheck = parentCheck->Parent().Get();
        }
    }

    // Remove from old parent's child vector
    if (oldParent)
    {
        ChildEntityVector& children = oldParent->children_;
        for (size_t i = 0; i < children.Size(); ++i)
        {
            if (children[i].Lock().Get() == this)
            {
                children.Erase(children.Begin() + i);
                break;
            }
        }
    }

    // Add to new parent's child vector
    if (parent)
        parent->children_.Push(EntityPtr(this));

    parent_ = parent;

    // Emit change signals
    if (change != AttributeChange::Disconnected)
    {
        if (change == AttributeChange::Default)
            change = IsLocal() ? AttributeChange::LocalOnly : AttributeChange::Replicate;
        ParentChanged.Emit(this, parent.Get(), change);
        if (scene_)
            scene_->EmitEntityParentChanged(this, parent.Get(), change);
    }
}

EntityPtr Entity::CreateChild(entity_id_t id, const StringVector &components, AttributeChange::Type change, bool replicated, bool componentsReplicated, bool temporary)
{
    if (!scene_)
    {
        LOGERROR("Entity::CreateChild: not attached to a scene, can not create a child entity");
        return EntityPtr();
    }

    EntityPtr child = scene_->CreateEntity(id, components, change, replicated, componentsReplicated, temporary);
    // Set the parent silently to match entity creation signaling, which is only done at the end of the frame
    if (child)
        child->SetParent(EntityPtr(this), AttributeChange::Disconnected);
    return child;
}

EntityPtr Entity::CreateLocalChild(const StringVector &components, AttributeChange::Type change, bool componentsReplicated, bool temporary)
{
    if (!scene_)
    {
        LOGERROR("Entity::CreateLocalChild: not attached to a scene, can not create a child entity");
        return EntityPtr();
    }

    EntityPtr child = scene_->CreateLocalEntity(components, change, componentsReplicated, temporary);
    if (child)
        child->SetParent(EntityPtr(this), change);
    return child;
}

EntityPtr Entity::Child(size_t index) const
{
    return index < children_.Size() ? children_[index].Lock() : EntityPtr();
}

EntityPtr Entity::ChildByName(const String& name, bool recursive) const
{
    for (size_t i = 0; i < children_.Size(); ++i)
    {
        // Safeguard in case the entity has become null without us knowing
        EntityPtr child = children_[i].Lock();
        if (child)
        {
            if (name.Compare(child->GetName(), false) == 0)
                return child;
            if (recursive)
            {
                EntityPtr childResult = child->ChildByName(name, true);
                if (childResult)
                    return childResult;
            }
        }
    }

    return EntityPtr();
}

EntityVector Entity::Children(bool recursive) const
{
    EntityVector ret;
    CollectChildren(ret, recursive);
    return ret;
}

bool Entity::ShouldBeSerialized(bool serializeTemporary, bool serializeLocal, bool serializeChildren) const
{
    if (IsTemporary() && !serializeTemporary)
        return false;
    if (IsLocal() && !serializeLocal)
        return false;
    if (Parent() && !serializeChildren)
        return false;
    return true;
}

void Entity::CollectChildren(EntityVector& children, bool recursive) const
{
    for (size_t i = 0; i < children_.Size(); ++i)
    {
        // Safeguard in case the entity has become null without us knowing
        EntityPtr child = children_[i].Lock();
        if (child)
        {
            children.Push(child);
            if (recursive)
                child->CollectChildren(children, true);
        }
    }
}

}
