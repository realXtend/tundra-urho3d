// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"
#include "CoreDefines.h"
#include "SceneFwd.h"
#include "AttributeChangeType.h"
#include "EntityAction.h"
#include "UniqueIdGenerator.h"
#include "Math/float3.h"
#include "SceneDesc.h"
#include "Entity.h"

#include <Vector.h>

namespace Tundra
{

class UserConnection;

/// A collection of entities which form an observable world.
/** Acts as a factory for all entities.
    Has subsystem-specific worlds, such as rendering and physics.

    To create, access and remove scenes, see SceneAPI.

    \ingroup Scene_group */
class TUNDRACORE_API Scene : public Urho3D::Object
{
    OBJECT(Scene);

public:
    ~Scene();

    typedef HashMap<entity_id_t, EntityPtr> EntityMap; ///< Maps entities to their unique IDs.
    typedef EntityMap::Iterator Iterator; ///< entity iterator, see begin() and end()
    typedef EntityMap::ConstIterator ConstIterator; ///< const entity iterator. see begin() and end()
    typedef HashMap<entity_id_t, entity_id_t> EntityIdMap; ///< Used to map entity ID changes (oldId, newId).
    typedef HashMap<StringHash, SharedPtr<Urho3D::Object> > SubsystemMap; ///< Maps scene subsystems by type

    /// Returns name of the scene.
    const String &Name() const { return name_; }

    /// Returns iterator to the beginning of the entities.
    Iterator Begin() { return Iterator(entities_.Begin()); }

    /// Returns iterator to the end of the entities.
    Iterator End() { return Iterator(entities_.End()); }

    /// Returns constant iterator to the beginning of the entities.
    ConstIterator Begin() const { return ConstIterator(entities_.Begin()); }

    /// Returns constant iterator to the end of the entities.
    ConstIterator End() const { return ConstIterator(entities_.End()); }

    /// Returns entity map for introspection purposes
    const EntityMap &Entities() const { return entities_; }

    /// Returns true if the two scenes have the same name
    bool operator == (const Scene &other) const { return Name() == other.Name(); }

    /// Returns true if the two scenes have different names
    bool operator != (const Scene &other) const { return !(*this == other); }

    /// Order by scene name
    bool operator < (const Scene &other) const { return Name() < other.Name(); }
    
    /// Add a subsystem world (GraphicsWorld, PhysicsWorld)
    void AddSubsystem(Urho3D::Object* system);

    /// Remove a subsystem world
    void RemoveSubsystem(Urho3D::Object* system);

    /// Return a subsystem world 
    template <class T>
    SharedPtr<T> Subsystem() const;

    /// Forcibly changes id of an existing entity. If there already is an entity with the new id, it will be purged
    /** @note Called by scenesync. This will not trigger any signals
        @param old_id Old id of the existing entity
        @param new_id New id to set */
    void ChangeEntityId(entity_id_t old_id, entity_id_t new_id);

    /// Starts an attribute interpolation
    /** @param attr Attribute inside a static-structured component.
        @param endvalue Same kind of attribute holding the endpoint value. You must dynamically allocate this yourself, but Scene
               will always take care of deleting it.
        @param length Time length
        @return true if successful (attribute must be in interpolated mode (set in metadata), must be in component, component 
                must be static-structured, component must be in an entity which is in a scene, scene must be us) */
    bool StartAttributeInterpolation(IAttribute* attr, IAttribute* endvalue, float length);

    /// Ends an attribute interpolation. The last set value will remain.
    /** @param attr Attribute inside a static-structured component.
        @return true if an interpolation existed */
    bool EndAttributeInterpolation(IAttribute* attr);

    /// Ends all attribute interpolations
    void EndAllAttributeInterpolations();

    /// Processes all running attribute interpolations. LocalOnly change will be used.
    /** @param frametime Time step */
    void UpdateAttributeInterpolations(float frametime);

