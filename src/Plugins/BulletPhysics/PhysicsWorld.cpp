// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#define MATH_BULLET_INTEROP

#include "BulletPhysics.h"
#include "PhysicsWorld.h"
#include "PhysicsUtils.h"
#include "RigidBody.h"
#include "Framework.h"
#include "LoggingFunctions.h"

#include "Scene.h"
#include "GraphicsWorld.h"
#include "Camera.h"

#include "Geometry/LineSegment.h"
#include "Geometry/OBB.h"
#include "Geometry/Frustum.h"
#include "Math/float3x3.h"
#include "Math/Quat.h"

#include "Entity.h"

#include <LinearMath/btIDebugDraw.h>
// Disable unreferenced formal parameter coming from Bullet
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4100)
#endif
#include <btBulletDynamicsCommon.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <Urho3D/Core/Profiler.h>
#include <Urho3D/Core/Timer.h>

namespace Tundra
{

struct CollisionSignal
{
    WeakPtr<RigidBody> bodyA;
    WeakPtr<RigidBody> bodyB;
    float3 position;
    float3 normal;
    float distance;
    float impulse;
    bool newCollision;
};

struct ObbCallback : public btCollisionWorld::ContactResultCallback
{
    ObbCallback(HashSet<btCollisionObjectWrapper*>& result) : result_(result) {}

    virtual btScalar addSingleResult(btManifoldPoint &/*cp*/, const btCollisionObjectWrapper *colObj0, int, int, const btCollisionObjectWrapper *colObj1, int, int)
    {
        result_.Insert((btCollisionObjectWrapper*)(colObj0));
        result_.Insert((btCollisionObjectWrapper*)(colObj1));
        return 0.0f;
    }
    
    HashSet<btCollisionObjectWrapper*>& result_;
};

void TickCallback(btDynamicsWorld *world, btScalar timeStep)
{
    static_cast<PhysicsWorld*>(world->getWorldUserInfo())->ProcessPostTick(timeStep);
}

struct PhysicsWorld::Impl : public btIDebugDraw
{
    struct DebugDrawLineCacheItem
    {
        float3 from;
        float3 to;
        Color color;
    };

    explicit Impl(PhysicsWorld *owner) :
        debugDrawMode(0),
        collisionConfiguration(0),
        collisionDispatcher(0),
        broadphase(0),
        solver(0),
        world(0),
        cachedGraphicsWorld(0)
    {
        collisionConfiguration = new btDefaultCollisionConfiguration();
        collisionDispatcher = new btCollisionDispatcher(collisionConfiguration);
        broadphase = new btDbvtBroadphase();
        solver = new btSequentialImpulseConstraintSolver();
        world = new btDiscreteDynamicsWorld(collisionDispatcher, broadphase, solver, collisionConfiguration);
        world->setDebugDrawer(this);
        world->setInternalTickCallback(TickCallback, (void*)owner, false);
    }

    ~Impl()
    {
        delete world;
        delete solver;
        delete broadphase;
        delete collisionDispatcher;
        delete collisionConfiguration;
    }

    void PreDebugDraw()
    {
        debugDrawState.Starting();
    }

    void PostDebugDraw()
    {
        if (debugDrawState.exhausted)
            debugDrawMode = debugDrawState.preExhaustionDrawMode;
        debugDrawState.Completed();
    }

    void DrawCachedDebugLines()
    {
        if (!IsDebugGeometryEnabled() || !cachedGraphicsWorld)
            return;
        
        for(uint i = 0, len = debugDrawState.lineCache.Size(); i < debugDrawState.lineCacheIndex && i < len; ++i)
        {
            const DebugDrawLineCacheItem *cacheLine = debugDrawState.lineCache[i];
            cachedGraphicsWorld->DebugDrawLine(cacheLine->from, cacheLine->to, cacheLine->color);
        }
    }

