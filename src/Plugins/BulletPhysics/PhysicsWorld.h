// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "CoreTypes.h"
#include "SceneFwd.h"
#include "BulletPhysicsApi.h"
#include "BulletPhysicsFwd.h"
#include "Math/float3.h"
#include "Signals.h"

#include <Urho3D/Core/Object.h>

namespace Tundra
{

class GraphicsWorld;

/// Result of a raycast to the physical representation of a scene.
/** Other fields are valid only if entity is non-null
    @sa PhysicsWorld
  */
struct PhysicsRaycastResult
{
    Entity* entity; ///< Entity that was hit, null if none
    float3 pos; ///< World coordinates of hit position
    float3 normal; ///< World face normal of hit.
    float distance; ///< Distance from ray origin to the hit point.
};

/// A physics world that encapsulates a Bullet physics world
class BULLETPHYSICS_API PhysicsWorld : public Object
{
    URHO3D_OBJECT(PhysicsWorld, Object);

    friend class BulletPhysics;
    friend class RigidBody;

public:
    /// Constructor.
    /** @param scene Scene of which this PhysicsWorld is physical representation of.
        @param isClient Whether this physics world is for a client scene i.e. only simulates local entities' motion on their own.*/
    PhysicsWorld(Scene* scene, bool isClient);
    virtual ~PhysicsWorld();
    
    /// Step the physics world. May trigger several internal simulation substeps, according to the deltatime given. [noscript]
    void Simulate(float frametime);
    
    /// Process collision from an internal sub-step (Bullet post-tick callback) [noscript]
    void ProcessPostTick(float subStepTime);
    
    /// Returns the set of collisions that occurred during the previous frame.
    /// \important Use this function only for debugging, the availability of this set data structure is not guaranteed in the future.
    const HashSet<Pair<const btCollisionObject*, const btCollisionObject*> > &PreviousFrameCollisions() const { return previousCollisions_; }

    /// Set physics update period (= length of each simulation step.) By default 1/60th of a second.
    /** @param updatePeriod Update period */
    void SetPhysicsUpdatePeriod(float updatePeriod);

    /// Return internal physics timestep [property]
    float PhysicsUpdatePeriod() const { return physicsUpdatePeriod_; }

    /// Set maximum physics substeps to perform on a single frame. Once this maximum is reached, time will appear to slow down if framerate is too low.
    /** @param steps Maximum physics substeps */
    void SetMaxSubSteps(int steps);

    /// Return amount of maximum physics substeps on a single frame. [property]
    int MaxSubSteps() const { return maxSubSteps_; }

    /// Set gravity that affects all moving objects of the physics world
    /** @param gravity Gravity vector */
    void SetGravity(const float3& gravity);

    /// Return gravity [property]
    float3 Gravity() const;

    /// Enable/disable debug geometry
    void SetDebugGeometryEnabled(bool enable);
    
    /// Get debug geometry enabled status [property]
    bool IsDebugGeometryEnabled() const;
    
    /// Enable/disable physics simulation
    void SetRunning(bool enable) { runPhysics_ = enable; }
    
    /// Return whether simulation is on [property]
    bool IsRunning() const { return runPhysics_; }

    /// Return the Bullet world object
    btDiscreteDynamicsWorld* BulletWorld() const;

    /// Return whether the physics world is for a client scene. Client scenes only simulate local entities' motion on their own. [property]
    bool IsClient() const { return isClient_; }

    /// Raycast to the world. Returns only a single (the closest) result.
    /** @param origin World origin position
        @param direction Direction to raycast to. Will be normalized automatically
        @param maxDistance Length of ray
        @param collisionGroup Collision layer. Default has all bits set.
        @param collisionMask Collision mask. Default has all bits set.
        @return result PhysicsRaycastResult structure */
    PhysicsRaycastResult Raycast(const float3& origin, const float3& direction, float maxDistance, int collisionGroup = -1, int collisionMask = -1);

    /// Performs collision query for OBB.
    /** @param obb Oriented bounding box to test
        @param collisionGroup Collision layer of the OBB. Default has all bits set.
        @param collisionMask Collision mask of the OBB. Default has all bits set.
        @return List of entities with RigidBody component intersecting the OBB */
    EntityVector ObbCollisionQuery(const OBB &obb, int collisionGroup = -1, int collisionMask = -1);

    /// A physics collision has happened between two entities. 
    /** Note: both rigidbodies participating in the collision will also emit a signal separately. 
        Also, if there are several contact points, the signal will be sent multiple times for each contact.
        @param entityA The first entity
        @param entityB The second entity
        @param position World position of collision
        @param normal World normal of collision
        @param distance Contact distance
        @param impulse Impulse applied to the objects to separate them
        @param newCollision True if same collision did not happen on the previous frame.
                If collision has multiple contact points, newCollision can only be true for the first of them. */
    Signal7<Entity* ARG(entityA), Entity* ARG(entityB), const float3& ARG(position), const float3& ARG(normal), float ARG(distance), float ARG(impulse), bool ARG(newCollision)> PhysicsCollision;
    
    /// A new physics collision has happened between two entities.
    /** This is exactly like the PhysicsCollision but is only triggered once.
        If you are not interested in long running collisions like objects being inside each other or one is resting on the other, use this signal.
        @see PhysicsCollision */
    Signal6<Entity* ARG(entityA), Entity* ARG(entityB), const float3& ARG(position), const float3& ARG(normal), float ARG(distance), float ARG(impulse)> NewPhysicsCollision;

    /// Emitted before the simulation steps. Note: emitted only once per frame, not before each substep.
    /** @param frametime Length of simulation steps */
    Signal1<float ARG(frametime)> AboutToUpdate;
    
    /// Emitted after each simulation step
    /** @param frametime Length of simulation step */
    Signal1<float ARG(frametime)> Updated;

private:
    /// Draw physics debug geometry, if debug drawing enabled
    void DrawDebugGeometry();

    struct Impl;
    Impl *impl;
    /// Length of one physics simulation step
    float physicsUpdatePeriod_;
    /// Maximum amount of physics simulation substeps to run on a frame
    int maxSubSteps_;
    /// Client scene flag
    bool isClient_;
    /// Parent scene
    SceneWeakPtr scene_;
    /// Previous frame's collisions. We store these to know whether the collision was new or "ongoing"
    HashSet<Pair<const btCollisionObject*, const btCollisionObject*> > previousCollisions_;
    /// Debug geometry manually enabled/disabled (with physicsdebug console command). If true, do not automatically enable/disable debug geometry anymore
    bool drawDebugManuallySet_;
    /// Whether should run physics. Default true
    bool runPhysics_;
    /// Variable timestep flag
    bool useVariableTimestep_;
    
    /// Debug draw-enabled rigidbodies. Note: these pointers are never dereferenced, it is just used for counting
    HashSet<RigidBody*> debugRigidBodies_;

    float debugDrawUpdatePeriod_;
    float debugDrawT_;
};

}
