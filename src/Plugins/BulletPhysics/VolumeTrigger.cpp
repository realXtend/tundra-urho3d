// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

#include "VolumeTrigger.h"
#include "RigidBody.h"
#include "PhysicsWorld.h"
#include "PhysicsUtils.h"

#include "Placeable.h"
#include "Entity.h"
#include "Scene/Scene.h"
#include "LoggingFunctions.h"

#include <btBulletDynamicsCommon.h>

#include <Urho3D/Core/Profiler.h>

namespace Tundra
{

VolumeTrigger::VolumeTrigger(Urho3D::Context* context, Scene* scene) :
    IComponent(context, scene),
    INIT_ATTRIBUTE_VALUE(byPivot, "By Pivot", false),
    INIT_ATTRIBUTE(entities, "Entities")
{
    ParentEntitySet.Connect(this, &VolumeTrigger::UpdateSignals);
}

VolumeTrigger::~VolumeTrigger()
{
}

EntityVector VolumeTrigger::EntitiesInside() const
{
    EntityVector ret;
    for(EntitiesWithinVolumeMap::ConstIterator it = entities_.Begin(); it != entities_.End(); ++it)
    {
        ret.Push(it->first_.Lock());
    }
    return ret;
}

size_t VolumeTrigger::NumEntitiesInside() const
{
    return entities_.Size();
}

Entity* VolumeTrigger::EntityInside(size_t idx) const
{
    if (idx < entities_.Size())
    {
        size_t currentIndex = 0;
        for(EntitiesWithinVolumeMap::ConstIterator it = entities_.Begin(); it != entities_.End(); ++it)
        {
            if (currentIndex == idx)
                return it->first_;
            ++currentIndex;
        }
    }
    return 0;
}

float VolumeTrigger::EntityInsidePercent(const Entity *entity) const
{
    if (entity)
    {
        SharedPtr<RigidBody> otherRigidbody = entity->Component<RigidBody>();
        SharedPtr<RigidBody> rigidbody = rigidbody_.Lock();
        if (rigidbody && otherRigidbody)
        {
            const AABB thisBox = rigidbody->ShapeAABB();
            const AABB otherBox = otherRigidbody->ShapeAABB();
            return (thisBox.Intersection(otherBox).Volume() / otherBox.Volume());
        }
        else
            LogWarning("VolumeTrigger: no RigidBody for entity or volume.");
    }
    return 0.0f;
}

float VolumeTrigger::EntityInsidePercentByName(const String &name) const
{
    for(EntitiesWithinVolumeMap::ConstIterator it = entities_.Begin(); it != entities_.End(); ++it)
        if (!it->first_.Expired() && it->first_->Name().Compare(name) == 0)
            return EntityInsidePercent(it->first_);
    return 0.f;
}

bool VolumeTrigger::IsInterestingEntity(const String &name) const
{
    URHO3D_PROFILE(VolumeTrigger_IsInterestingEntity); ///\todo The performance of this function feels really fishy - on each physics collision, we iterate through a list performing string comparisons.

    const VariantList &interestingEntities = entities.Get();
    if (interestingEntities.Empty())
        return true;

    foreach(const Variant &intname, interestingEntities)
        if (intname.GetString().Compare(name) == 0)
            return true;
    return false;
}

bool VolumeTrigger::IsPivotInside(Entity *entity) const
{
    SharedPtr<Placeable> placeable = entity->Component<Placeable>();
    SharedPtr<RigidBody> rigidbody = rigidbody_.Lock();
    if (placeable && rigidbody)
    {
        const Transform& trans = placeable->transform.Get();
        const float3& pivot = trans.pos;

        return ( RayTestSingle(float3(pivot.x, pivot.y - 1e7f, pivot.z), pivot, rigidbody->BulletRigidBody()) &&
                 RayTestSingle(float3(pivot.x, pivot.y + 1e7f, pivot.z), pivot, rigidbody->BulletRigidBody()) );
    }
    LogWarning("VolumeTrigger::IsPivotInside(): entity has no Placeable or volume has no RigidBody.");
    return false;
}

bool VolumeTrigger::IsInsideVolume(const float3& point) const
{
    SharedPtr<RigidBody> rigidbody = rigidbody_.Lock();
    if (!rigidbody)
    {
        LogWarning("Volume has no RigidBody.");
        return false;
    }

    return RayTestSingle(float3(point.x, point.y - 1e7f, point.z), point, rigidbody->BulletRigidBody()) &&
           RayTestSingle(float3(point.x, point.y + 1e7f, point.z), point, rigidbody->BulletRigidBody());
}

void VolumeTrigger::AttributesChanged()
{
    /// \todo Attribute updates not handled yet, there are a bit too many problems of what signals to send after the update -cm

    //if (mass.ValueChanged())
    //    ReadBody();
}

void VolumeTrigger::UpdateSignals()
{
    Entity* parent = ParentEntity();
    if (!parent)
        return;
    
    CheckForRigidBody();
    parent->ComponentAdded.Connect(this, &VolumeTrigger::OnComponentAdded);

    Scene* scene = parent->ParentScene();
    PhysicsWorld* world = scene->Subsystem<PhysicsWorld>().Get();
    if (world)
        world->Updated.Connect(this, &VolumeTrigger::OnPhysicsUpdate);
}

void VolumeTrigger::OnComponentAdded(IComponent* /*component*/, AttributeChange::Type /*change*/)
{
    CheckForRigidBody();
}

void VolumeTrigger::CheckForRigidBody()
{
    Entity* parent = ParentEntity();
    if (!parent)
        return;
    
    if (!rigidbody_.Lock())
    {
        SharedPtr<RigidBody> rigidbody = parent->Component<RigidBody>();
        if (rigidbody)
        {
            rigidbody_ = rigidbody;
            rigidbody->PhysicsCollision.Connect(this, &VolumeTrigger::OnPhysicsCollision);
        }
    }
}

void VolumeTrigger::OnPhysicsUpdate(float /*timeStep*/)
{
    URHO3D_PROFILE(VolumeTrigger_OnPhysicsUpdate);
    for(EntitiesWithinVolumeMap::Iterator it = entities_.Begin(); it != entities_.End();)
    {
        bool remove = false;
        EntityPtr entity = it->first_.Lock();
        
        // Entity was destroyed without us knowing? Remove from map in that case
        if (!entity)
            remove = true;
        else
        {
            // If collision is old, remove the entity if its rigidbody is active
            // (inactive rigidbodies do not refresh the collision, so we would remove the entity mistakenly)
            if (!it->second_)
            {
                bool active = true;
                SharedPtr<RigidBody> rigidbody = entity->Component<RigidBody>();
                if (rigidbody && rigidbody->mass.Get() > 0.0f)
                    active = rigidbody->IsActive();
                if (active)
                    remove = true;
            }
            else
            {
                // Age the collision from new to old
                it->second_ = false;
            }
        }
        
        if (!remove)
            ++it;
        else
        {
            it = entities_.Erase(it);

            if (entity)
            {
                EntityLeave.Emit(entity.Get());
                entity->EntityRemoved.Disconnect(this, &VolumeTrigger::OnEntityRemoved);
            }
        }
    }
}

void VolumeTrigger::OnPhysicsCollision(Entity* otherEntity, const float3& /*position*/,
    const float3& /*normal*/, float /*distance*/, float /*impulse*/, bool newCollision)
{
    URHO3D_PROFILE(VolumeTrigger_OnPhysicsCollision);

    assert(otherEntity && "Physics collision with no entity.");

    if (!entities.Get().Empty() && !IsInterestingEntity(otherEntity->Name()))
        return;

    // If byPivot attribute is enabled, we require the object pivot to enter the volume trigger area.
    // Otherwise, we react on each physics collision message (i.e. we accept if the volumetrigger and other entity just touch).
    if (byPivot.Get() && !IsPivotInside(otherEntity))
        return;

    bool refreshed = false;
    EntityWeakPtr otherEntityWeak(otherEntity);

    if (newCollision)
    {
        // make sure the entity isn't already inside the volume
        if (entities_.Find(otherEntityWeak) == entities_.End())
        {
            entities_[otherEntityWeak] = true;
            EntityEnter.Emit(otherEntity);
            refreshed = true;
            otherEntity->EntityRemoved.Connect(this, &VolumeTrigger::OnEntityRemoved);
        }
    }

    if (!refreshed)
    {
        // Refresh the collision status to new
        entities_[otherEntityWeak] = true;
    }
}

/** Called when the given entity is deleted from the scene. In that case, remove the Entity immediately from our tracking data structure (and signal listeners). */
void VolumeTrigger::OnEntityRemoved(Entity *entity, AttributeChange::Type /*change*/)
{
    assert(entity);
    EntityWeakPtr entityWeak(entity);
    EntitiesWithinVolumeMap::Iterator i = entities_.Find(entityWeak);
    if (i != entities_.End())
    {
        entities_.Erase(i);
        EntityLeave.Emit(entity);
    }
}

}