    /// See if scene is currently performing interpolations, to differentiate between interpolative & non-interpolative attribute changes.
    bool IsInterpolating() const { return interpolating_; }

    /// Returns Framework
    Framework *GetFramework() const { return framework_; }

    /// Inspects file and returns a scene description structure from the contents of XML file.
    /** @param filename File name. */
    SceneDesc CreateSceneDescFromXml(const String &filename) const;
    /// @overload
    /** @param data XML data to be processed.
        @param sceneDesc Initialized SceneDesc with filename prepared. */
    SceneDesc CreateSceneDescFromXml(const String &data, SceneDesc &sceneDesc) const;

    /// Inspects file and returns a scene description structure from the contents of binary file.
    /** @param filename File name. */
    SceneDesc CreateSceneDescFromBinary(const String &filename) const;
    /// @overload
    /** @param data Binary data to be processed. */
    SceneDesc CreateSceneDescFromBinary(PODVector<unsigned char> &data, SceneDesc &sceneDesc) const;

    /// Creates scene content from scene description.
    /** @param desc Scene description.
        @param useEntityIDsFromFile If true, the created entities will use the Entity IDs from the original file.
                  If the scene contains any previous entities with conflicting IDs, those are removed. If false, the entity IDs from the files are ignored,
                  and new IDs are generated for the created entities.
        @param change Change type that will be used, when removing the old scene, and deserializing the new
        @return List of created entities.
        @todo Return list of EntityPtrs instead of raw pointers. Could also consider EntityVector ,though Vector[] has the nice operator [] accessor. */
    Vector<Entity *> CreateContentFromSceneDesc(const SceneDesc &desc, bool useEntityIDsFromFile, AttributeChange::Type change);

    /// Emits notification of an attribute changing. Called by IComponent.
    /** @param comp Component pointer
        @param attribute Attribute pointer
        @param change Change signaling mode */
    void EmitAttributeChanged(IComponent* comp, IAttribute* attribute, AttributeChange::Type change);

    /// Emits notification of an attribute having been created. Called by IComponent's with dynamic structure
    /** @param comp Component pointer
        @param attribute Attribute pointer
        @param change Change signaling mode */
    void EmitAttributeAdded(IComponent* comp, IAttribute* attribute, AttributeChange::Type change);

    /// Emits notification of an attribute about to be deleted. Called by IComponent's with dynamic structure
    /** @param comp Component pointer
        @param attribute Attribute pointer
        @param change Change signaling mode */
     void EmitAttributeRemoved(IComponent* comp, IAttribute* attribute, AttributeChange::Type change);

    /// Emits a notification of a component being added to entity. Called by the entity
    /** @param entity Entity pointer
        @param comp Component pointer
        @param change Change signaling mode */
    void EmitComponentAdded(Entity* entity, IComponent* comp, AttributeChange::Type change);

    /// Emits a notification of a component being removed from entity. Called by the entity
    /** @param entity Entity pointer
        @param comp Component pointer
        @param change Change signaling mode
        @note This is emitted before just before the component is removed. */
    void EmitComponentRemoved(Entity* entity, IComponent* comp, AttributeChange::Type change);

    /// Emits a notification of an entity being removed.
    /** @note the entity pointer will be invalid shortly after!
        @param entity Entity pointer
        @param change Change signaling mode */
    void EmitEntityRemoved(Entity* entity, AttributeChange::Type change);

    /// Emits a notification of an entity action being triggered.
    /** @param entity Entity pointer
        @param action Name of the action
        @param params Parameters
        @param type Execution type. */
    void EmitActionTriggered(Entity *entity, const String &action, const StringVector &params, EntityAction::ExecTypeField type);

    /// Emits a notification of an entity creation acked by the server, and the entity ID changing as a result. Called by SyncManager
    void EmitEntityAcked(Entity* entity, entity_id_t oldId);

