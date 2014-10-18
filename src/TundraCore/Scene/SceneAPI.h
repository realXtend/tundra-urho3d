// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"
#include "SceneFwd.h"
#include "CoreTypes.h"
#include "AttributeChangeType.h"
#include "SceneDesc.h"
#include "Signals.h"

#include <Object.h>

namespace Tundra
{

/// Gives access to the scenes in the system.
/** With this API you can create, remove, query scenes and listen to scene additions and removals.
    Owned by Framework. */
class TUNDRACORE_API SceneAPI : public Object
{
    OBJECT(SceneAPI);

public:
    typedef HashMap<unsigned, ComponentDesc> PlaceholderComponentTypeMap;
    typedef HashMap<String, unsigned> PlaceholderComponentTypeIdMap;

    ~SceneAPI();

    /// Creates new component of the type @c T.
    /** @param newComponentName Name for the component (optional).
        @note Important: if creating a non-parented component (parentScene == 0), do not get() the raw pointer immediately
        upon this function call as the reference count of the SharedPtr goes to zero and the object is deleted. */
    template<typename T>
    SharedPtr<T> CreateComponent(Scene* parentScene, const String &newComponentName = "") const
    {
        return Urho3D::StaticCast<T>(CreateComponentById(parentScene, T::TypeIdStatic(), newComponentName));
    }

    /// Returns a list of all attribute type names that can be used in the CreateAttribute function to create an attribute.
    /** @note Unlike the available component types, the available attribute type set is static and cannot change at runtime.
        The available attribute types types are:
        "string", "int", "real", "Color", "float2", "float3", "float4", "bool", "uint", "Quat",
        "AssetReference", "AssetReferenceList", "EntityReference", "Variant", "VariantList",
        "Transform", and "Point" (can be considered somewhat deprecated). */
    static const StringVector &AttributeTypes();

    /// Returns the scene map for self reflection / introspection.
    SceneMap &Scenes();
    const SceneMap &Scenes() const;

    /// Register a placeholder component type by using an XML dump of the component's data
    void RegisterPlaceholderComponentType(Urho3D::XMLElement& element, AttributeChange::Type change = AttributeChange::Default);

    /// Register a placeholder component type by using a ComponentDesc
    void RegisterPlaceholderComponentType(ComponentDesc desc, AttributeChange::Type change = AttributeChange::Default);

    /// Returns the registered placeholder component descs.
    const PlaceholderComponentTypeMap& GetPlaceholderComponentTypes() const { return placeholderComponentTypes; }

    /// Returns a pointer to a scene
    /** Manage the pointer carefully, as scenes may not get deleted properly if
        references to the pointer are left alive.

        @note Returns a shared pointer, but it is preferable to use a weak pointer, SceneWeakPtr,
              to avoid dangling references that prevent scenes from being properly destroyed.

        @param name Name of the scene to return
        @return The scene, or empty pointer if the scene with the specified name could not be found. */
    ScenePtr SceneByName(const String &name) const;
    /// @todo ScenePtr SceneById(scene_id_t id) const;

    /// Returns the Scene the current active main camera is in.
    /** If there is no active main camera, this function returns the first found scene.
        If no scenes have been created, returns 0.
        @todo Refactor into ScenePtr MainCameraScene() const; */
    Scene *MainCameraScene();

    /// Creates new empty scene.
    /** @param name name of the new scene.
        @param viewEnabled Whether the scene is view enabled.
        @param authority True for server & standalone scenes, false for network client scene.
        @param change Notification/network replication mode.

        @note As Tundra doesn't currently support multiple replicated scenes, @c change has no real effect,
        unless AttributeChange::Disconnected is passed, which can be used f.ex. when creating dummy scenes silently
        (no GraphicsWorld or PhysicsWorld will be created) for serialization purposes.

        @return The new scene, or empty pointer if scene with the specified name already exists. */
    ScenePtr CreateScene(const String &name, bool viewEnabled, bool authority, AttributeChange::Type change = AttributeChange::Default);

    /// Removes a scene with the specified name.
    /** The scene may not get deleted since there may be dangling references to it.
        If the scene does get deleted, removes all entities which are not shared with
        another existing scene.

        Does nothing if scene with the specified name doesn't exist.

        @param name name of the scene to delete
        @param change Notification/network replication mode.

        @note As Tundra doesn't currently support multiple replicated scenes, @c change has no real effect,
        unless AttributeChange::Disconnected is passed, meaning that the SceneAboutToBeRemoved signal is not emitted.

        @return True if the scene was found and removed, false otherwise. */
    bool RemoveScene(const String &name, AttributeChange::Type change = AttributeChange::Default);

    /// Return if a component factory has been registered for a type name.
    bool IsComponentFactoryRegistered(const String &typeName) const;

    /// Return if a placeholder component type has been registered for a type name.
    bool IsPlaceholderComponentRegistered(const String &typeName) const;