    /// btIDebugDraw override
    virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& color)
    {
        if (debugDrawState.exhausted)
            return;
        if (!IsDebugGeometryEnabled() || !cachedGraphicsWorld)
            return;

        // Incrementally make cache bigger up to debugDrawCacheMaxSize_.
        bool cacheFull = debugDrawState.IsCacheFull();
        if (cacheFull && !debugDrawState.CacheAtFullCapacity())
        {
            if (debugDrawState.IncrementCache(50000))
                cacheFull = false;
        }
        if (cacheFull || debugDrawState.IsOverAllowedExecutionTime())
        {
            /* This mode is queried per object inside bullets debug draw, and is the only mechanism to halt the internals
               mid rendering. Once max rendering time has been reached, we want to tell bullet to stop calculating the lines.
               Even if we don't render them here, bullet will stil carry on via debugDrawMode if not changed here. */
            debugDrawState.exhausted = (cacheFull ? DebugDrawState::E_CacheFull : DebugDrawState::E_ExecutionTime);
            debugDrawState.preExhaustionDrawMode = debugDrawMode;

            debugDrawMode = btIDebugDraw::DBG_NoDebug;
            return;
        }

        /* @todo If would be nice to filter out things that are not in camera frustum
        if (debugDrawState.frustum.type != InvalidFrustum)
        {
            debugDrawState.testLine.a.Set(from.x(), from.y(), from.z());
            debugDrawState.testLine.b.Set(to.x(), to.y(), to.z());

            if (!debugDrawState.frustum.Contains(debugDrawState.testLine))
                return;
        }*/

        /* If last frame was exhausted by the cache side, try rendering less often
           does not work well yet, see the function.
        if (debugDrawState.ShouldSkipLine())
            return;*/

        DebugDrawLineCacheItem *cacheLine = debugDrawState.Next();
        cacheLine->from.Set(from.x(), from.y(), from.z());
        cacheLine->to.Set(to.x(), to.y(), to.z());
        cacheLine->color.Set(color.x(), color.y(), color.z());

        cachedGraphicsWorld->DebugDrawLine(cacheLine->from, cacheLine->to, cacheLine->color);
    }

    /// btIDebugDraw override
    virtual void reportErrorWarning(const char* warningString)
    {
        LogWarning("Physics: " + String(warningString));
    }

    /// btIDebugDraw override, does nothing.
    /// @todo Now that we have PhysicsConstraint, implement this!
    virtual void drawContactPoint(const btVector3& /*pointOnB*/, const btVector3& /*normalOnB*/, btScalar /*distance*/, int /*lifeTime*/, const btVector3& /*color*/) {}
    
    /// btIDebugDraw override, does nothing.
    virtual void draw3dText(const btVector3& /*location*/, const char* /*textString*/) {}
    
    /// btIDebugDraw override
    virtual void setDebugMode(int debugMode)
    {
        debugDrawMode = debugMode;

        // Create new cache. Start with 50k and ramp up from there (in drawLine) towards debugDrawCacheMaxSize_.
        debugDrawState.ReserveCache(IsDebugGeometryEnabled() ? 50000 : -1);
    }
    
    /// btIDebugDraw override
    virtual int getDebugMode() const { return debugDrawMode; }

    bool IsDebugGeometryEnabled() const { return getDebugMode() != btIDebugDraw::DBG_NoDebug; }

    /// Bullet collision config
    btCollisionConfiguration* collisionConfiguration;
    /// Bullet collision dispatcher
    btDispatcher* collisionDispatcher;
    /// Bullet collision broadphase
    btBroadphaseInterface* broadphase;
    /// Bullet constraint equation solver
    btConstraintSolver* solver;
    /// Bullet physics world
    btDiscreteDynamicsWorld* world;
    /// Bullet debug draw / debug behaviour flags
    int debugDrawMode;
    /// Cached GraphicsWorld pointer for drawing debug geometry
    GraphicsWorld* cachedGraphicsWorld;

    /// Choking for debug rendering
    struct DebugDrawState
    {
        enum Exhausted
        {
            E_NotExhausted = 0,
            E_ExecutionTime,
            E_CacheFull
        };