    /// Emits a notification of a component creation acked by the server, and the component ID changing as a result. Called by SyncManager
    void EmitComponentAcked(IComponent* component, component_id_t oldId);

    /// Returns all components of type T (and additionally with specific name) in the scene.
    template <typename T>
    Vector<SharedPtr<T> > Components(const String &name = "") const;

    /// Returns list of entities with a specific component present.
    /** @param name Name of the component, optional.
        @note O(n) */
    template <typename T>
    EntityVector EntitiesWithComponent(const String &name = "") const;

    /// @cond PRIVATE
    /// Do not directly allocate new scenes using operator new, but use the factory-based SceneAPI::CreateScene functions instead.
    /** @param name Name of the scene.
        @param fw Parent framework.
        @param viewEnabled Whether the scene is view enabled.
        @param authority Whether the scene has authority i.e. a single user or server scene, false for network client scenes */
    Scene(const String &name, Framework *fw, bool viewEnabled, bool authority);
    /// @endcond

    /// @cond PRIVATE
    // Not publicly documented as ideally Scene should not know about Placeable until its defined in TundraCore.
    /// Fix parent Entity ids that are set to Placeable::parentRef.
    /** If @c printStats is true, a summary of the execution is printed once done.
        @return Number of fixed parent refs. */
    uint FixPlaceableParentIds(const Vector<Entity *> &entities, const EntityIdMap &oldToNewIds, AttributeChange::Type change, bool printStats = false) const;
    uint FixPlaceableParentIds(const Vector<EntityWeakPtr> &entities, const EntityIdMap &oldToNewIds, AttributeChange::Type change, bool printStats = false) const; /**< @overload */
    /// @endcond

    /// Creates new entity that contains the specified components.
    /** Entities should never be created directly, but instead created with this function.

        To create an empty entity, omit the components parameter.

        @param id Id of the new entity. Specify 0 to use the next free (replicated) ID, see also NextFreeId and NextFreeIdLocal.
        @param components Optional list of component names ("EC_" prefix can be omitted) the entity will use. If omitted or the list is empty, creates an empty entity.
        @param change Notification/network replication mode
        @param replicated Whether entity is replicated. Default true.
        @param componentsReplicated Whether components will be replicated, true by default.
        @param temporary Will the entity be temporary i.e. it is no serialized to disk by default, false by default.
        @sa CreateLocalEntity, CreateLocalTemporaryEntity, CreateTemporaryEntity*/
    EntityPtr CreateEntity(entity_id_t id = 0, const StringVector &components = StringVector(),
        AttributeChange::Type change = AttributeChange::Default, bool replicated = true,
        bool componentsReplicated = true, bool temporary = false);

    /// Creates new local entity that contains the specified components
    /** Entities should never be created directly, but instead created with this function.

        To create an empty entity omit components parameter.

        @param components Optional list of component names ("EC_" prefix can be omitted) the entity will use. If omitted or the list is empty, creates an empty entity.
        @param change Notification/network replication mode
        @param componentsReplicated Whether components will be replicated, false by default, but components of local entities are not replicated so this has no effect.
        @param temporary Will the entity be temporary i.e. it is no serialized to disk by default.
        @sa CreateEntity, CreateLocalTemporaryEntity, CreateTemporaryEntity */
    EntityPtr CreateLocalEntity(const StringVector &components = StringVector(),
        AttributeChange::Type change = AttributeChange::Default, bool componentsReplicated = false, bool temporary = false);

    /// Convenience function for creating a temporary entity.
    /** @sa CreateEntity, CreateLocalEntity, CreateLocalTemporaryEntity */
    EntityPtr CreateTemporaryEntity(const StringVector &components = StringVector(),
        AttributeChange::Type change = AttributeChange::Default, bool componentsReplicated = true);

