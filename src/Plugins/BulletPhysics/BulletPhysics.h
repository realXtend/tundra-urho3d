// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "BulletPhysicsApi.h"
#include "BulletPhysicsFwd.h"
#include "IModule.h"
#include "SceneFwd.h"
#include "StdPtr.h"
#include "AttributeChangeType.h"

namespace Urho3D
{
    class Model;
    class UIElement;
}

namespace Tundra
{

class IMeshAsset;

/// Provides physics rendering by utilizing Bullet.
class BULLETPHYSICS_API BulletPhysics : public IModule
{
public:
    BulletPhysics(Framework* owner);
    virtual ~BulletPhysics();

    void Load();
    void Initialize();
    void Update(float frametime);
    void Uninitialize();

    /// Forget cache bullet shapes.
    /** Code that loads into the cache with calling GetTriangleMeshFromMesh and GetConvexHullSetFromMesh is
        responsible to call this function when it has reseted its own shared ptr, to ensure if your code was the last
        use of this particular Mesh, the shapes memory will get released. */
    int ForgetUnusedCacheShapes();

    /// Get a Bullet triangle mesh corresponding to a graphics mesh.
    /** If already has been generated, returns the previously created one */
    shared_ptr<btTriangleMesh> GetTriangleMeshFromMeshAsset(IMeshAsset* mesh);

    /// Get a Bullet convex hull set (using minimum recursion, not very accurate but fast) corresponding to an Ogre mesh.
    /** If already has been generated, returns the previously created one */
    shared_ptr<ConvexHullSet> GetConvexHullSetFromMeshAsset(IMeshAsset* mesh);

    /// Set default physics update rate for new physics worlds
    void SetDefaultPhysicsUpdatePeriod(float updatePeriod);

    /// Return default physics update rate for new physics worlds
    float DefaultPhysicsUpdatePeriod() const { return defaultPhysicsUpdatePeriod_; }

    /// Set default physics max substeps for new physics worlds
    void SetDefaultMaxSubSteps(int steps);

    /// Return default physics max substeps for new physics worlds
    int DefaultMaxSubSteps() const { return defaultMaxSubSteps_; }

    /// Toggles physics debug geometry
    void ToggleDebugGeometry();

    /// Stops physics for all physics worlds
    void StopPhysics();

    /// Starts physics for all physics worlds
    void StartPhysics();

    /// Autoassigns static rigid bodies with collision meshes to visible meshes
    void AutoCollisionMesh();

    /// Enable/disable physics simulation from all physics worlds
    void SetRunPhysics(bool enable);
    
private:
    /// Creates PhysicsWorld for a Scene.
    void CreatePhysicsWorld(Scene *scene, AttributeChange::Type change);
    /// Removes PhysicsWorld of a Scene.
    void RemovePhysicsWorld(Scene *scene, AttributeChange::Type change);

    /// All PhysicsWorlds created.
    Vector<PhysicsWorldPtr> physicsWorlds_;

    typedef HashMap<String, shared_ptr<btTriangleMesh> > TriangleMeshMap;
    /// Bullet triangle meshes generated from graphics meshes
    TriangleMeshMap triangleMeshes_;

    typedef HashMap<String, shared_ptr<ConvexHullSet> > ConvexHullSetMap;
    /// Bullet convex hull sets generated from graphics meshes
    ConvexHullSetMap convexHullSets_;
    
    float defaultPhysicsUpdatePeriod_;
    int defaultMaxSubSteps_;
};

}

#ifdef PROFILING
//void BULLETPHYSICS_API UpdateBulletProfilingData(UIElement *treeRoot, int numFrames);
#endif