        long long timeStarted;
        long long timeCompleted;
        /// Timer for measuring elapsed time in debug geometry rendering
        mutable Urho3D::HiresTimer debugTimer;

        Exhausted exhausted;
        Exhausted exhaustedLastUpdate;

        int allowedExecutionMSecs;
        int preExhaustionDrawMode;

        PODVector<DebugDrawLineCacheItem*> lineCache;
        uint lineCacheIndex;
        uint lineCacheMaxSize;
        uint lineCacheSkipped;

        Frustum frustum;
        LineSegment testLine;

        DebugDrawState() :  
            exhausted(E_NotExhausted), exhaustedLastUpdate(E_NotExhausted), allowedExecutionMSecs(33),
            preExhaustionDrawMode(0), lineCacheIndex(0),
#if _DEBUG
            // >100k will start to hurt memory usage
            lineCacheMaxSize(100000)
#else
            lineCacheMaxSize(1000000)
#endif
        {
            timeStarted = debugTimer.GetUSec(false);
            timeCompleted = timeStarted;
        }

        ~DebugDrawState()
        {
            ReserveCache(-1);
        }

        void Starting()
        {
            timeStarted = debugTimer.GetUSec(false);

            exhaustedLastUpdate = exhausted;
            exhausted = E_NotExhausted;
            
            lineCacheIndex = 0;
            lineCacheSkipped = 0;
        }

        void Completed()
        {
            timeCompleted = debugTimer.GetUSec(false);

            /* As we will be doing rendering less often, 1fps when exhausted,
               we can take a bit more time to do it and provide more rendering results
               around or below the same time spent. */
            allowedExecutionMSecs = (exhausted ? 300 : 33);
        }

        bool IsExhausted() const
        {
            return (exhausted != E_NotExhausted);
        }

        bool IsOverAllowedExecutionTime() const
        {
            return (debugTimer.GetUSec(false) - timeStarted) >= allowedExecutionMSecs * 1000;
        }

        void ReserveCache(int num)
        {
            for(int i=0, len=lineCache.Size(); i<len; ++i)
                delete lineCache[i];
            lineCache.Clear();
            lineCacheIndex = 0;
            lineCacheSkipped = 0;

            for(int i=0; i<num; ++i)
                lineCache.Push(new DebugDrawLineCacheItem());
        }

        bool IncrementCache(int num)
        {
            bool newAllocated = false;
            for(int i=0; i<num; ++i)
            {
                if (!CacheAtFullCapacity())
                {
                    lineCache.Push(new DebugDrawLineCacheItem());
                    newAllocated = true;
                }
                else
                    break;
            }
            return newAllocated;
        }

        bool IsCacheFull() const
        {
            return (lineCacheIndex >= lineCache.Size());
        }

        bool CacheAtFullCapacity() const
        {
            return (lineCache.Size() >= lineCacheMaxSize);
        }
        
        bool ShouldSkipLine()
        {
            // Meh this blinks, not sure if this can be done without more book keeping
            // eg. render every other for 60 frames when E_CacheFull happens. Would still
            // introduce blinking in the rendering every other second...
            return false;

            if (lineCacheIndex == 0)
                return false;

            if (exhaustedLastUpdate == E_CacheFull && ((lineCacheIndex + lineCacheSkipped) % 2 == 0))
            {
                lineCacheSkipped++;
                return true;
            }
            return false;
        }

        DebugDrawLineCacheItem *Next()
        {
            DebugDrawLineCacheItem *item = lineCache[lineCacheIndex];
            lineCacheIndex++;
            return item;
        }
    };
    DebugDrawState debugDrawState;
};

PhysicsWorld::PhysicsWorld(Scene* scene, bool isClient) :
    Object(scene->GetContext()),
    scene_(scene),
    physicsUpdatePeriod_(1.0f / 60.0f),
    debugDrawUpdatePeriod_(1.0f),
    debugDrawT_(0.0f),
    maxSubSteps_(6), // If fps is below 10, we start to slow down physics
    isClient_(isClient),
    runPhysics_(true),
    drawDebugManuallySet_(false),
    useVariableTimestep_(false),
    impl(new Impl(this))
{
    if (scene->GetFramework()->HasCommandLineParameter("--variablephysicsstep"))
        useVariableTimestep_ = true;
}