    /// Convenience function for creating a local temporary entity.
    /** @sa CreateEntity, CreateLocalEntity, CreateTemporaryEntity */
    EntityPtr CreateLocalTemporaryEntity(const StringVector &components = StringVector(),
        AttributeChange::Type change = AttributeChange::Default);

    /// Returns scene up vector. For now it is a compile-time constant
    /** @sa RightVector,.ForwardVector */
    float3 UpVector() const;

    /// Returns scene right vector. For now it is a compile-time constant
    /** @sa UpVector, ForwardVector */
    float3 RightVector() const;

    /// Returns scene forward vector. For now it is a compile-time constant
    /** @sa UpVector, RightVector */
    float3 ForwardVector() const;

    /// Is scene view enabled (i.e. rendering-related components actually create stuff).
    bool ViewEnabled() const { return viewEnabled_; }

    /// Is scene authoritative i.e. a server or standalone scene
    bool IsAuthority() const { return authority_; }

    /// Returns entity with the specified id
    /** @note Returns a shared pointer, but it is preferable to use a weak pointer, EntityWeakPtr,
        to avoid dangling references that prevent entities from being properly destroyed.
        @note O(log n)
        @sa EntityByName*/
    EntityPtr EntityById(entity_id_t id) const;

    /// Returns entity with the specified name.
    /** @note The name of the entity is stored in a Name component. If this component is not present in the entity, it has no name.
        @note Returns a shared pointer, but it is preferable to use a weak pointer, EntityWeakPtr,
              to avoid dangling references that prevent entities from being properly destroyed.
        @note @note O(n)
        @sa EntityById, FindEntitiesContaining */
    EntityPtr EntityByName(const String &name) const;

    /// Returns whether name is unique within the scene, i.e. is only encountered once, or not at all.
    /** @note O(n) */
    bool IsUniqueName(const String& name) const;

    /// Returns true if entity with the specified id exists in this scene, false otherwise
    /** @note O(log n) */
    bool HasEntity(entity_id_t id) const { return (entities_.Find(id) != entities_.End()); }

    /// Removes entity with specified id
    /** The entity may not get deleted if dangling references to a pointer to the entity exists.
        @param id Id of the entity to remove
        @param change Origin of change regards to network replication.
        @return Was the entity found and removed. */
    bool RemoveEntity(entity_id_t id, AttributeChange::Type change = AttributeChange::Default);

    /// Removes all entities
    /** The entities may not get deleted if dangling references to a pointer to them exist.
        @param signal Whether to send signals of each delete. */
    void RemoveAllEntities(bool signal = true, AttributeChange::Type change = AttributeChange::Default);

    /// Gets and allocates the next free entity id.
    /** @sa NextFreeIdLocal */
    entity_id_t NextFreeId();

    /// Gets and allocates the next free entity id.
    /** @sa NextFreeId */
    entity_id_t NextFreeIdLocal();

    /// Returns list of entities with a specific component present.
    /** @param typeId Type ID of the component
        @param name Name of the component, optional.
        @note O(n) */
    EntityVector EntitiesWithComponent(u32 typeId, const String &name = "") const;
    /// @overload
    /** @param typeName typeName Type name of the component.
        @note The overload taking type ID is more efficient than this overload. */
    EntityVector EntitiesWithComponent(const String &typeName, const String &name = "") const;

    /// Returns list of entities that belong to the group 'groupName'
    /** @param groupName The name of the group to be queried */
    EntityVector EntitiesOfGroup(const String &groupName) const;

    /// Returns all components of specific type (and additionally with specific name) in the scene.
    /*  @param typeId Component type ID.
        @param name Arbitrary name of the component (optional). */
    Entity::ComponentVector Components(u32 typeId, const String &name = "") const;
    /// overload
    /** @param typeName Component type name.
        @note The overload taking type ID is more efficient than this overload. */
    Entity::ComponentVector Components(const String &typeName, const String &name = "") const;