    /// Return if a component type has been registered either as a C++ component factory or a placeholder type.
    bool IsComponentTypeRegistered(const String &typeName) const;

    /// Registers a new factory to create new components of type name IComponentFactory::TypeName and ID IComponentFactory::TypeId.
    void RegisterComponentFactory(const ComponentFactoryPtr &factory);

    /// Creates a new component instance by specifying the typename of the new component to create, and the scene where to create.
    ComponentPtr CreateComponentByName(Scene* scene, const String &componentTypeName, const String &newComponentName = "") const;

    /// Creates a new component instance by specifying the typeid of the new component to create, and the scene where to create.
    ComponentPtr CreateComponentById(Scene* scene, u32 componentTypeid, const String &newComponentName = "") const;

    /// Looks up the given type id and returns the type name string for that id.
    String ComponentTypeNameForTypeId(u32 componentTypeId) const;

    /// Looks up the given type name and returns the type id for that component type.
    u32 ComponentTypeIdForTypeName(const String &componentTypeName) const;

    /// Looks up the attribute type name for an attribute type id
    static String AttributeTypeNameForTypeId(u32 attributeTypeId);

    /// Looks up the type id for an attribute type name, or zero if not found
    /** @param attributeTypeName Attribute type name, handled as case-insensitive. */
    static u32 AttributeTypeIdForTypeName(const String &attributeTypeName);

    /// Creates a clone of the specified component. The new component will be detached, i.e. it has no parent entity.
    ///\todo Implement this.
//    ComponentPtr CloneComponent(const ComponentPtr &component, const String &newComponentName);

    /// Creates a new dynamic attribute without attaching it to a component and returns it. Returns null if illegal type name/ID.
    /** @note Never use this from script (the allocated IAttribute* cannot be freed)! In script use DynamicComponent::CreateAttribute instead.
        @param attributeTypeName Attribute type name, handled as case-insensitive.
        @param newAttributeId Arbitrary user-defined ID which identifies the attribute. 
        @note The name of the attribute will be assigned same as the ID. As dynamic attributes are serialized over the network per-object,
        it would unnecessarily increase the bandwidth needed to transfer both a name and an ID */
    static IAttribute* CreateAttribute(const String &attributeTypeName, const String& newAttributeId);
    static IAttribute* CreateAttribute(u32 attributeTypeId, const String& newAttributeId); /**< @overload @param attributeTypeId Attribute type name ID. */

    /// Returns a list of all component type names that can be used in the CreateComponentByName function to create a component.
    StringVector ComponentTypes() const;

    /// Register a custom static-attribute component type by using an existing component (DynamicComponent) as a blueprint.
    /** This is the same mechanism as the RegisterPlaceholderComponent above, but meant to be used from scripts.
        @param typeName The typename that is to be registered
        @param component The DynamicComponent that is used as a blueprint */
    void RegisterComponentType(const String& typeName, IComponent* component);

    /// Emitted after new scene has been added to framework.
    /** @param scene Scene that was just created.
        @param change Change signaling mode.
        @note As Tundra doesn't currently support multiple replicated scenes, @c change has no real meaning. */
    Signal2<Scene*, AttributeChange::Type> SceneCreated;

    /// Emitted after scene has been removed from the framework.
    /** @param scene Scene that is about to be removed.
        @param change Change signaling mode.
        @note As Tundra doesn't currently support multiple replicated scenes, @c change has no real meaning. */
    Signal2<Scene*, AttributeChange::Type> SceneAboutToBeRemoved;

    /// Emitted after a new placeholder (custom) component type has been registered.
    /** @param typeId Id of new component type
        @param typeName Name of new component type */
    Signal3<u32, const String&, AttributeChange::Type> PlaceholderComponentTypeRegistered;

private:
    friend class Framework;

    /// Constructor. Framework takes ownership of this object.
    /** @param owner Owner Framework. */
    explicit SceneAPI(Framework *owner);

    /// Frees all known scene and the scene interact object.
    /** Called by Framework during application shutdown. */
    void Reset();

    /// Creates a placeholder component when the real component is not available
    ComponentPtr CreatePlaceholderComponentById(Scene* scene, u32 componentTypeid, const String &newComponentName = "") const;

    ComponentFactoryPtr GetFactory(const String &typeName) const;
    ComponentFactoryPtr GetFactory(u32 typeId) const;

    typedef HashMap<String, ComponentFactoryPtr> ComponentFactoryMap;
    typedef HashMap<unsigned, WeakPtr<IComponentFactory> > ComponentFactoryWeakMap;

    ComponentFactoryMap componentFactories;
    ComponentFactoryWeakMap componentFactoriesByTypeid;
    PlaceholderComponentTypeMap placeholderComponentTypes;
    PlaceholderComponentTypeIdMap placeholderComponentTypeIds;

    Framework *framework;
    SceneMap scenes; ///< All currently created scenes.
    static StringVector attributeTypeNames;
};

}
