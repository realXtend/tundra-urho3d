// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

#include "BulletPhysics.h"
#include "PhysicsWorld.h"
#include "CollisionShapeUtils.h"
#include "ConvexHull.h"
#include "RigidBody.h"
#include "VolumeTrigger.h"
#include "PhysicsMotor.h"
#include "PhysicsConstraint.h"

#include "UrhoRenderer.h"
#include "Mesh.h"
#include "Placeable.h"
#include "Terrain.h"
#include "Entity.h"
#include "SceneAPI.h"
#include "Framework.h"
#include "Scene/Scene.h"
#include "ConsoleAPI.h"
#include "IComponentFactory.h"
#include "LoggingFunctions.h"
#include "IMeshAsset.h"

#include "JavaScript.h"
#include "JavaScriptInstance.h"
#include "BulletPhysicsBindings/BulletPhysicsBindings.h"

#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Core/Profiler.h>

// Disable unreferenced formal parameter coming from Bullet
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4100)
#endif
#include <btBulletDynamicsCommon.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

using namespace JSBindings;

namespace Tundra
{

BulletPhysics::BulletPhysics(Framework* owner)
:IModule("BulletPhysics", owner),
defaultPhysicsUpdatePeriod_(1.0f / 60.0f),
defaultMaxSubSteps_(6) // If fps is below 10, we start to slow down physics
{
}

BulletPhysics::~BulletPhysics()
{
}

void BulletPhysics::Load()
{
    framework->Scene()->RegisterComponentFactory(ComponentFactoryPtr(new GenericComponentFactory<RigidBody>()));
    framework->Scene()->RegisterComponentFactory(ComponentFactoryPtr(new GenericComponentFactory<VolumeTrigger>()));
    framework->Scene()->RegisterComponentFactory(ComponentFactoryPtr(new GenericComponentFactory<PhysicsMotor>()));
    framework->Scene()->RegisterComponentFactory(ComponentFactoryPtr(new GenericComponentFactory<PhysicsConstraint>()));
}

void BulletPhysics::Initialize()
{
    framework->Scene()->SceneCreated.Connect(this, &BulletPhysics::CreatePhysicsWorld);
    framework->Scene()->SceneAboutToBeRemoved.Connect(this, &BulletPhysics::RemovePhysicsWorld);

    framework->Console()->RegisterCommand("physicsDebug",
        "Toggles drawing of physics debug geometry.",
        this, &BulletPhysics::ToggleDebugGeometry);
    framework->Console()->RegisterCommand("stopPhysics",
        "Stops physics simulation.",
        this, &BulletPhysics::StopPhysics);
    framework->Console()->RegisterCommand("startPhysics",
        "(Re)starts physics simulation.",
        this, &BulletPhysics::StartPhysics);
    framework->Console()->RegisterCommand("autoCollisionMesh",
        "Auto-assigns static rigid bodies with collision mesh to all visible meshes.",
        this, &BulletPhysics::AutoCollisionMesh);
    
    // Check physics execution rate related command line parameters
    StringList params = framework->CommandLineParameters("--physicsRate");
    if (!params.Empty())
    {
        int rate = Urho3D::ToInt(params.Front());
        if (rate > 0)
            SetDefaultPhysicsUpdatePeriod(1.0f / (float)rate);
    }

    params = framework->CommandLineParameters("--physicsMaxSteps");
    if (!params.Empty())
    {
        int steps = Urho3D::ToInt(params.Front());
        if (steps > 0)
            SetDefaultMaxSubSteps(steps);
    }
    
    // Connect to JavaScript module instance creation to be able to expose the physics classes to each instance
    JavaScript* javaScript = framework->Module<JavaScript>();
    if (javaScript)
        javaScript->ScriptInstanceCreated.Connect(this, &BulletPhysics::OnScriptInstanceCreated);
}

void BulletPhysics::Uninitialize()
{
}

void BulletPhysics::ToggleDebugGeometry()
{
    for (Vector<PhysicsWorldPtr>::Iterator i = physicsWorlds_.Begin(); i != physicsWorlds_.End(); ++i)
    {
        (*i)->SetDebugGeometryEnabled(!(*i)->IsDebugGeometryEnabled());
        (*i)->drawDebugManuallySet_ = true; // Disable automatic debugdraw state change
    }
}

void BulletPhysics::SetDefaultPhysicsUpdatePeriod(float updatePeriod)
{
    // Allow max.1000 fps
    if (updatePeriod <= 0.001f)
        updatePeriod = 0.001f;
    defaultPhysicsUpdatePeriod_ = updatePeriod;
}

void BulletPhysics::SetDefaultMaxSubSteps(int steps)
{
    if (steps > 0)
        defaultMaxSubSteps_ = steps;
}

void BulletPhysics::StopPhysics()
{
    SetRunPhysics(false);
}

void BulletPhysics::StartPhysics()
{
    SetRunPhysics(true);
}

void BulletPhysics::AutoCollisionMesh()
{
    Scene *scene = GetFramework()->Scene()->MainCameraScene();
    if (!scene)
    {
        LogError("BulletPhysics::AutoCollisionMesh: No active scene!");
        return;
    }
    
    for(Scene::Iterator iter = scene->Begin(); iter != scene->End(); ++iter)
    {
        EntityPtr entity = iter->second_;
        // Only assign to entities that don't have a rigidbody yet, but have a mesh and a placeable
        if (!entity->Component<RigidBody>() && entity->Component<Placeable>() && entity->Component<Mesh>())
        {
            SharedPtr<RigidBody> body = entity->GetOrCreateComponent<RigidBody>();
            body->SetShapeFromVisibleMesh();
        }
        // Terrain mode: assign if no rigid body, but there is a terrain component
        if (!entity->Component<RigidBody>() && entity->Component<Terrain>())
        {
            SharedPtr<RigidBody> body = entity->GetOrCreateComponent<RigidBody>();
            body->shapeType.Set(RigidBody::HeightField, AttributeChange::Default);
        }
    }
}

void BulletPhysics::Update(float frametime)
{
    URHO3D_PROFILE(BulletPhysics_Update);
    // Loop all the physics worlds and update them.
    Vector<PhysicsWorldPtr>::Iterator i = physicsWorlds_.Begin();
    while(i != physicsWorlds_.End())
    {
        (*i)->Simulate(frametime);
        ++i;
    }
}

void BulletPhysics::CreatePhysicsWorld(Scene *scene, AttributeChange::Type /*change*/)
{
    SharedPtr<PhysicsWorld> newWorld(new PhysicsWorld(scene, !scene->IsAuthority()));
    newWorld->SetGravity(scene->UpVector() * -9.81f);
    newWorld->SetPhysicsUpdatePeriod(defaultPhysicsUpdatePeriod_);
    newWorld->SetMaxSubSteps(defaultMaxSubSteps_);
    scene->AddSubsystem("physics", newWorld);
    physicsWorlds_.Push(newWorld);
}

void BulletPhysics::RemovePhysicsWorld(Scene *scene, AttributeChange::Type /*change*/)
{
    PhysicsWorldPtr worldPtr = scene->Subsystem<PhysicsWorld>();
    scene->RemoveSubsystem("physics");
    physicsWorlds_.Remove(worldPtr);

    // At this point all Entities have been destroyed. There should be nothing in the cache
    // if there are no bugs in RigidBody. Either way make sure to cleanup cached shape memory.
    ForgetUnusedCacheShapes();
}

void BulletPhysics::SetRunPhysics(bool enable)
{
    for (Vector<PhysicsWorldPtr>::Iterator i = physicsWorlds_.Begin(); i != physicsWorlds_.End(); ++i)
        (*i)->SetRunning(enable);
}

int BulletPhysics::ForgetUnusedCacheShapes()
{
    /* This function check shared ptrs where use count == 1, meaning our cache map is the only
       this keeping the ptr alive. These will be forgotten. */
    int forgotten = 0;

    for (TriangleMeshMap::Iterator iter = triangleMeshes_.Begin(), end = triangleMeshes_.End();
        iter != end;)
    {
        shared_ptr<btTriangleMesh> &ptr = iter->second_;
        if (ptr.use_count() == 1)
        {
            iter = triangleMeshes_.Erase(iter);
            forgotten++;
        }
        else
            iter++;
    }

    for (ConvexHullSetMap::Iterator iter = convexHullSets_.Begin(), end = convexHullSets_.End();
        iter != end;)
    {
        shared_ptr<ConvexHullSet> &ptr = iter->second_;
        if (ptr.use_count() == 1)
        {
            iter = convexHullSets_.Erase(iter);
            forgotten++;
        }
        else
            iter++;
    }

    return forgotten;
}

shared_ptr<btTriangleMesh> BulletPhysics::GetTriangleMeshFromMeshAsset(IMeshAsset* mesh)
{
    shared_ptr<btTriangleMesh> ptr;
    if (!mesh)
        return ptr;
    
    // Check if has already been converted
    TriangleMeshMap::ConstIterator iter = triangleMeshes_.Find(mesh->Name());
    if (iter != triangleMeshes_.End())
        return iter->second_;
    
    // Create new, then interrogate the mesh
    ptr = shared_ptr<btTriangleMesh>(new btTriangleMesh());
    GenerateTriangleMesh(mesh, ptr.get());
    
    triangleMeshes_[mesh->Name()] = ptr;
    
    return ptr;
}

shared_ptr<ConvexHullSet> BulletPhysics::GetConvexHullSetFromMeshAsset(IMeshAsset* mesh)
{
    shared_ptr<ConvexHullSet> ptr;
    if (!mesh)
        return ptr;
    
    // Check if has already been converted
    ConvexHullSetMap::ConstIterator iter = convexHullSets_.Find(mesh->Name());
    if (iter != convexHullSets_.End())
        return iter->second_;
    
    // Create new, then interrogate the mesh
    ptr = shared_ptr<ConvexHullSet>(new ConvexHullSet());
    GenerateConvexHullSet(mesh, ptr.get());

    convexHullSets_[mesh->Name()] = ptr;
    
    return ptr;
}

void BulletPhysics::OnScriptInstanceCreated(JavaScriptInstance* instance)
{
    URHO3D_PROFILE(ExposeBulletPhysicsClasses);

    duk_context* ctx = instance->Context();
    ExposeBulletPhysicsClasses(ctx);
    
    instance->RegisterService("physics", this);
}

}

extern "C"
{

DLLEXPORT void TundraPluginMain(Tundra::Framework *fw)
{
    fw->RegisterModule(new Tundra::BulletPhysics(fw));
}

}