    /// Performs a search through the entities, and returns a list of all the entities that contain @c substring in their Entity name.
    /** @param substring String to be searched.
        @param caseSensitive Case sensitivity for the string matching. */
    EntityVector FindEntitiesContaining(const String &substring, bool caseSensitive = true) const;

    /// Performs a search through the entities, and returns a list of all the entities where Entity name matches @c name.
    /** @param name Entity name to match.
        @param caseSensitive Case sensitivity for the string matching. */
    EntityVector FindEntitiesByName(const String &name, bool caseSensitive = true) const;

    /// Return root-level entities, i.e. those that have no parent.
    EntityVector RootLevelEntities() const;

    /// Loads the scene from XML.
    /** @param filename File name
        @param clearScene Do we want to clear the existing scene.
        @param useEntityIDsFromFile If true, the created entities will use the Entity IDs from the original file. 
                  If the scene contains any previous entities with conflicting IDs, those are removed. If false, the entity IDs from the files are ignored,
                  and new IDs are generated for the created entities.
        @param change Change type that will be used, when removing the old scene, and deserializing the new
        @return List of created entities.
        @todo Return list of EntityPtrs instead of raw pointers. Could also consider EntityVector ,though Vector[] has the nice operator [] accessor. */
    Vector<Entity *> LoadSceneXML(const String& filename, bool clearScene, bool useEntityIDsFromFile, AttributeChange::Type change);

    /// Returns scene content as an XML string.
    /** @param serializeTemporary Are temporary entities wanted to be included.
        @param serializeLocal Are local entities wanted to be included.
        @return The scene XML as a string. */
    String SerializeToXmlString(bool serializeTemporary, bool serializeLocal) const;

    /// Saves the scene to XML.
    /** @param filename File name
        @param saveTemporary Are temporary entities wanted to be included.
        @param saveLocal Are local entities wanted to be included.
        @return true if successful */
    bool SaveSceneXML(const String& filename, bool saveTemporary, bool saveLocal) const;

    /// Loads the scene from a binary file.
    /** @param filename File name
        @param clearScene Do we want to clear the existing scene.
        @param useEntityIDsFromFile If true, the created entities will use the Entity IDs from the original file. 
                  If the scene contains any previous entities with conflicting IDs, those are removed. If false, the entity IDs from the files are ignored,
                  and new IDs are generated for the created entities.
        @param change Change type that will be used, when removing the old scene, and deserializing the new
        @return List of created entities.
        @todo Return list of EntityPtrs instead of raw pointers. Could also consider EntityVector ,though Vector[] has the nice operator [] accessor. */
    Vector<Entity *> LoadSceneBinary(const String& filename, bool clearScene, bool useEntityIDsFromFile, AttributeChange::Type change);

    /// Save the scene to binary
    /** @param filename File name
        @param saveTemporary Are temporary entities wanted to be included.
        @param saveLocal Are local entities wanted to be included.
        @return true if successful */
    bool SaveSceneBinary(const String& filename, bool saveTemporary, bool saveLocal) const;

    /// Creates scene content from XML.
    /** @param xml XML document as string.
        @param useEntityIDsFromFile If true, the created entities will use the Entity IDs from the original file.
                  If the scene contains any previous entities with conflicting IDs, those are removed. If false, the entity IDs from the files are ignored,
                  and new IDs are generated for the created entities.
        @param change Change type that will be used, when removing the old scene, and deserializing the new
        @return List of created entities.
        @todo Return list of EntityPtrs instead of raw pointers. Could also consider EntityVector ,though Vector[] has the nice operator [] accessor. */
    Vector<Entity *> CreateContentFromXml(const String &xml, bool useEntityIDsFromFile, AttributeChange::Type change);
    Vector<Entity *> CreateContentFromXml(Urho3D::XMLFile &xml, bool useEntityIDsFromFile, AttributeChange::Type change); /**< @overload @param xml XML document. */