PhysicsWorld::~PhysicsWorld()
{
    delete impl;
}

void PhysicsWorld::SetPhysicsUpdatePeriod(float updatePeriod)
{
    // Allow max.1000 fps
    if (updatePeriod <= 0.001f)
        updatePeriod = 0.001f;
    physicsUpdatePeriod_ = updatePeriod;
}

void PhysicsWorld::SetMaxSubSteps(int steps)
{
    if (steps > 0)
        maxSubSteps_ = steps;
}

void PhysicsWorld::SetGravity(const float3& gravity)
{
    impl->world->setGravity(gravity);
}

float3 PhysicsWorld::Gravity() const
{
    return impl->world->getGravity();
}

btDiscreteDynamicsWorld* PhysicsWorld::BulletWorld() const
{
    return impl->world;
}

void PhysicsWorld::Simulate(float frametime)
{
    if (!runPhysics_)
        return;
    
    URHO3D_PROFILE(PhysicsWorld_Simulate);

    const float fFrametime = static_cast<float>(frametime);
    
    AboutToUpdate.Emit(fFrametime);
    
    {
        URHO3D_PROFILE(Bullet_stepSimulation); ///\note Do not delete or rename this URHO3D_PROFILE() block. The DebugStats profiler uses this string as a label to know where to inject the Bullet internal profiling data.
        
        // Use variable timestep if enabled, and if frame timestep exceeds the single physics simulation substep
        if (useVariableTimestep_ && frametime > physicsUpdatePeriod_)
        {
            float clampedTimeStep = fFrametime;
            if (clampedTimeStep > 0.1f)
                clampedTimeStep = 0.1f; // Advance max. 1/10 sec. during one frame
            impl->world->stepSimulation(clampedTimeStep, 0, clampedTimeStep);
        }
        else
            impl->world->stepSimulation(fFrametime, maxSubSteps_, physicsUpdatePeriod_);
    }
    
    if (!scene_.Expired() && !scene_.Lock()->GetFramework()->IsHeadless())
    {
        // Don't choke debug rendering if it is not spending too much time and cache items per frame.
        // If debug rendering is having performance issues, drop to rendering it few times a second (debugDrawUpdatePeriod_).
        debugDrawT_ += fFrametime;
        if (!impl->debugDrawState.IsExhausted() || debugDrawT_ >= debugDrawUpdatePeriod_)
        {
            debugDrawT_ = 0.0f;

            // Automatically enable debug geometry if at least one debug-enabled rigidbody. Automatically disable if no debug-enabled rigidbodies
            // However, do not do this if user has used the physicsdebug console command
            if (!drawDebugManuallySet_)
            {
                if (!IsDebugGeometryEnabled() && !debugRigidBodies_.Empty())
                    SetDebugGeometryEnabled(true);
                if (IsDebugGeometryEnabled() && debugRigidBodies_.Empty())
                    SetDebugGeometryEnabled(false);
            }
    
            if (IsDebugGeometryEnabled())
                DrawDebugGeometry();
        }
        else
            impl->DrawCachedDebugLines();
    }
}

