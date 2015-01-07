// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

#include "SceneAPI.h"
#include "Scene/Scene.h"
#include "Entity.h"
#include "SceneDesc.h"
#include "IComponent.h"
#include "IAttribute.h"
#include "Name.h"
#include "AttributeMetadata.h"
#include "ChangeRequest.h"
#include "EntityReference.h"
#include "Framework.h"
#include "FrameAPI.h"
#include "LoggingFunctions.h"
#include "AssetAPI.h"

#include <kNet/DataDeserializer.h>
#include <kNet/DataSerializer.h>

#include <Urho3D/IO/File.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Core/Profiler.h>

using namespace kNet;
using namespace std;

namespace Tundra
{

Scene::Scene(const String &name, Framework *framework, bool viewEnabled, bool authority) :
    Object(framework->GetContext()),
    name_(name),
    framework_(framework),
    interpolating_(false),
    authority_(authority)
{
    // In headless mode only view disabled-scenes can be created
    viewEnabled_ = framework->IsHeadless() ? false : viewEnabled;

    // Connect to frame update to handle signaling entities created on this frame
    framework->Frame()->Updated.Connect(this, &Scene::OnUpdated);
}

Scene::~Scene()
{
    if (subsystems.Size())
        LogWarning("Subsystems not removed at scene destruct time");

    EndAllAttributeInterpolations();
    
    // Do not send entity removal or scene cleared events on destruction
    RemoveAllEntities(false);
    
    Removed.Emit(this);
}

EntityPtr Scene::CreateLocalEntity(const StringVector &components, AttributeChange::Type change, bool componentsReplicated, bool temporary)
{
    return CreateEntity(0, components, change, false, componentsReplicated, temporary);
}

EntityPtr Scene::CreateEntity(entity_id_t id, const StringVector &components, AttributeChange::Type change, bool replicated, bool componentsReplicated, bool temporary)
{
    // Figure out new entity id
    if (id == 0)
    {
        // Loop until a free ID found
        for (;;)
        {
            if (IsAuthority())
                id = replicated ? idGenerator_.AllocateReplicated() : idGenerator_.AllocateLocal();
            else
                id = replicated ? idGenerator_.AllocateUnacked() : idGenerator_.AllocateLocal();
            if (entities_.Find(id) == entities_.End())
                break;
        }
    }
    else
    {
        if(entities_.Find(id) != entities_.End())
        {
            LogError("Can't create entity with given id because it's already used: " + String(id));
            return EntityPtr();
        }
        else
        {
            // Reset the ID generator to the manually assigned value to avoid unnecessary free ID probing in the future
            if (id < UniqueIdGenerator::FIRST_LOCAL_ID)
                idGenerator_.ResetReplicatedId(Max(id, idGenerator_.id));
        }
    }

    EntityPtr entity(new Entity(framework_, id, temporary, this));
    for(u32 i = 0; i < components.Size(); ++i)
    {
        ComponentPtr newComp = framework_->Scene()->CreateComponentByName(this, components[i]);
        if (newComp)
        {
            newComp->SetReplicated(componentsReplicated);
            entity->AddComponent(newComp, change); //change the param to a Stringlist or so \todo XXX
        }
    }
    entities_[entity->Id()] = entity;

    // Remember the creation and signal at end of frame if EmitEntityCreated() not called for this entity manually
    entitiesCreatedThisFrame_.Push(MakePair(EntityWeakPtr(entity), change));

    return entity;
}

EntityPtr Scene::CreateTemporaryEntity(const StringVector &components, AttributeChange::Type change, bool componentsReplicated)
{
    return CreateEntity(0, components, change, true, componentsReplicated, true);
}

EntityPtr Scene::CreateLocalTemporaryEntity(const StringVector &components, AttributeChange::Type change)
{
    return CreateEntity(0, components, change, false, false, true);
}

EntityPtr Scene::EntityById(entity_id_t id) const
{
    EntityMap::ConstIterator it = entities_.Find(id);
    if (it != entities_.End())
        return it->second_;

    return EntityPtr();
}

EntityPtr Scene::EntityByName(const String &name) const
{
    if (name.Empty())
        return EntityPtr();

    for(ConstIterator it = Begin(); it != End(); ++it)
        if (it->second_->Name() == name)
            return it->second_;

    return EntityPtr();
}

bool Scene::IsUniqueName(const String& name) const
{
    return !EntityByName(name);
}

void Scene::AddSubsystem(Object* system)
{
    if (system)
        subsystems[system->GetType()] = system;
}

void Scene::RemoveSubsystem(Object* system)
{
    if (system)
        subsystems.Erase(system->GetType());
}

void Scene::ChangeEntityId(entity_id_t old_id, entity_id_t new_id)
{
    if (old_id == new_id)
        return;
    
    EntityPtr old_entity = EntityById(old_id);
    if (!old_entity)
        return;
    
    if (EntityById(new_id))
    {
        LogWarning("Purged entity " + String(new_id) + " to make room for a ChangeEntityId request. This should not happen");
        RemoveEntity(new_id, AttributeChange::LocalOnly);
    }
    
    old_entity->SetNewId(new_id);
    entities_.Erase(old_id);
    entities_[new_id] = old_entity;
}

bool Scene::RemoveEntity(entity_id_t id, AttributeChange::Type change)
{
    EntityMap::Iterator it = entities_.Find(id);
    if (it != entities_.End())
    {
        EntityPtr del_entity = it->second_;
        if (!del_entity.Get())
        {
            LogError("Scene::RemoveEntity: Found null EntityPtr from internal state with id " + String(id));
            return false;
        }
        
        /// \bug Emit entity removal first, as there may be scripts which depend on components still being there for their cleanup.
        /// This is necessary as QScriptEngine may not handle later access to removed objects gracefully and may eg exit() the whole program.
        /// Even a malfunctioning script should not be able to bring the Tundra process down in this manner.
        EmitEntityRemoved(del_entity.Get(), change);
        
        // Then make the entity remove all of its components, so that their individual removals are signaled properly
        del_entity->RemoveAllComponents(change);
        
        // If the entity is parented, remove from the parent. No signaling is necessary
        if (del_entity->Parent())
            del_entity->SetParent(EntityPtr(), AttributeChange::Disconnected);

        // Remove all child entities. This may be recursive
        del_entity->RemoveAllChildren(change);

        entities_.Erase(it);
        
        // If entity somehow manages to live, at least it doesn't belong to the scene anymore
        del_entity->SetScene(0);
        del_entity.Reset();
        return true;
    }
    return false;
}

void Scene::RemoveAllEntities(bool signal, AttributeChange::Type change)
{
    // If we don't want to emit signals, make sure the change mode is disconnected.
    if (!signal && change != AttributeChange::Disconnected)
        change = AttributeChange::Disconnected;

    // Gather entity ids to call RemoveEntity, as it modifies 
    // the entities_ map we should not call RemoveEntity while iterating it.
    ///\todo The following code was done to resolve a mysterious crash bug.
    ///      See https://github.com/Adminotech/tundra/commit/cb051bb270be3ce6e64a822593f1e14675bbf922
    ///      Contact Jonne for more info. -cs
    List<entity_id_t> entIds;
    for (Iterator it = Begin(); it != End(); ++it)
    {
        // Only root-level entities need to be removed, the rest clean themselves up automatically
        if (it->second_.Get() && !it->second_->Parent())
            entIds.Push(it->second_->Id());
    }
    while(entIds.Size() > 0)
    {
        RemoveEntity(entIds.Back(), change);
        entIds.Pop();
    }
    
    if (entities_.Size())
    {
        LogWarning("Scene::RemoveAllEntities: entity map was not clear after removing all entities, clearing manually");
        entities_.Clear();
    }
    
    if (signal)
        SceneCleared.Emit(this);

    idGenerator_.Reset();
}

entity_id_t Scene::NextFreeId()
{
    if (IsAuthority())
        return idGenerator_.AllocateReplicated();
    else
        return idGenerator_.AllocateUnacked();
}

entity_id_t Scene::NextFreeIdLocal()
{
    return idGenerator_.AllocateLocal();
}

EntityVector Scene::EntitiesWithComponent(const String &typeName, const String &name) const
{
    return EntitiesWithComponent(framework_->Scene()->ComponentTypeIdForTypeName(typeName), name);
}

EntityVector Scene::EntitiesWithComponent(u32 typeId, const String &name) const
{
    EntityVector entities;
    for(ConstIterator it = Begin(); it != End(); ++it)
        if ((name.Empty() && it->second_->Component(typeId)) || it->second_->Component(typeId, name))
            entities.Push(it->second_);
    return entities;
}

EntityVector Scene::EntitiesOfGroup(const String &groupName) const
{
    EntityVector entities;
    if (groupName.Empty())
        return entities;

    for (ConstIterator it = Begin(); it != End(); ++it)
        if (it->second_->Group() == groupName)
            entities.Push(it->second_);

    return entities;
}

Entity::ComponentVector Scene::Components(const String &typeName, const String &name) const
{
    return Components(framework_->Scene()->ComponentTypeIdForTypeName(typeName), name);
}

Entity::ComponentVector Scene::Components(u32 typeId, const String &name) const
{
    Entity::ComponentVector ret;
    if (name.Empty())
    {
        for(ConstIterator it = Begin(); it != End(); ++it)
        {
            Entity::ComponentVector components =  it->second_->ComponentsOfType(typeId);
            if (!components.Empty())
                ret.Insert(ret.End(), components.Begin(), components.End());
        }
    }
    else
    {
        for(ConstIterator it = Begin(); it != End(); ++it)
        {
            ComponentPtr component = it->second_->Component(typeId, name);
            if (component)
                ret.Push(component);
        }
    }
    return ret;
}

void Scene::EmitComponentAdded(Entity* entity, IComponent* comp, AttributeChange::Type change)
{
    if (change == AttributeChange::Disconnected)
        return;
    if (change == AttributeChange::Default)
        change = comp->UpdateMode();
    ComponentAdded.Emit(entity, comp, change);
}

void Scene::EmitComponentRemoved(Entity* entity, IComponent* comp, AttributeChange::Type change)
{
    if (change == AttributeChange::Disconnected)
        return;
    if (change == AttributeChange::Default)
        change = comp->UpdateMode();
    ComponentRemoved.Emit(entity, comp, change);
}

void Scene::EmitAttributeChanged(IComponent* comp, IAttribute* attribute, AttributeChange::Type change)
{
    if (!comp || !attribute || change == AttributeChange::Disconnected)
        return;
    if (change == AttributeChange::Default)
        change = comp->UpdateMode();
    AttributeChanged.Emit(comp, attribute, change);
}

void Scene::EmitAttributeAdded(IComponent* comp, IAttribute* attribute, AttributeChange::Type change)
{
    // "Stealth" addition (disconnected changetype) is not supported. Always signal.
    if (!comp || !attribute)
        return;
    if (change == AttributeChange::Default)
        change = comp->UpdateMode();
    AttributeAdded.Emit(comp, attribute, change);
}

void Scene::EmitAttributeRemoved(IComponent* comp, IAttribute* attribute, AttributeChange::Type change)
{
    // "Stealth" removal (disconnected changetype) is not supported. Always signal.
    if (!comp || !attribute)
        return;
    if (change == AttributeChange::Default)
        change = comp->UpdateMode();
    AttributeRemoved.Emit(comp, attribute, change);
}

void Scene::EmitEntityCreated(Entity *entity, AttributeChange::Type change)
{
    // Remove from the create signalling queue
    for (unsigned i = 0; i < entitiesCreatedThisFrame_.Size(); ++i)
    {
        if (entitiesCreatedThisFrame_[i].first_.Get() == entity)
        {
            entitiesCreatedThisFrame_.Erase(entitiesCreatedThisFrame_.Begin() + i);
            break;
        }
    }
    
    if (change == AttributeChange::Disconnected)
        return;
    if (change == AttributeChange::Default)
        change = AttributeChange::Replicate;
    ///@note This is not enough, it might be that entity is deleted after this call so we have dangling pointer in queue. 
    if (entity)
        EntityCreated.Emit(entity, change);
}

void Scene::EmitEntityParentChanged(Entity* entity, Entity* newParent, AttributeChange::Type change)
{
    if (!entity || change == AttributeChange::Disconnected)
        return;
    if (change == AttributeChange::Default)
        change = entity->IsLocal() ? AttributeChange::LocalOnly : AttributeChange::Replicate;
    EntityParentChanged.Emit(entity, newParent, change);
}

void Scene::EmitEntityRemoved(Entity* entity, AttributeChange::Type change)
{
    if (change == AttributeChange::Disconnected)
        return;
    if (change == AttributeChange::Default)
        change = AttributeChange::Replicate;
    EntityRemoved.Emit(entity, change);
    entity->EmitEntityRemoved(change);
}

void Scene::EmitActionTriggered(Entity *entity, const String &action, const StringVector &params, EntityAction::ExecTypeField type)
{
    ActionTriggered.Emit(entity, action, params, type);
}

//before-the-fact counterparts for the modification signals above, for permission checks
bool Scene::AllowModifyEntity(UserConnection* user, Entity *entity)
{
    ChangeRequest req;
    AboutToModifyEntity.Emit(&req, user, entity);
    return req.allowed;
}

void Scene::EmitEntityAcked(Entity* entity, entity_id_t oldId)
{
    if (entity)
    {
        EntityAcked.Emit(entity, oldId);

        /** On client feed the acked ids, once tracker receives the last
            id it will process the tracked scene part for broken parenting.
            This is done after above emit so that behavior for all entities
            in the tracker is the same (emit before parenting is fixed). */
        if (!IsAuthority())
            parentTracker_.Ack(this, entity->Id(), oldId);
    }
}

void Scene::EmitComponentAcked(IComponent* comp, component_id_t oldId)
{
    if (comp)
        ComponentAcked.Emit(comp, oldId);
}

Vector<Entity *> Scene::LoadSceneXML(const String& filename, bool clearScene, bool useEntityIDsFromFile, AttributeChange::Type change)
{
    Urho3D::File file(context_);
    if (!file.Open(filename, Urho3D::FILE_READ))
    {
        LogError("Scene::LoadSceneXML: Failed to open file " + filename + ".");
        return Vector<Entity*>();
    }

    Urho3D::XMLFile scene_doc(context_);
    if (!scene_doc.Load(file))
    {
        LogError("Parsing scene XML from " + filename + " failed.");
        return Vector<Entity*>();
    }
    file.Close();

    // Purge all old entities. Send events for the removal
    if (clearScene)
        RemoveAllEntities(true, change);

    return CreateContentFromXml(scene_doc, useEntityIDsFromFile, change);
}

String Scene::SerializeToXmlString(bool serializeTemporary, bool serializeLocal) const
{
    Urho3D::XMLFile sceneDoc(context_);
    Urho3D::XMLElement sceneElem = sceneDoc.CreateRoot("scene");

    const bool serializeChildren = true;

    EntityVector list = RootLevelEntities();
    foreach(EntityPtr ent, list)
        if (ent->ShouldBeSerialized(serializeTemporary, serializeLocal, serializeChildren))
            ent->SerializeToXML(sceneDoc, sceneElem, serializeTemporary, serializeLocal, serializeChildren);

    return sceneDoc.ToString();
}

bool Scene::SaveSceneXML(const String& filename, bool serializeTemporary, bool serializeLocal) const
{
    String sceneXML = SerializeToXmlString(serializeTemporary, serializeLocal);

    Urho3D::File scenefile(context_);
    if (!scenefile.Open(filename, Urho3D::FILE_WRITE))
    {
        LogError("SaveSceneXML: Failed to open file " + filename + " for writing.");
        return false;
    }

    scenefile.WriteString(sceneXML);
    return true;
}

Vector<Entity *> Scene::LoadSceneBinary(const String& filename, bool clearScene, bool useEntityIDsFromFile, AttributeChange::Type change)
{
    Vector<Entity *> ret;

    Urho3D::File file(context_);
    if (!file.Open(filename, Urho3D::FILE_READ))
    {
        LogError("Scene::LoadSceneBinary: Failed to open file " + filename + ".");
        return ret;
    }

    if (!file.GetSize())
    {
        LogError("Scene::LoadSceneBinary: File " + filename + " contained 0 bytes when loading scene binary.");
        return ret;
    }

    PODVector<char> bytes;
    bytes.Resize(file.GetSize());
    file.Read(&bytes[0], bytes.Size());
    file.Close();

    if (clearScene)
        RemoveAllEntities(true, change);

    return CreateContentFromBinary(&bytes[0], bytes.Size(), useEntityIDsFromFile, change);
}

bool Scene::SaveSceneBinary(const String& filename, bool serializeTemporary, bool serializeLocal) const
{
    PODVector<char> bytes;
    // Assume 4MB max for now
    /// \todo Use Urho serialization for dynamic buffer
    bytes.Resize(4 * 1024 * 1024);
    DataSerializer dest(&bytes[0], bytes.Size());

    // Filter the entities we accept
    const bool serializeChildren = true;
    EntityVector serialized = RootLevelEntities();
    for(EntityVector::Iterator iter = serialized.Begin(); iter != serialized.End();)
    {
        if ((*iter)->ShouldBeSerialized(serializeTemporary, serializeLocal, serializeChildren))
             ++iter;
        else
            iter = serialized.Erase(iter);
    }

    dest.Add<u32>(serialized.Size());

    foreach(EntityPtr entity, serialized)
        entity->SerializeToBinary(dest, serializeTemporary, serializeChildren, serializeLocal);

    bytes.Resize(static_cast<uint>(dest.BytesFilled()));
    Urho3D::File scenefile(context_);
    if (!scenefile.Open(filename, Urho3D::FILE_WRITE))
    {
        LogError("Scene::SaveSceneBinary: Could not open file " + filename + " for writing when saving scene binary.");
        return false;
    }

    if (bytes.Size())
        scenefile.Write(&bytes[0], bytes.Size());
    return true;
}

Vector<Entity *> Scene::CreateContentFromXml(const String &xml,  bool useEntityIDsFromFile, AttributeChange::Type change)
{
    Vector<Entity *> ret;
    String errorMsg;
    Urho3D::XMLFile scene_doc(context_);
    if (!scene_doc.FromString(xml))
    {
        LogError("Scene::CreateContentFromXml: Parsing scene XML from text failed");
        return ret;
    }

    return CreateContentFromXml(scene_doc, useEntityIDsFromFile, change);
}

Vector<Entity *> Scene::CreateContentFromXml(Urho3D::XMLFile &xml, bool useEntityIDsFromFile, AttributeChange::Type change)
{
    if (!IsAuthority() && parentTracker_.IsTracking())
    {
        LogError("Scene::CreateContentFromXml: Still waiting for previous content creation to complete on the server. Try again after it completes.");
        return Vector<Entity*>();
    }

    /// @todo Vector was observed to be slighty more optimal here, but going with Vector for now
    /// in order to clean up the very messy Vector/Vector<Entity*/EntityWeakPtr/EntityPtr> conversions.
    Vector<EntityWeakPtr> entities;
    
    // Check for existence of the scene element before we begin
    Urho3D::XMLElement scene_elem = xml.GetRoot("scene");
    if (!scene_elem)
    {
        LogError("Scene::CreateContentFromXml: Could not find 'scene' element from XML.");
        return Vector<Entity*>();
    }

    // Create all storages fro the scene file.
    Urho3D::XMLElement storage_elem = scene_elem.GetChild("storage");
    while(storage_elem)
    {
        framework_->Asset()->DeserializeAssetStorageFromString(framework_->ParseWildCardFilename(storage_elem.GetAttribute("specifier")), false);
        storage_elem = storage_elem.GetNext("storage");
    }
    
    EntityIdMap oldToNewIds;

    // Spawn all entities in the scene storage.
    Urho3D::XMLElement ent_elem = scene_elem.GetChild("entity");
    while(ent_elem)
    {
        CreateEntityFromXml(EntityPtr(), ent_elem, useEntityIDsFromFile, change, entities, oldToNewIds);
        ent_elem = ent_elem.GetNext("entity");
    }

    // Sort the entity list so that parents are before children.
    // This is done to combat the runtime detection of "parent entity/placeable created"
    // that slows down the import considerably on large scenes that relies heavily on parenting.
    // Note: Unlike CreateContentFromSceneDesc this is done post Entity creation as we don't have full
    // information prior to it. This will create the entities but the actual signaling happens below,
    // so sorting here still makes a difference.
    Vector<EntityWeakPtr> sortedDescEntities = SortEntities(entities);

    // Fix parent ref of Placeable if new entity IDs were generated.
    // This should be done first so that we wont be firing signals
    // with partially updated state (these ends are already in the scene for querying).
    if (!useEntityIDsFromFile)
        FixPlaceableParentIds(sortedDescEntities, oldToNewIds, AttributeChange::Disconnected);

    // Now that we have each entity spawned to the scene, trigger all the signals for EntityCreated/ComponentChanged messages.
    for(u32 i=0, len=sortedDescEntities.Size(); i<len; ++i)
    {
        EntityWeakPtr weakEnt = sortedDescEntities[i];

        // On a client start tracking of the server ack messages.
        if (!IsAuthority() && !weakEnt.Expired())
            parentTracker_.Track(weakEnt.Get());

        if (!weakEnt.Expired())
            EmitEntityCreated(weakEnt.Get(), change);
        if (!weakEnt.Expired())
        {
            EntityPtr entityShared = weakEnt.Lock();
            const Entity::ComponentMap &components = entityShared->Components();
            for (Entity::ComponentMap::ConstIterator it = components.Begin(); it != components.End(); ++it)
                it->second_->ComponentChanged(change);
        }
    }
    
    // The above signals may have caused scripts to remove entities. Return those that still exist.
    Vector<Entity *> ret;
    ret.Reserve(sortedDescEntities.Size());
    for(int i=0, len=sortedDescEntities.Size(); i<len; ++i)
    {
        if (!sortedDescEntities[i].Expired())
            ret.Push(sortedDescEntities[i].Get());
    }
    
    return ret;
}

void Scene::CreateEntityFromXml(EntityPtr parent, const Urho3D::XMLElement& ent_elem, bool useEntityIDsFromFile,
    AttributeChange::Type change, Vector<EntityWeakPtr>& entities, EntityIdMap& oldToNewIds)
{
    const bool replicated = ent_elem.HasAttribute("sync") ? ent_elem.GetBool("sync") : true;
    const bool entityTemporary = ent_elem.GetBool("temporary");

    entity_id_t id = ent_elem.GetUInt("id"); // toUInt() return 0 on failure.
    if (!useEntityIDsFromFile || id == 0) // If we don't want to use entity IDs from file, or if file doesn't contain one, generate a new one.
    {
        entity_id_t originaId = id;
        id = replicated ? NextFreeId() : NextFreeIdLocal();
        if (originaId != 0 && !oldToNewIds.Contains(originaId))
            oldToNewIds[originaId] = id;
    }
    else if (useEntityIDsFromFile && HasEntity(id)) // If we use IDs from file and they conflict with some of the existing IDs, change the ID of the old entity
    {
        entity_id_t newID = replicated ? NextFreeId() : NextFreeIdLocal();
        ChangeEntityId(id, newID);
    }

    if (HasEntity(id)) // If the entity we are about to add conflicts in ID with an existing entity in the scene, delete the old entity.
    {
        LogDebug("Scene::CreateContentFromXml: Destroying previous entity with id " + String(id) + " to avoid conflict with new created entity with the same id.");
        LogError("Warning: Invoking buggy behavior: Object with id " + String(id) +" might not replicate properly!");
        RemoveEntity(id, AttributeChange::Replicate); ///<@todo Consider do we want to always use Replicate
    }

    EntityPtr entity;
    if (!parent)
        entity = CreateEntity(id);
    else
        entity = parent->CreateChild(id);

    if (entity)
    {
        entity->SetTemporary(entityTemporary);

        Urho3D::XMLElement comp_elem = ent_elem.GetChild("component");
        while (comp_elem)
        {
            const String typeName = comp_elem.GetAttribute("type");
            const u32 typeId = comp_elem.GetUInt("typeId");
            const String name = comp_elem.GetAttribute("name");
            const bool compReplicated = comp_elem.HasAttribute("sync") ? comp_elem.GetBool("sync") : true;
            const bool compTemporary = comp_elem.GetBool("temporary");

            // If we encounter an unknown component type, now is the time to register a placeholder type for it
            // The XML holds all needed data for it, while binary doesn't
            SceneAPI* sceneAPI = framework_->Scene();
            if (!sceneAPI->IsComponentTypeRegistered(typeName))
                sceneAPI->RegisterPlaceholderComponentType(comp_elem);
            
            ComponentPtr new_comp = (!typeName.Empty() ? entity->GetOrCreateComponent(typeName, name, AttributeChange::Default, compReplicated) :
                entity->GetOrCreateComponent(typeId, name, AttributeChange::Default, compReplicated));
            if (new_comp)
            {
                new_comp->SetTemporary(compTemporary);
                new_comp->DeserializeFrom(comp_elem, AttributeChange::Disconnected);// Trigger no signal yet when scene is in incoherent state
            }

            comp_elem = comp_elem.GetNext("component");
        }
        entities.Push(entity);
    }
    else
    {
        LogError("Scene::CreateContentFromXml: Failed to create entity with id " + String(id) + "!");
    }

    // Spawn any child entities
    Urho3D::XMLElement childEnt_elem = ent_elem.GetChild("entity");
    while (childEnt_elem)
    {
        CreateEntityFromXml(entity, childEnt_elem, useEntityIDsFromFile, change, entities, oldToNewIds);
        childEnt_elem = childEnt_elem.GetNext("entity");
    }
}

Vector<Entity *> Scene::CreateContentFromBinary(const String &filename, bool useEntityIDsFromFile, AttributeChange::Type change)
{
    Urho3D::File file(context_);
    if (!file.Open(filename, Urho3D::FILE_READ))
    {
        LogError("Scene::LoadSceneBinary: Failed to open file " + filename + ".");
        return Vector<Entity*>();
    }

    if (!file.GetSize())
    {
        LogError("Scene::LoadSceneBinary: File " + filename + " contained 0 bytes when loading scene binary.");
        return Vector<Entity*>();
    }

    PODVector<char> bytes;
    bytes.Resize(file.GetSize());
    file.Read(&bytes[0], bytes.Size());
    file.Close();

    return CreateContentFromBinary(&bytes[0], bytes.Size(), useEntityIDsFromFile, change);
}

Vector<Entity *> Scene::CreateContentFromBinary(const char *data, int numBytes, bool useEntityIDsFromFile, AttributeChange::Type change)
{
    if (!IsAuthority() && parentTracker_.IsTracking())
    {
        LogError("Scene::CreateContentFromBinary: Still waiting for previous content creation to complete on the server. Try again after it completes.");
        return Vector<Entity*>();
    }

    assert(data);
    assert(numBytes > 0);

    Vector<EntityWeakPtr> entities;
    EntityIdMap oldToNewIds;

    try
    {
        DataDeserializer source(data, numBytes);

        uint num_entities = source.Read<u32>();
        for(uint i = 0; i < num_entities; ++i)
            CreateEntityFromBinary(EntityPtr(), source, useEntityIDsFromFile, change, entities, oldToNewIds);
    }
    catch(...)
    {
        // Note: if exception happens, no change signals are emitted
        return Vector<Entity *>();
    }

    // Fix parent ref of Placeable if new entity IDs were generated.
    // This should be done first so that we wont be firing signals
    // with partially updated state (these ends are already in the scene for querying).
    if (!useEntityIDsFromFile)
        FixPlaceableParentIds(entities, oldToNewIds, AttributeChange::Disconnected);

    // Now that we have each entity spawned to the scene, trigger all the signals for EntityCreated/ComponentChanged messages.
    for(u32 i = 0; i < entities.Size(); ++i)
    {
        EntityWeakPtr weakEnt = entities[i];

        // On a client start tracking of the server ack messages.
        if (!IsAuthority() && !weakEnt.Expired())
            parentTracker_.Track(weakEnt.Get());

        if (!weakEnt.Expired())
            EmitEntityCreated(weakEnt.Get(), change);
        if (!weakEnt.Expired())
        {
            EntityPtr entityShared = weakEnt.Lock();
            const Entity::ComponentMap &components = entityShared->Components();
            for (Entity::ComponentMap::ConstIterator it = components.Begin(); it != components.End(); ++it)
                it->second_->ComponentChanged(change);
        }
    }
    
    // The above signals may have caused scripts to remove entities. Return those that still exist.
    Vector<Entity *> ret;
    ret.Reserve(entities.Size());
    for(u32 i = 0; i < entities.Size(); ++i)
        if (!entities[i].Expired())
            ret.Push(entities[i].Get());

    return ret;
}

void Scene::CreateEntityFromBinary(EntityPtr parent, kNet::DataDeserializer& source, bool useEntityIDsFromFile,
    AttributeChange::Type change, Vector<EntityWeakPtr>& entities, EntityIdMap& oldToNewIds)
{
    entity_id_t id = source.Read<u32>();
    bool replicated = source.Read<u8>() ? true : false;
    if (!useEntityIDsFromFile || id == 0)
    {
        entity_id_t originalId = id;
        id = replicated ? NextFreeId() : NextFreeIdLocal();
        if (originalId != 0 && !oldToNewIds.Contains(originalId))
            oldToNewIds[originalId] = id;
    }
    else if (useEntityIDsFromFile && HasEntity(id))
    {
        entity_id_t newID = replicated ? NextFreeId() : NextFreeIdLocal();
        ChangeEntityId(id, newID);
    }

    if (HasEntity(id)) // If the entity we are about to add conflicts in ID with an existing entity in the scene.
    {
        LogDebug("Scene::CreateContentFromBinary: Destroying previous entity with id " + String(id) + " to avoid conflict with new created entity with the same id.");
        LogError("Warning: Invoking buggy behavior: Object with id " + String(id) + "might not replicate properly!");
        RemoveEntity(id, AttributeChange::Replicate); ///<@todo Consider do we want to always use Replicate
    }

    EntityPtr entity;
    if (!parent)
        entity = CreateEntity(id);
    else
        entity = parent->CreateChild(id);

    if (!entity)
    {
        LogError("Scene::CreateEntityFromBinary: Failed to create entity.");
        return;
    }
    
    uint num_components = source.Read<u32>();
    uint num_childEntities = num_components >> 16;
    num_components &= 0xffff;

    for(uint i = 0; i < num_components; ++i)
    {
        u32 typeId = source.Read<u32>(); ///\todo VLE this!
        String name = String(source.ReadString().c_str());
        bool compReplicated = source.Read<u8>() ? true : false;
        uint data_size = source.Read<u32>();

        // Read the component data into a separate byte array, then deserialize from there.
        // This way the whole stream should not desync even if something goes wrong
        PODVector<unsigned char> comp_bytes;
        comp_bytes.Resize(data_size);
        if (data_size)
            source.ReadArray<u8>((u8*)&comp_bytes[0], comp_bytes.Size());
        
        try
        {
            ComponentPtr new_comp = entity->GetOrCreateComponent(typeId, name, AttributeChange::Default, compReplicated);
            if (new_comp)
            {
                if (data_size)
                {
                    DataDeserializer comp_source((char*)&comp_bytes[0], comp_bytes.Size());
                    // Trigger no signal yet when scene is in incoherent state
                    new_comp->DeserializeFromBinary(comp_source, AttributeChange::Disconnected);
                }
            }
            else
                LogError("Scene::CreateEntityFromBinary: Failed to load component \"" + framework_->Scene()->ComponentTypeNameForTypeId(typeId) + "\"!");
        }
        catch(...)
        {
            LogError("Scene::CreateEntityFromBinary: Failed to load component \"" + framework_->Scene()->ComponentTypeNameForTypeId(typeId) + "\"!");
        }
    }

    entities.Push(entity);

    for (uint i = 0; i < num_childEntities; ++i)
        CreateEntityFromBinary(entity, source, useEntityIDsFromFile, change, entities, oldToNewIds);
}

Vector<Entity *> Scene::CreateContentFromSceneDesc(const SceneDesc &desc, bool useEntityIDsFromFile, AttributeChange::Type change)
{
    Vector<Entity *> ret;
    if (desc.entities.Empty())
    {
        LogError("Scene::CreateContentFromSceneDesc: Empty scene description.");
        return ret;
    }
    if (!IsAuthority() && parentTracker_.IsTracking())
    {
        LogError("Scene::CreateContentFromSceneDesc: Still waiting for previous content creation to complete on the server. Try again after it completes.");
        return ret;
    }

    // Sort the entity list so that parents are before children.
    // This is done to combat the runtime detection of "parent entity/placeable created"
    // that slows down the import considerably on large scenes that relies heavily on parenting.
    EntityDescList sortedDescEntities = SortEntities(desc.entities);

    EntityIdMap oldToNewIds;
    for (int ei=0, eilen=sortedDescEntities.Size(); ei<eilen; ++ei)
        CreateEntityFromDesc(EntityPtr(), sortedDescEntities[ei], useEntityIDsFromFile, change, ret, oldToNewIds);

    // Fix parent ref of Placeable if new entity IDs were generated.
    // This should be done first so that we wont be firing signals
    // with partially updated state (these ends are already in the scene for querying).
    if (!useEntityIDsFromFile)
        FixPlaceableParentIds(ret, oldToNewIds, AttributeChange::Disconnected);

    // All entities & components have been loaded. Trigger change for them now.
    foreach(Entity *entity, ret)
    {
        // On a client start tracking of the server ack messages.
        if (!IsAuthority())
            parentTracker_.Track(entity);

        // Entity
        EmitEntityCreated(entity, change);

        // Components
        const Entity::ComponentMap &components = entity->Components();
        for(Entity::ComponentMap::ConstIterator i = components.Begin(); i != components.End(); ++i)
            i->second_->ComponentChanged(change);
    }

    return ret;
}

void Scene::CreateEntityFromDesc(EntityPtr parent, const EntityDesc& e, bool useEntityIDsFromFile,
    AttributeChange::Type change, Vector<Entity *>& entities, EntityIdMap& oldToNewIds)
{
    entity_id_t id = Urho3D::ToUInt(e.id);

    if (e.id.Empty() || !useEntityIDsFromFile)
    {
        entity_id_t originaId = id;
        id = e.local ? NextFreeIdLocal() : NextFreeId();
        if (originaId != 0 && !oldToNewIds.Contains(originaId))
            oldToNewIds[originaId] = id;
    }

    if (HasEntity(id)) // If the entity we are about to add conflicts in ID with an existing entity in the scene.
    {
        LogDebug("Scene::CreateEntityFromDesc: Destroying previous entity with id " + String(id) + " to avoid conflict with new created entity with the same id.");
        LogError("Warning: Invoking buggy behavior: Object with id " + String(id) + " might not replicate properly!");
        RemoveEntity(id, AttributeChange::Replicate); ///<@todo Consider do we want to always use Replicate
    }

    EntityPtr entity;
    if (!parent)
        entity = CreateEntity(id);
    else
        entity = parent->CreateChild(id);

    assert(entity);
    if (entity)
    {
        foreach(const ComponentDesc &c, e.components)
        {
            if (c.typeName.Empty())
                continue;

            // If we encounter an unknown component type, now is the time to register a placeholder type for it
            // The componentdesc holds all needed data for it
            SceneAPI* sceneAPI = framework_->Scene();
            if (!sceneAPI->IsComponentTypeRegistered(c.typeName))
                sceneAPI->RegisterPlaceholderComponentType(c);

            ComponentPtr comp = entity->GetOrCreateComponent(c.typeName, c.name);
            assert(comp);
            if (!comp)
            {
                LogError("Scene::CreateEntityFromDesc: failed to create component " + c.typeName + " " + c.name);
                continue;
            }
            if (comp->TypeId() == 25 /*DynamicComponent*/)
            {
                Urho3D::XMLFile temp_doc(context_);
                Urho3D::XMLElement root_elem = temp_doc.CreateRoot("component");
                root_elem.SetUInt("typeId", c.typeId); // Ambiguous on VC9 as u32
                root_elem.SetAttribute("type", c.typeName);
                root_elem.SetAttribute("name", c.name);
                root_elem.SetBool("sync", c.sync);
                foreach(const AttributeDesc &a, c.attributes)
                {
                    Urho3D::XMLElement child_elem = root_elem.CreateChild("attribute");
                    child_elem.SetAttribute("id", a.id);
                    child_elem.SetAttribute("value", a.value);
                    child_elem.SetAttribute("type", a.typeName);
                    child_elem.SetAttribute("name", a.name);
                }
                comp->DeserializeFrom(root_elem, AttributeChange::Default);
            }
            else
            {
                foreach(IAttribute *attr, comp->Attributes())
                {
                    if (!attr)
                        continue;
                    foreach(const AttributeDesc &a, c.attributes)
                    {
                        if (attr->TypeName().Compare(a.typeName, false) == 0 &&
                            (attr->Id().Compare(a.id, false) == 0 ||
                             attr->Name().Compare(a.name, false) == 0))
                        {
                           attr->FromString(a.value, AttributeChange::Disconnected); // Trigger no signal yet when scene is in incoherent state
                        }
                    }
                }
            }
        }

        entity->SetTemporary(e.temporary);
        entities.Push(entity.Get());

        // Create child entities recursively
        foreach(const EntityDesc &ce, e.children)
            CreateEntityFromDesc(entity, ce, useEntityIDsFromFile, change, entities, oldToNewIds);
    }
}

SceneDesc Scene::CreateSceneDescFromXml(const String &filename) const
{
    SceneDesc sceneDesc(filename);
    if (!filename.EndsWith(".txml", false))
    {
        if (filename.EndsWith(".tbin", false))
            LogError("Scene::CreateSceneDescFromXml: Try using CreateSceneDescFromBinary() instead for " + filename);
        else
            LogError("Scene::CreateSceneDescFromXml: Unsupported file extension " + filename + " when trying to create scene description.");
        return sceneDesc;
    }

    Urho3D::File file(context_);
    if (!file.Open(filename, Urho3D::FILE_READ))
    {
        LogError("Scene::CreateSceneDescFromXml: Failed to open file " + filename + ".");
        return sceneDesc;
    }

    if (!file.GetSize())
    {
        LogError("Scene::LoadSceneBinary: File " + filename + " contained 0 bytes when loading scene binary.");
        return sceneDesc;
    }

    String xmlData = file.ReadString();
    file.Close();

    return CreateSceneDescFromXml(xmlData, sceneDesc);
}

SceneDesc Scene::CreateSceneDescFromXml(const String &data, SceneDesc &sceneDesc) const
{
    Urho3D::XMLFile scene_doc(context_);
    if (!scene_doc.FromString(data))
    {
        LogError("Scene::CreateSceneDescFromXml: Parsing scene XML from " + sceneDesc.filename + " failed when loading Scene XML");
        return sceneDesc;
    }

    // Check for existence of the scene element before we begin
    Urho3D::XMLElement scene_elem = scene_doc.GetRoot("scene");
    if (!scene_elem)
    {
        LogError("Scene::CreateSceneDescFromXml: Could not find 'scene' element from XML.");
        return sceneDesc;
    }

    Urho3D::XMLElement ent_elem = scene_elem.GetChild("entity");
    while(ent_elem)
    {
        CreateEntityDescFromXml(sceneDesc, sceneDesc.entities, ent_elem);
        // Process siblings.
        ent_elem = ent_elem.GetNext("entity");
    }

    return sceneDesc;
}

void Scene::CreateEntityDescFromXml(SceneDesc& sceneDesc, Vector<EntityDesc>& dest, const Urho3D::XMLElement& ent_elem) const
{
    String id_str = ent_elem.GetAttribute("id");
    if (id_str.Empty())
        return;
    
    EntityDesc entityDesc;
    entityDesc.id = id_str;
    entityDesc.local = false;
    if (ent_elem.HasAttribute("sync"))
        entityDesc.local = !ent_elem.GetBool("sync"); /**< @todo if no "sync"* attr, deduct from the ID. */
    entityDesc.temporary = ent_elem.GetBool("temporary");

    Urho3D::XMLElement comp_elem = ent_elem.GetChild("component");
    while(comp_elem)
    {
        ComponentDesc compDesc;
        compDesc.typeName = comp_elem.GetAttribute("type");
        compDesc.typeId = 0xffffffff;
        if (comp_elem.GetUInt("typeId"))
            compDesc.typeId = comp_elem.GetUInt("typeId");
        /// @todo 27.09.2013 assert that typeName and typeId match
        /// @todo 27.09.2013 If mismatch, show warning, and use SceneAPI's
        /// ComponentTypeNameForTypeId and ComponentTypeIdForTypeName to resolve one or the other?
        compDesc.name = comp_elem.GetAttribute("name");
        compDesc.sync = comp_elem.GetBool("sync");
        const bool hasTypeId = compDesc.typeId != 0xffffffff;

        // A bit of a hack to get the name from Name.
        if (entityDesc.name.Empty() && (compDesc.typeId == Name::ComponentTypeId ||
            IComponent::EnsureTypeNameWithoutPrefix(compDesc.typeName) == Name::TypeNameStatic()))
        {
            ComponentPtr comp = (hasTypeId ? framework_->Scene()->CreateComponentById(0, compDesc.typeId, compDesc.name) :
                framework_->Scene()->CreateComponentByName(0, compDesc.typeName, compDesc.name));
            Tundra::Name *ecName = static_cast<Tundra::Name*>(comp.Get());
            ecName->DeserializeFrom(comp_elem, AttributeChange::Disconnected);
            entityDesc.name = ecName->name.Get();
            entityDesc.group = ecName->group.Get();
        }

        ComponentPtr comp = (hasTypeId ? framework_->Scene()->CreateComponentById(0, compDesc.typeId, compDesc.name) :
            framework_->Scene()->CreateComponentByName(0, compDesc.typeName, compDesc.name));
        if (!comp) // Move to next element if component creation fails.
        {
            comp_elem = comp_elem.GetNext("component");
            continue;
        }

        // Find asset references.
        comp->DeserializeFrom(comp_elem, AttributeChange::Disconnected);
        foreach(IAttribute *a,comp->Attributes())
        {
            if (!a)
                continue;
                    
            const String typeName = a->TypeName();
            AttributeDesc attrDesc = { typeName, a->Name(), a->ToString(), a->Id() };
            compDesc.attributes.Push(attrDesc);

            String attrValue = a->ToString();
            if ((typeName.Compare("AssetReference", false) == 0 || typeName.Compare("AssetReferenceList", false) == 0|| 
                (a->Metadata() && a->Metadata()->elementType.Compare("AssetReference", false) == 0)) &&
                !attrValue.Empty())
            {
                // We might have multiple references, ";" used as a separator.
                StringVector assetRefs = attrValue.Split(';');
                for (int avi=0, avilen=assetRefs.Size(); avi<avilen; ++avi)
                {
                    const String &assetRef = assetRefs[avi];

                    AssetDesc ad;
                    ad.typeName = a->Name();

                    // Resolve absolute file path for asset reference and the destination name (just the filename).
                    if (!sceneDesc.assetCache.Fill(assetRef, ad))
                    {
                        framework_->Asset()->ResolveLocalAssetPath(assetRef, sceneDesc.assetCache.basePath, ad.source);
                        ad.destinationName = AssetAPI::ExtractFilenameFromAssetRef(ad.source);
                        sceneDesc.assetCache.Add(assetRef, ad);
                    }

                    sceneDesc.assets[MakePair(ad.source, ad.subname)] = ad;

                    /// \todo Implement elsewhere
                    // If this is a script, look for dependecies
                    //if (ad.source.ToLower().EndsWith(".js"))
                    //    SearchScriptAssetDependencies(ad.source, sceneDesc);
                } 
            }
        }

        entityDesc.components.Push(compDesc);

        comp_elem = comp_elem.GetNext("component");
    }

    // Process child entities
    Urho3D::XMLElement childEnt_elem = ent_elem.GetChild("entity");
    while (childEnt_elem)
    {
        CreateEntityDescFromXml(sceneDesc, entityDesc.children, childEnt_elem);
        childEnt_elem = childEnt_elem.GetNext("entity");
    }

    dest.Push(entityDesc);
}

SceneDesc Scene::CreateSceneDescFromBinary(const String &filename) const
{
    SceneDesc sceneDesc(filename);

    if (!filename.EndsWith(".tbin", false))
    {
        if (filename.EndsWith(".txml", false))
            LogError("Scene::CreateSceneDescFromBinary: Try using CreateSceneDescFromXml() instead for " + filename);
        else
            LogError("Scene::CreateSceneDescFromBinary: Unsupported file extension : " + filename + " when trying to create scene description.");
        return sceneDesc;
    }

    Urho3D::File file(context_);
    if (!file.Open(filename, Urho3D::FILE_READ))
    {
        LogError("Scene::CreateSceneDescFromBinary: Failed to open file " + filename + " when trying to create scene description.");
        return sceneDesc;
    }

    PODVector<unsigned char> bytes;
    bytes.Resize(file.GetSize());
    if (bytes.Size())
        file.Read(&bytes[0], bytes.Size());
    file.Close();

    return CreateSceneDescFromBinary(bytes, sceneDesc);
}

SceneDesc Scene::CreateSceneDescFromBinary(PODVector<unsigned char> &data, SceneDesc &sceneDesc) const
{
    if (!data.Size())
    {
        LogError("Scene::CreateSceneDescFromBinary: File " + sceneDesc.filename + " contained 0 bytes when trying to create scene description.");
        return sceneDesc;
    }

    try
    {
        DataDeserializer source((const char*)&data[0], data.Size());
        
        const uint num_entities = source.Read<u32>();
        for(uint i = 0; i < num_entities; ++i)
        {
            EntityDesc entityDesc;
            entity_id_t id = source.Read<u32>();
            entityDesc.id = String(id);

            const uint num_components = source.Read<u32>();
            for(uint j = 0; j < num_components; ++j)
            {
                SceneAPI *sceneAPI = framework_->Scene();

                ComponentDesc compDesc;
                compDesc.typeId = source.Read<u32>(); /**< @todo VLE this! */
                compDesc.typeName = sceneAPI->ComponentTypeNameForTypeId(compDesc.typeId);
                compDesc.name = String(source.ReadString().c_str());
                compDesc.sync = source.Read<u8>() ? true : false;
                uint data_size = source.Read<u32>();

                // Read the component data into a separate byte array, then deserialize from there.
                // This way the whole stream should not desync even if something goes wrong
                PODVector<unsigned char> comp_bytes;
                comp_bytes.Resize(data_size);
                if (data_size)
                    source.ReadArray<u8>((u8*)&comp_bytes[0], comp_bytes.Size());

                try
                {
                    ComponentPtr comp = sceneAPI->CreateComponentById(0, compDesc.typeId, compDesc.name);
                    if (comp)
                    {
                        if (data_size)
                        {
                            DataDeserializer comp_source((const char*)&comp_bytes[0], comp_bytes.Size());
                            // Trigger no signal yet when scene is in incoherent state
                            comp->DeserializeFromBinary(comp_source, AttributeChange::Disconnected);
                            foreach(IAttribute *a, comp->Attributes())
                            {
                                if (!a)
                                    continue;
                                
                                String typeName = a->TypeName();
                                AttributeDesc attrDesc = { typeName, a->Name(), a->ToString(), a->Id() };
                                compDesc.attributes.Push(attrDesc);

                                String attrValue = a->ToString();
                                if ((typeName.Compare("AssetReference", false) == 0 || typeName.Compare("AssetReferenceList", false) == 0 || 
                                    (a->Metadata() && a->Metadata()->elementType.Compare("AssetReference", false) == 0)) &&
                                    !attrValue.Empty())
                                {
                                    // We might have multiple references, ";" used as a separator.
                                    StringVector assetRefs = attrValue.Split(';');
                                    for (int avi=0, avilen=assetRefs.Size(); avi<avilen; ++avi)
                                    {
                                        const String &assetRef = assetRefs[avi];

                                        AssetDesc ad;
                                        ad.typeName = a->Name();

                                        // Resolve absolute file path for asset reference and the destination name (just the filename).
                                        if (!sceneDesc.assetCache.Fill(assetRef, ad))
                                        {
                                            framework_->Asset()->ResolveLocalAssetPath(assetRef, sceneDesc.assetCache.basePath, ad.source);
                                            ad.destinationName = AssetAPI::ExtractFilenameFromAssetRef(ad.source);
                                            sceneDesc.assetCache.Add(assetRef, ad);
                                        }

                                        sceneDesc.assets[MakePair(ad.source, ad.subname)] = ad;

                                        /// \todo Implement elsewhere
                                        // If this is a script, look for dependecies
                                        //if (ad.source.ToLower().EndsWith(".js"))
                                        //    SearchScriptAssetDependencies(ad.source, sceneDesc);
                                    }
                                }
                            }
                        }

                        entityDesc.components.Push(compDesc);
                    }
                    else
                    {
                        LogError("Scene::CreateSceneDescFromBinary: Failed to load component " + compDesc.typeName + " " + compDesc.name);
                    }
                }
                catch(...)
                {
                    LogError("Scene::CreateSceneDescFromBinary: Exception while trying to load component " + compDesc.typeName + " " + compDesc.name);
                }
            }

            sceneDesc.entities.Push(entityDesc);
        }
    }
    catch(...)
    {
        return SceneDesc("");
    }

    return sceneDesc;
}

float3 Scene::UpVector() const
{
    return float3::unitY;
}

float3 Scene::RightVector() const
{
    return float3::unitX;
}

float3 Scene::ForwardVector() const
{
    return -float3::unitZ;
}

bool Scene::StartAttributeInterpolation(IAttribute* attr, IAttribute* endvalue, float length)
{
    if (!endvalue)
        return false;
    
    IComponent* comp = attr ? attr->Owner() : 0;
    Entity* entity = comp ? comp->ParentEntity() : 0;
    Scene* scene = entity ? entity->ParentScene() : 0;
    
    if (length <= 0.0f || !attr || !attr->Metadata() || attr->Metadata()->interpolation == AttributeMetadata::None ||
        !comp || !entity || !scene || scene != this)
    {
        delete endvalue;
        return false;
    }
    
    // End previous interpolation if existed
    bool previous = EndAttributeInterpolation(attr);
    
    // If previous interpolation does not exist, perform a direct snapping to the end value
    // but still start an interpolation period, so that on the next update we detect that an interpolation is going on,
    // and will interpolate normally
    if (!previous)
        attr->CopyValue(endvalue, AttributeChange::LocalOnly);
    
    AttributeInterpolation newInterp;
    newInterp.dest = AttributeWeakPtr(comp, attr);
    newInterp.start = AttributeWeakPtr(comp, attr->Clone());
    newInterp.end = AttributeWeakPtr(comp, endvalue);
    newInterp.length = length;
    
    interpolations_.Push(newInterp);
    return true;
}

bool Scene::EndAttributeInterpolation(IAttribute* attr)
{
    for(uint i = 0; i < interpolations_.Size(); ++i)
    {
        AttributeInterpolation& interp = interpolations_[i];
        if (interp.dest.Get() == attr)
        {
            delete interp.start.Get();
            delete interp.end.Get();
            interpolations_.Erase(interpolations_.Begin() + i);
            return true;
        }
    }
    return false;
}

void Scene::EndAllAttributeInterpolations()
{
    for(uint i = 0; i < interpolations_.Size(); ++i)
    {
        AttributeInterpolation& interp = interpolations_[i];
        delete interp.start.Get();
        delete interp.end.Get();
    }
    
    interpolations_.Clear();
}

void Scene::UpdateAttributeInterpolations(float frametime)
{
    PROFILE(Scene_UpdateInterpolation);
    
    interpolating_ = true;
    
    for(uint i = interpolations_.Size() - 1; i < interpolations_.Size(); --i)
    {
        AttributeInterpolation& interp = interpolations_[i];
        bool finished = false;
        
        // Check that the component still exists i.e. it's safe to access the attribute
        if (!interp.start.owner.Expired())
        {
            // Allow the interpolation to persist for 2x time, though we are no longer setting the value
            // This is for the continuous/discontinuous update detection in StartAttributeInterpolation()
            if (interp.time <= interp.length)
            {
                interp.time += frametime;
                float t = interp.time / interp.length;
                if (t > 1.0f)
                    t = 1.0f;
                interp.dest.Get()->Interpolate(interp.start.Get(), interp.end.Get(), t, AttributeChange::LocalOnly);
            }
            else
            {
                interp.time += frametime;
                if (interp.time >= interp.length * 2.0f)
                    finished = true;
            }
        }
        else // Component pointer has expired, abort this interpolation
            finished = true;
        
        // Remove interpolation (& delete start/endpoints) when done
        if (finished)
        {
            delete interp.start.Get();
            delete interp.end.Get();
            interpolations_.Erase(interpolations_.Begin() + i);
        }
    }

    interpolating_ = false;
}

void Scene::OnUpdated(float /*frameTime*/)
{
    // Signal queued entity creations now
    for (unsigned i = 0; i < entitiesCreatedThisFrame_.Size(); ++i)
    {
        Entity* entity = entitiesCreatedThisFrame_[i].first_.Get();
        if (!entity)
            continue;
        
        AttributeChange::Type change = entitiesCreatedThisFrame_[i].second_;
        if (change == AttributeChange::Disconnected)
            continue;
        if (change == AttributeChange::Default)
            change = AttributeChange::Replicate;
        
        EntityCreated.Emit(entity, change);
    }
    
    entitiesCreatedThisFrame_.Clear();
}

EntityVector Scene::FindEntitiesContaining(const String &substring, bool caseSensitivity) const
{
    EntityVector entities;
    if (substring.Empty())
        return entities;

    for(ConstIterator it = Begin(); it != End(); ++it)
    {
        EntityPtr entity = it->second_;
        if (entity->Name().Contains(substring, caseSensitivity))
            entities.Push(entity);
    }

    return entities;
}

EntityVector Scene::FindEntitiesByName(const String &name, bool caseSensitivity) const
{
    // Don't check if name is empty, we want to allow querying for all entities without a name too.
    EntityVector entities;
    for(ConstIterator it = Begin(); it != End(); ++it)
    {
        EntityPtr entity = it->second_;
        if (entity->Name().Compare(name, caseSensitivity) == 0)
            entities.Push(entity);
    }

    return entities;
}

EntityVector Scene::RootLevelEntities() const
{
    EntityVector entities;
    for(ConstIterator it = Begin(); it != End(); ++it)
    {
        EntityPtr entity = it->second_;
        if (!entity->Parent())
            entities.Push(entity);
    }

    return entities;
}

// Returns insert index to 'container' for 'ent'. Return -1 if can be appended as last element.
// Note: At the moment the Entity parenting with both Entity and Placable level is so complex that this proably
// will not handle all exotic deep recursions correctly. In the case of Placeable parenting children may come before parent
// in 'container'. In the case of Entity level parenting the 'container' can be assumed to be in the correct order already.
// Todo: Deeper Placeable hierarchy inspection to fix above note.
template <typename T>
int EntityInsertIndex(const Scene *scene, const Entity *ent, T &container)
{
    int insertIndex = -1;
    if (!ent)
        return insertIndex;

    entity_id_t parentId = scene->EntityParentId(ent);
    
    // Has children
    if (ent->NumChildren() > 0)
    {
        // Parented parent
        if (parentId > 0)
        {
            for (int sorti=0, sortlen=container.Size(); sorti<sortlen; ++sorti)
            {
                // Todo: Inspect if parent is one of children of this entity?
                // Parent already in the container? Insert after it.
                if (container[sorti]->Id() == parentId)
                {
                    insertIndex = sorti + 1;
                    break;
                }
            }
        }
        // - Parented parent where the parent was not already in container
        // - Unparented parent
        if (insertIndex == -1)
        {
            // Find first non-parent Entity and insert before it.
            for (int sorti=0, sortlen=container.Size(); sorti<sortlen; ++sorti)
            {
                if (container[sorti]->NumChildren() == 0)
                {
                    insertIndex = sorti;
                    break;
                }
            }
        }
    }
    // No children
    else
    {
        if (parentId > 0)
        {
            for (int sorti=0, sortlen=container.Size(); sorti<sortlen; ++sorti)
            {
                // Todo: Inspect if parent is one of children of this entity?
                // Parent already in the container? Insert after it.
                if (container[sorti]->Id() == parentId)
                {
                    insertIndex = sorti + 1;
                    break;
                }
            }
        }
        // - Parented entity where the parent was not already in container.
        // - Unparented entity
        if (insertIndex == -1)
        {
            // Find first non-parent + unparented Entity and insert after it.
            for (int sorti=0, sortlen=container.Size(); sorti<sortlen; ++sorti)
            {
                if (container[sorti]->NumChildren() == 0 && scene->EntityParentId(container[sorti]) == 0)
                {
                    insertIndex = sorti + 1;
                    break;
                }
            }
        }
    }
    return (insertIndex < (int)container.Size() ? insertIndex : -1);
}

Vector<EntityWeakPtr> Scene::SortEntities(const Vector<EntityWeakPtr> &entities) const
{
    Urho3D::HiresTimer t;

    Vector<Entity*> rawEntities;
    /// @todo In Scene's internal usage we know that nothing has could not have
    /// deleted the entities yet (no signals triggered) and this could be skipped.
    for (u32 ei=0, eilen=entities.Size(); ei<eilen; ++ei)
    {
        EntityWeakPtr weakEnt = entities[ei];
        if (weakEnt.Expired())
        {
            LogError("Scene::SortEntities: Input contained an expired WeakPtr at index " + String(ei) + ". Aborting sort and returning original list.");
            return entities;
        }
        rawEntities.Push(weakEnt.Get());
    }

    Vector<EntityWeakPtr> sortedEntities;
    for (int ei=0, eilen=entities.Size(); ei<eilen; ++ei)
    {
        EntityWeakPtr weakEnt = entities[ei];
        int insertIndex = EntityInsertIndex(this, weakEnt.Get(), rawEntities);
        if (insertIndex == -1 || insertIndex >= (int)sortedEntities.Size())
            sortedEntities.Push(weakEnt);
        else
            sortedEntities.Insert(insertIndex, weakEnt);
    }
    if (sortedEntities.Size() != entities.Size())
    {
        LogError("Scene::SortEntities: Sorting resulted in loss of information. Returning original unsorted list. Sorted size: " + String(sortedEntities.Size()) + " Unsorted size: " + String(entities.Size()));
        return entities;
    }

    LogDebug("Scene::SortEntities: Sorted Entities in " + String((int)(t.GetUSec(false)/1000)) + " msecs. Input Entities " + String(entities.Size()));
    return sortedEntities;
}

Vector<Entity*> Scene::SortEntities(const Vector<Entity*> &entities) const
{
    Urho3D::HiresTimer t;

    Vector<Entity*> sortedEntities;
    for (u32 ei=0, eilen=entities.Size(); ei<eilen; ++ei)
    {
        Entity *ent = entities[ei];
        if (!ent)
        {
            LogError("Scene::SortEntities: Input contained a null pointer at index " + String(ei) + ". Aborting sort and returning original list.");
            return entities;
        }

        int insertIndex = EntityInsertIndex(this, ent, sortedEntities);
        if (insertIndex == -1 || insertIndex >= (int)sortedEntities.Size())
            sortedEntities.Push(ent);
        else
            sortedEntities.Insert(insertIndex, ent);
    }
    if (sortedEntities.Size() != entities.Size())
    {
        LogError("Scene::SortEntities: Sorting resulted in loss of information. Returning original unsorted list. Sorted size: " + String(sortedEntities.Size()) + " Unsorted size: " + String(entities.Size()));
        return entities;
    }

    LogDebug("Scene::SortEntities: Sorted Entities in " + String((int)(t.GetUSec(false)/1000)) + " msecs. Input Entities " + String(entities.Size()));
    return sortedEntities;
}

EntityDescList Scene::SortEntities(const EntityDescList &entities) const
{
    Urho3D::HiresTimer t;

    EntityDescList iterDescEntities = entities;
    EntityDescList sortedDescEntities;

    // Put Entities that have children first (real Entity level parenting).
    for (u32 ei=0; ei<iterDescEntities.Size(); ++ei)
    {
        const EntityDesc &ent = iterDescEntities[ei];
        if (!ent.children.Empty())
        {
            // This does not yet mean its a root level Entity.
            // Check for deeper hierarchies: parent -> child with children.
            int insertIndex = -1;
            for (int sorti=0, sortlen=sortedDescEntities.Size(); sorti<sortlen; ++sorti)
            {
                const EntityDesc &parentCandidate = sortedDescEntities[sorti];
                if (parentCandidate.IsParentFor(ent))
                {
                    insertIndex = sorti + 1;
                    break;
                }
            }
            if (insertIndex == -1 || insertIndex >= (int)sortedDescEntities.Size())
                sortedDescEntities.Push(ent);
            else
                sortedDescEntities.Insert(insertIndex, ent);

            iterDescEntities.Erase(ei);
            ei--;
        }
    }

    // Find entities that have parent ref set (Placeable::parentRef parenting).
    int childrenStartIndex = sortedDescEntities.Size();
    for (int ei=0; ei < (int)iterDescEntities.Size(); ++ei)
    {
        int insertIndex = -1;
        const EntityDesc &ent = iterDescEntities[ei];

        entity_id_t parentId = PlaceableParentId(ent);
        if (parentId > 0)
        {
            // Find parent from the list and insert after, othewise add to end.
            String parentIdStr = String(parentId);
            for (int sorti=childrenStartIndex, sortlen=sortedDescEntities.Size(); sorti<sortlen; ++sorti)
            {
                const EntityDesc &parentCandidate = sortedDescEntities[sorti];
                if (parentCandidate.id == parentIdStr)
                {
                    insertIndex = sorti + 1;
                    break;
                }
            }
        }
        if (insertIndex == -1 || insertIndex >= (int)sortedDescEntities.Size())
            sortedDescEntities.Push(ent);
        else
            sortedDescEntities.Insert(insertIndex, ent);

        iterDescEntities.Erase(ei);
        ei--;
    }

    // Double check no information was lost. If these do not match, use the original passed in entity desc list.
    if (!iterDescEntities.Empty() || sortedDescEntities.Size() != entities.Size())
    {
        LogError("Scene::SortEntities: Sorting Entity hierarchy resulted in loss of information. "
            "Using original unsorted Entity list. Iteration list size: " + String(iterDescEntities.Size()) + 
            " Sorted Entities: " + String(sortedDescEntities.Size()) + " Original Entities: " + String(entities.Size()));
        return entities;
    }

    LogDebug("Scene::SortEntities: Sorted Entities in " + String((int)(t.GetUSec(false)/1000)) + " msecs. Input Entities " + String(entities.Size()));
    return sortedDescEntities;
}

entity_id_t Scene::EntityParentId(const Entity *ent) const
{
    if (ent->HasParent())
        return ent->Parent()->Id();
    return PlaceableParentId(ent);
}

entity_id_t Scene::PlaceableParentId(const Entity *ent) const
{
    ComponentPtr comp = ent->Component(20); // Placeable
    if (!comp)
        return 0;
    Attribute<EntityReference> *parentRef = static_cast<Attribute<EntityReference> *>(comp->AttributeById("parentRef"));
    if (!parentRef->Get().IsEmpty())
        return Urho3D::ToUInt(parentRef->Get().ref);
    return 0;
}

entity_id_t Scene::PlaceableParentId(const EntityDesc &ent) const
{
    // Find placeable typeId 20
    for (int ci=0, cilen=ent.components.Size(); ci<cilen; ++ci)
    {
        const ComponentDesc &comp = ent.components[ci];
        if (comp.typeId == 20)
        {
            // Find attribute "parentRef"
            const String parentRefId = "parentRef";
            for (int ai=0, ailen=comp.attributes.Size(); ai<ailen; ++ai)
            {
                const AttributeDesc &attr = comp.attributes[ai];
                if (attr.id.Compare(parentRefId, false) == 0)
                    return Urho3D::ToUInt(attr.value);
            }
            return 0; // parentRef not found for some reason (invalid desc data?);
        }
    }
    return 0;
}

uint Scene::FixPlaceableParentIds(const Vector<EntityWeakPtr> &entities, const EntityIdMap &oldToNewIds,
    AttributeChange::Type change, bool printStats) const
{
    Vector<Entity*> rawEntities;
    for (int i=0, len=entities.Size(); i<len; ++i)
    {
        if (!entities[i].Expired())
            rawEntities.Push(entities[i].Get());
    }
    return FixPlaceableParentIds(rawEntities, oldToNewIds, change, printStats);
}

uint Scene::FixPlaceableParentIds(const Vector<Entity *> &entities, const EntityIdMap &oldToNewIds,
    AttributeChange::Type change, bool printStats) const
{
    Urho3D::HiresTimer t;
    uint fixed = 0;

    foreach(Entity *entity, entities)
    {
        if (!entity)
            continue;

        ComponentPtr placeable = entity->Component(20); // Placeable
        Attribute<EntityReference> *parentRef = (placeable.Get() ? static_cast<Attribute<EntityReference> *>(placeable->AttributeById("parentRef")) : 0);
        if (parentRef && !parentRef->Get().IsEmpty())
        {
            // We only need to fix the id parent refs. Ones with Entity names should
            // work as expected (if names are unique which would be a authoring problem
            // and not addressed by Tundra).
            entity_id_t refId = Urho3D::ToUInt(parentRef->Get().ref); 
            if (refId > 0 && oldToNewIds.Contains(refId))
            {
                parentRef->Set(EntityReference(const_cast<EntityIdMap&>(oldToNewIds)[refId]), change);
                fixed++;
            }
        }
    }

    if (printStats && fixed > 0)
        LogInfo("Scene::FixPlaceableParentIds: Fixed " + String(fixed) + " parentRefs in " + String((int)(t.GetUSec(false)/1000)) + " msecs. Input Entities " + String(entities.Size()));
    else
        LogDebug("Scene::FixPlaceableParentIds: Fixed " + String(fixed) + " parentRefs in " + String((int)(t.GetUSec(false)/1000)) + " msecs. Input Entities " + String(entities.Size()));
    return fixed;
}

}