    /// Creates scene content from binary file.
    /** @param filename File name.
        @param useEntityIDsFromFile If true, the created entities will use the Entity IDs from the original file.
                  If the scene contains any previous entities with conflicting IDs, those are removed. If false, the entity IDs from the files are ignored,
                  and new IDs are generated for the created entities.
        @param change Change type that will be used, when removing the old scene, and deserializing the new
        @return List of created entities.
        @todo Return list of EntityPtrs instead of raw pointers. Could also consider EntityVector ,though Vector[] has the nice operator [] accessor. */
    Vector<Entity *> CreateContentFromBinary(const String &filename, bool useEntityIDsFromFile, AttributeChange::Type change);
    Vector<Entity *> CreateContentFromBinary(const char *data, int numBytes, bool useEntityIDsFromFile, AttributeChange::Type change); /**< @overload @param data Data buffer @param numBytes Data size. */

    /// Returns @c ent parent Entity id.
    /** Check both Entity and EC_Placeble::parentRef parenting,
        Entity parenting takes precedence.
        @return Returns 0 if parent is not set or the parent ref is not a Entity id (but a entity name). */
    entity_id_t EntityParentId(const Entity *ent) const;

    /// Sorts @c entities by scene hierarchy and returns the sorted list.
    /** Takes into account both Entity::Parent and Placeable::parentRef parenting,
        Entity-level parenting takes precedence. */
    Vector<Entity*> SortEntities(const Vector<Entity*> &entities) const;
    Vector<EntityWeakPtr> SortEntities(const Vector<EntityWeakPtr> &entities) const;
    EntityDescList SortEntities(const EntityDescList &entities) const;

    /// Checks whether editing an entity is allowed.
    /** Emits AboutToModifyEntity.
        @user entity Connection that is requesting permission to modify an entity.
        @param entity Entity that is requested to be modified. */
    bool AllowModifyEntity(UserConnection *user, Entity *entity);

    /// Emits a notification of an entity having been created
    /** Creates are also automatically signaled at the end of frame, so you do not necessarily need to call this.
        @param entity Entity pointer
        @param change Change signaling mode */
    void EmitEntityCreated(Entity *entity, AttributeChange::Type change = AttributeChange::Default);

    /// Emits a notification of entity reparenting
    /** @param entity Entity that is being reparented
        @param newParent New parent entity
        @param change Change signaling mode */
    void EmitEntityParentChanged(Entity *entity, Entity *newParent, AttributeChange::Type change = AttributeChange::Default);

    /// Signal when an attribute of a component has changed
    /** Network synchronization managers should connect to this. */
    Signal3<IComponent*, IAttribute*, AttributeChange::Type> AttributeChanged;

    /// Signal when an attribute of a component has been added (dynamic structure components only)
    /** Network synchronization managers should connect to this. */
    Signal3<IComponent*, IAttribute*, AttributeChange::Type> AttributeAdded;

    /// Signal when an attribute of a component has been added (dynamic structure components only)
    /** Network synchronization managers should connect to this. */
    Signal3<IComponent*, IAttribute*, AttributeChange::Type> AttributeRemoved;

    /// Signal when a component is added to an entity and should possibly be replicated (if the change originates from local)
    /** Network synchronization managers should connect to this. */
    Signal3<Entity*, IComponent*, AttributeChange::Type> ComponentAdded;

    /// Signal when a component is removed from an entity and should possibly be replicated (if the change originates from local)
    /** Network synchronization managers should connect to this */
    Signal3<Entity*, IComponent*, AttributeChange::Type> ComponentRemoved;

    /// Signal when an entity created
    /** @note Entity::IsTemporary() information might not be accurate yet, as it depends on the method that was used to create the entity. */
    Signal2<Entity*, AttributeChange::Type> EntityCreated;

    /// Signal when an entity deleted
    Signal2<Entity*, AttributeChange::Type> EntityRemoved;