void PhysicsWorld::ProcessPostTick(float substeptime)
{
    URHO3D_PROFILE(PhysicsWorld_ProcessPostTick);
    // Check contacts and send collision signals for them
    int numManifolds = impl->collisionDispatcher->getNumManifolds();
    
    HashSet<Pair<const btCollisionObject*, const btCollisionObject*> > currentCollisions;
    
    // Collect all collision signals to a list before emitting any of them, in case a collision
    // handler changes physics state before the loop below is over (which would lead into catastrophic
    // consequences)
    Vector<CollisionSignal> collisions;
    collisions.Reserve(numManifolds * 3); // Guess some initial memory size for the collision list.

    if (numManifolds > 0)
    {
        URHO3D_PROFILE(PhysicsWorld_SendCollisions);
        
        for(int i = 0; i < numManifolds; ++i)
        {
            btPersistentManifold* contactManifold = impl->collisionDispatcher->getManifoldByIndexInternal(i);
            int numContacts = contactManifold->getNumContacts();
            if (numContacts == 0)
                continue;

            const btCollisionObject* objectA = contactManifold->getBody0();
            const btCollisionObject* objectB = contactManifold->getBody1();

            Pair<const btCollisionObject*, const btCollisionObject*> objectPair;
            if (objectA < objectB)
                objectPair = Urho3D::MakePair(objectA, objectB);
            else
                objectPair = Urho3D::MakePair(objectB, objectA);
            
            RigidBody* bodyA = static_cast<RigidBody*>(objectA->getUserPointer());
            RigidBody* bodyB = static_cast<RigidBody*>(objectB->getUserPointer());
            
            // We are only interested in collisions where both RigidBody components are known
            if (!bodyA || !bodyB)
            {
                LogError("Inconsistent Bullet physics scene state! An object exists in the physics scene which does not have an associated RigidBody!");
                continue;
            }
            // Also, both bodies should have valid parent entities
            Entity* entityA = bodyA->ParentEntity();
            Entity* entityB = bodyB->ParentEntity();
            if (!entityA || !entityB)
            {
                LogError("Inconsistent Bullet physics scene state! A parentless RigidBody exists in the physics scene!");
                continue;
            }
            // Check that at least one of the bodies is active
            if (!objectA->isActive() && !objectB->isActive())
                continue;
            
            bool newCollision = previousCollisions_.Find(objectPair) == previousCollisions_.End();
            
            for(int j = 0; j < numContacts; ++j)
            {
                btManifoldPoint& point = contactManifold->getContactPoint(j);
                
                CollisionSignal s;
                s.bodyA = bodyA;
                s.bodyB = bodyB;
                s.position = point.m_positionWorldOnB;
                s.normal = point.m_normalWorldOnB;
                s.distance = point.m_distance1;
                s.impulse = point.m_appliedImpulse;
                s.newCollision = newCollision;
                collisions.Push(s);
                
                // Report newCollision = true only for the first contact, in case there are several contacts, and application does some logic depending on it
                // (for example play a sound -> avoid multiple sounds being played)
                newCollision = false;
            }
            
            currentCollisions.Insert(objectPair);
        }
    }

    // Now fire all collision signals. Safeguard for the body components expiring in case signal handlers delete them from the scene
    {
        URHO3D_PROFILE(PhysicsWorld_emit_PhysicsCollisions);
        for(size_t i = 0; i < collisions.Size(); ++i)
        {
            const CollisionSignal &collision = collisions[i];
            const float3 &pos = collision.position;
            const float3 &normal = collision.normal;
            const float distance = collision.distance;
            const float impulse = collision.impulse;
            const bool newCollision = collision.newCollision;

            if (collision.bodyA.Expired() || collision.bodyB.Expired())
                continue;
            if (newCollision)
                NewPhysicsCollision.Emit(collision.bodyA->ParentEntity(), collision.bodyB->ParentEntity(), pos, normal, distance, impulse);
            PhysicsCollision.Emit(collision.bodyA->ParentEntity(), collision.bodyB->ParentEntity(), pos, normal, distance, impulse, newCollision);
            
            if (collision.bodyA.Expired() || collision.bodyB.Expired())
                continue;
            collision.bodyA->EmitPhysicsCollision(collision.bodyB->ParentEntity(), pos, normal, distance, impulse, newCollision);
            
            if (collision.bodyA.Expired() || collision.bodyB.Expired())
                continue;
            collision.bodyB->EmitPhysicsCollision(collision.bodyA->ParentEntity(), pos, normal, distance, impulse, newCollision);
        }
    }

    previousCollisions_ = currentCollisions;
    
    {
        URHO3D_PROFILE(PhysicsWorld_ProcessPostTick_Updated);
        Updated.Emit(substeptime);
    }
}

