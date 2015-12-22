// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "StableHeaders.h"
#include "EntityPrioritizer.h"
#include "SyncState.h"

#include "Scene.h"
#include "Placeable.h"
#include "RigidBody.h"
#include "Mesh.h"
#include "IMeshAsset.h"
#include "Framework.h"
#include "LoggingFunctions.h"
//#include "Sound.h"

#include <Urho3D/Graphics/Model.h>

namespace Tundra
{

void DefaultEntityPrioritizer::ComputeSyncPriorities(EntitySyncStateMap &entities, const float3 &observerPos,const float3 &observerRot)
{
    // IDEA: could cache observerPos and observerRot and recompute priorities only of those are changed.
    // But of constantly moving objects it's probably good to recompute priorities every once in a while even if 
    // the observer doesn't move.
    if (!observerPos.IsFinite() || !observerRot.IsFinite())
        return; // camera information not received yet.
    ScenePtr scn = scene.Lock();
    if (!scn)
        return;

    for(EntitySyncStateMap::iterator it = entities.begin(); it != entities.end(); ++it)
    {
        EntitySyncState &entityState = it->second;
        Entity *entity = scn->EntityById(entityState.id).Get(); /**< @todo Use EntityWeakPtr in EntitySyncState when available */
        if (!entity)
            continue; // we (might) end up here e.g. when entity was just deleted

        /// @todo Check do we end up computing sync prio for local entities

        SharedPtr<Placeable> placeable = entity->Component<Placeable>();
        SharedPtr<Mesh> mesh = entity->Component<Mesh>();
        SharedPtr<RigidBody> rigidBody = entity->Component<RigidBody>();

        /// @todo sound sources
        /*
        SharedPtr<Sound> sound = entity->Component<Sound>();
        if (sound)
        {
            if (sound->spatial.Get() && placeable)
            {
                float r = sound->soundOuterRadius.Get();
                r *= r;
                entityState.priority = 4.f * pi * r / observerPos.DistanceSq(placeable->WorldPosition());
            }
            else
                entityState.priority = inf;
        }
        */
        /// @todo Handle terrains
        //shared_ptr<Terrain> terrain = entity->Component<Terrain>();
        //if (terrain) { ... }

        if (!placeable)
        {
            /// @todo Should handle special case entities with rigid body but no placeable?
            //if (rigidBody)
            // Non-spatial (probably), use max priority
            /// @todo Can have f.ex. Terrain component that has its own transform, but it can use Placeable too.
            entityState.priority = inf;
        }
        else if (placeable && !mesh)
        {
            // Spatial, but no mesh, for now use a harcoded priority of 20 (updateInterval = 1 / (priority * relevance),
            // so will probably yield the default SyncManager's update period 1/20th of a second
            entityState.priority = 20.f;
            /// @todo retrieve/calculate bounding volumes of possible billboards, particle systems, lights, etc.
            /// Not going to be easy with Ogre though, especially when running in headless mode.
        }
        else if (placeable && mesh)
        {
            OBB worldObb;
            if (scn->GetFramework()->IsHeadless())
            {
                // On headless mode, force mesh asset load in order to be able to inspect its AABB.
                if (!mesh->MeshAsset() && !mesh->meshRef.Get().ref.Trimmed().Empty())
                {
                    mesh->ForceMeshLoad();
                    continue; // compute the priority next time when mesh asset is available
                }
                // Mesh::WorldOBB not usable in headless mode
                // so we must dig the bounding volume information from the model asset instead.
                /// @todo For some meshes (f.ex. floor of the Avatar scene) there seems to be significant discrepancy
                // between the OBB values when running as headless or not. Investigate.
                Urho3D::Model* model = mesh->MeshAsset() ? mesh->MeshAsset()->UrhoModel() : (Urho3D::Model*)0;
                if (!model)
                    LogWarning("SyncManager::ComputeSyncPriorities: " + entity->ToString() + " has null Ogre mesh " + mesh->MeshName());
                worldObb = model ? AABB(model->GetBoundingBox()) : OBB();
                worldObb.Transform(placeable->LocalToWorld());
            }
            else
                worldObb = mesh->WorldOBB();
            float sizeSq = worldObb.SurfaceArea();
            sizeSq *= sizeSq;
            float distanceSq = observerPos.DistanceSq(placeable->WorldPosition());
            entityState.priority = sizeSq/distanceSq;
            //LogDebug(QString("%1 sizeSq %2 distanceSq %3").arg(entity->ToString()).arg(sizeSq).arg(distanceSq));
        }

        /// @todo Take direction and velocity of rigid bodies into account
            //if (rigidBody)
        /// @todo Hardcoded relevancy of 10 for entities with RigidBody component and 1 for others for now.
        /// @todo Movement of non-physical entities is too jerky.
        entityState.relevancy = rigidBody /*entity->Component("Avatar")*/ ? 10.f : 1.f;
        //LogDebug(QString("%1 P %2 R %3 P*R %4 syncRate %5").arg(entity->ToString()).arg(
            //entityState.priority).arg(entityState.relevancy).arg(entityState.FinalPriority()).arg(entityState.ComputePrioritizedUpdateInterval(updatePeriod_)));
    }
}

void DefaultEntityPrioritizer::ComputeSyncPriorities(EntitySyncState &entityState, const float3 &observerPos, const float3 &observerRot)
{
    EntitySyncStateMap m;
    m[entityState.id] = entityState;
    ComputeSyncPriorities(m, observerPos, observerRot);
}

}