    /// A entity creation has been acked by the server and assigned a proper replicated ID
    Signal2<Entity*, entity_id_t> EntityAcked;

    /// An entity's temporary state has been toggled
    Signal2<Entity*, AttributeChange::Type> EntityTemporaryStateToggled;

    /// A component creation into an entity has been acked by the server and assigned a proper replicated ID
    Signal2<IComponent*, component_id_t> ComponentAcked;

    /// Emitted when entity action is triggered.
    /** @param entity Entity for which action was executed.
        @param action Name of action that was triggered.
        @param params Parameters of the action.
        @param type Execution type.

        @note Use case-insensitive comparison for checking name of the @c action ! */
    Signal4<Entity*, const String &, const StringVector &, EntityAction::ExecTypeField> ActionTriggered;

    /// Emitted when an entity is about to be modified:
    Signal3<ChangeRequest*, UserConnection*, Entity*> AboutToModifyEntity;

    /// Emitted when being destroyed
    Signal1<Scene*> Removed;

    /// Signal when the whole scene is cleared
    Signal1<Scene*> SceneCleared;

    /// An entity's parent has changed.
    Signal3<Entity*, Entity*, AttributeChange::Type> EntityParentChanged;

private:
    /// Handle frame update. Signal this frame's entity creations.
    void OnUpdated(float frameTime);

    friend class SceneAPI;

    /// Create entity from an XML element and recurse into child entities. Called internally.
    void CreateEntityFromXml(EntityPtr parent, const Urho3D::XMLElement& ent_elem, bool useEntityIDsFromFile,
        AttributeChange::Type change, Vector<EntityWeakPtr>& entities, EntityIdMap& oldToNewIds);
    /// Create entity from binary data and recurse into child entities. Called internally.
    void CreateEntityFromBinary(EntityPtr parent, kNet::DataDeserializer& source, bool useEntityIDsFromFile,
        AttributeChange::Type change, Vector<EntityWeakPtr>& entities, EntityIdMap& oldToNewIds);
    /// Create entity from entity desc and recurse into child entities. Called internally.
    void CreateEntityFromDesc(EntityPtr parent, const EntityDesc& source, bool useEntityIDsFromFile,
        AttributeChange::Type change, Vector<Entity *>& entities, EntityIdMap& oldToNewIds);
    /// Create entity desc from an XML element and recurse into child entities. Called internally.
    void CreateEntityDescFromXml(SceneDesc& sceneDesc, Vector<EntityDesc>& dest, const Urho3D::XMLElement& ent_elem) const;

    /// Container for an ongoing attribute interpolation
    struct AttributeInterpolation
    {
        AttributeInterpolation() : time(0.0f), length(0.0f) {}
        AttributeWeakPtr dest, start, end;
        float time;
        float length;
    };

    /// Resolved parent Entity id that is set to Placeable::parentRef.
    /** @return Returns 0 if parent is not set or the parent ref is not a Entity id (but a entity name). */
    entity_id_t PlaceableParentId(const Entity *ent) const;
    entity_id_t PlaceableParentId(const EntityDesc &ent) const; ///< @overload

    UniqueIdGenerator idGenerator_; ///< Entity ID generator
    EntityMap entities_; ///< All entities in the scene.
    Framework *framework_; ///< Parent framework.
    String name_; ///< Name of the scene.
    bool viewEnabled_; ///< View enabled -flag.
    bool interpolating_; ///< Currently doing interpolation-flag.
    bool authority_; ///< Authority -flag
    Vector<AttributeInterpolation> interpolations_; ///< Running attribute interpolations.
    Vector<std::pair<EntityWeakPtr, AttributeChange::Type> > entitiesCreatedThisFrame_; ///< Entities to signal for creation at frame end.
    ParentingTracker parentTracker_; ///< Tracker for client side mass Entity imports (eg. SceneDesc based).
    SubsystemMap subsystems; ///< Scene subsystems
};

}

#include "Scene.inl"