PhysicsRaycastResult* PhysicsWorld::Raycast(const float3& origin, const float3& direction, float maxdistance, int collisiongroup, int collisionmask)
{
    URHO3D_PROFILE(PhysicsWorld_Raycast);
    
    static PhysicsRaycastResult result;
    
    float3 normalizedDir = direction.Normalized();
    
    btCollisionWorld::ClosestRayResultCallback rayCallback(origin, origin + maxdistance * normalizedDir);
    rayCallback.m_collisionFilterGroup = (short)collisiongroup;
    rayCallback.m_collisionFilterMask = (short)collisionmask;
    
    impl->world->rayTest(rayCallback.m_rayFromWorld, rayCallback.m_rayToWorld, rayCallback);
    
    result.entity = 0;
    result.distance = 0;
    
    if (rayCallback.hasHit())
    {
        result.pos = rayCallback.m_hitPointWorld;
        result.normal = rayCallback.m_hitNormalWorld;
        result.distance = (result.pos - origin).Length();
        if (rayCallback.m_collisionObject)
        {
            RigidBody* body = static_cast<RigidBody*>(rayCallback.m_collisionObject->getUserPointer());
            if (body)
                result.entity = body->ParentEntity();
        }
    }
    
    return &result;
}

EntityVector PhysicsWorld::ObbCollisionQuery(const OBB &obb, int collisionGroup, int collisionMask)
{
    URHO3D_PROFILE(PhysicsWorld_ObbCollisionQuery);
    
    HashSet<btCollisionObjectWrapper*> objects;
    EntityVector entities;
    
    btBoxShape box(obb.HalfSize()); // Note: Bullet uses box halfsize
    float3x3 m(obb.axis[0], obb.axis[1], obb.axis[2]);
    btTransform t1(m.ToQuat(), obb.CenterPoint());
    btRigidBody* tempRigidBody = new btRigidBody(1.0f, 0, &box);
    tempRigidBody->setWorldTransform(t1);
    impl->world->addRigidBody(tempRigidBody, (short)collisionGroup, (short)collisionMask);
    tempRigidBody->activate(); // To make sure we get collision results from static sleeping rigidbodies, activate the temp rigid body
    
    ObbCallback resultCallback(objects);
    impl->world->contactTest(tempRigidBody, resultCallback);
    
    for (HashSet<btCollisionObjectWrapper*>::Iterator i = objects.Begin(); i != objects.End(); ++i)
    {
        RigidBody* body = static_cast<RigidBody*>((*i)->getCollisionObject()->getUserPointer());
        if (body && body->ParentEntity())
            entities.Push(EntityPtr(body->ParentEntity()));
    }
    
    impl->world->removeRigidBody(tempRigidBody);
    delete tempRigidBody;
    
    return entities;
}

void PhysicsWorld::SetDebugGeometryEnabled(bool enable)
{
    if (scene_.Expired() || !scene_->ViewEnabled() || IsDebugGeometryEnabled() == enable)
        return;

    /// @todo Make possible to set other debug modes too.
    impl->setDebugMode(enable ? btIDebugDraw::DBG_DrawWireframe | btIDebugDraw::DBG_DrawConstraintLimits | btIDebugDraw::DBG_DrawConstraints : btIDebugDraw::DBG_NoDebug);
}

bool PhysicsWorld::IsDebugGeometryEnabled() const
{
    return impl->IsDebugGeometryEnabled();
}

void PhysicsWorld::DrawDebugGeometry()
{
    if (!IsDebugGeometryEnabled())
        return;

    URHO3D_PROFILE(BulletPhysics_DrawDebugGeometry);
    
    // Draw debug only for the active (visible) scene
    GraphicsWorldPtr graphicsWorld = scene_->Subsystem<GraphicsWorld>();
    impl->cachedGraphicsWorld = graphicsWorld.Get();
    if (!graphicsWorld)
        return;
    if (!graphicsWorld->IsActive())
        return;
    
    // Get all lines of the physics world
    impl->PreDebugDraw();
    impl->world->debugDrawWorld();
    impl->PostDebugDraw();
}

}
