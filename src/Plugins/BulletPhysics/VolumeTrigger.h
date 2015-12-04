// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IComponent.h"
#include "CoreDefines.h"
#include "Math/float3.h"
#include "BulletPhysicsFwd.h"
#include "Signals.h"
#include "AttributeChangeType.h"

namespace Tundra
{

/// Physics volume trigger component
/** <table class="header">
    <tr>
    <td>
    <h2>VolumeTrigger</h2>
    Physics volume trigger component

    Registered by BulletPhysics.

    <b>Attributes</b>:
    <ul>
    <li>bool: byPivot
    <div>@copydoc byPivot</div>
    <li>QVariantList: entities
    <div>@copydoc entities</div>
    </ul>

    <b>Reacts on the following actions:</b>
    <ul>
    <li> None.
    </ul>
    </td>
    </tr>

    Does not emit any actions.

    <b>Depends on the component RigitBody.</b>.

    @note If you use 'byPivot' -option or use IsPivotInside-function, the pivot point shouldn't be outside the mesh 
        (or physics collision primitive) because physics collisions are used for efficiency even in this case.
    @todo If you add an entity to the 'interesting entities list', no signals may get send for that entity,
          and it may not show up in any list of entities contained in this volume trigger until that entity moves.
          Also if you enable/disable 'byPivot' option when entities are inside the volume, no signals may get send for those entities,
          and they may not show up in any list of entities contained in this volume trigger until the entities move.

    </table> */
class VolumeTrigger : public IComponent
{
    friend class PhysicsWorld;
    
    COMPONENT_NAME(VolumeTrigger, 24)

public:
    /// @cond PRIVATE
    /// Do not directly allocate new components using operator new, but use the factory-based SceneAPI::CreateComponent functions instead.
    explicit VolumeTrigger(Urho3D::Context* context, Scene* scene);
    /// @endcond
    virtual ~VolumeTrigger();

    /// Pivot trigger flag.
    /** If false (default), triggers by entity volume. If true, triggers by entity pivot point (ie. entity pivot points enters/leaves the volume). */
    Attribute<bool> byPivot;

    /// List of interesting entities by name.
    /** Events are dispatched only for entities in this list, other entities are ignored. Leave empty to get events for all entities in the scene.
        @todo 19.02.2014 Because this is VariantList, we could also support having entity IDs here. */
    Attribute<VariantList> entities;

    /// Returns a list of entities currently residing inside the volume.
    /** @note Return value is invalidated by physics update so the list might contain null pointers. */
    EntityVector EntitiesInside() const;

    /// Returns number of entities inside the volume.
    size_t NumEntitiesInside() const;

    /// Returns an entity that is inside this volume trigger with specified index.
    /** Use with EntitiesInside() (.size() or .length in script) to get all entities inside this volume.
        @note Use together with EntitiesInside() during the same physics update frame, because physics
        update may change the number of entities inside the volume. */
    Entity* EntityInside(size_t idx) const;

    /// Returns an approximate percent of how much of the entity is inside this volume, [0,1]
    /** If entity is not inside this volume at all, returns 0, if entity is completely inside this volume, returns 1.
        @note Uses axis aligned bounding boxes for calculations, so it is not accurate.
        @note Return value is invalidated by physics update.

        @param entity entity
        @return approximated percent of how much of the entity is inside this volume */
    float EntityInsidePercent(const Entity* entity) const;

    /// Returns an approximate percent of how much of the entity is inside this volume, [0,1]
    /** If entity is not inside this volume at all, returns 0, if entity is completely inside this volume, returns 1.
        @note Uses axis aligned bounding boxes for calculations, so it is not accurate.
        @note Return value is invalidated by physics update.
        @param name entity name
        @return approximated percent of how much of the entity is inside this volume */
    float EntityInsidePercentByName(const String &name) const;

    /// Returns true if specified entity can be found in the 'interesting entities' list
    /** If list of entities for this volume trigger is empty, returns always true for any entity name
        (even non-existing ones)
        @param name entity name */
    bool IsInterestingEntity(const String &name) const;

    /// Returns true if the pivot point of the specified entity is inside this volume trigger
    /** @note Return value is invalidated by physics update.
        @return true if the pivot point of the specified entity is inside the volume, false otherwise */
    bool IsPivotInside(Entity *entity) const;

    /// Returns true if given world coordinate point is inside volume. 
    bool IsInsideVolume(const float3& point) const;

    /// Entity has entered the volume.
    Signal1<Entity*> EntityEnter;

    /// Entity has left the volume.
    Signal1<Entity*> EntityLeave;

private:
    void UpdateSignals();

    /// Check for rigid body component and connect to its signals.
    void CheckForRigidBody();

    /// Component has been added to the entity. Check for rigid body now.
    void OnComponentAdded(IComponent* /*component*/, AttributeChange::Type /*change*/);

    /// Collisions have been processed for the scene the parent entity is in
    void OnPhysicsUpdate(float /*timeStep*/);

    /// Called when physics collisions occurs.
    void OnPhysicsCollision(Entity* otherEntity, const float3& position, const float3& normal, float distance, float impulse, bool newCollision);

    /// Called when entity inside this volume is removed from the scene
    void OnEntityRemoved(Entity* entity, AttributeChange::Type /*change*/);

    /// Called when some of the attributes has been changed.
    void AttributesChanged();

    /// Rigid body component that is needed for collision signals
    WeakPtr<RigidBody> rigidbody_;

    typedef HashMap<EntityWeakPtr, bool> EntitiesWithinVolumeMap;
    /// Map of entities inside this volume. 
    /** The value is used in physics update to see if the entity is still inside
        this volume or if it left the volume during last physics update. */
    EntitiesWithinVolumeMap entities_;
};
COMPONENT_TYPEDEFS(VolumeTrigger);

}

