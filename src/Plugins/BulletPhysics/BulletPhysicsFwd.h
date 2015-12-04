/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   BulletPhysicsFwd.h
    @brief  Forward declarations and type defines for commonly used BulletPhysics plugin classes. */

#pragma once

#include "CoreTypes.h"

namespace Tundra
{
    struct ConvexHull;
    struct ConvexHullSet;

    class BulletPhysics;
    class PhysicsWorld;
    struct PhysicsRaycastResult;
    class RigidBody;
    class VolumeTrigger;

    typedef SharedPtr<PhysicsWorld> PhysicsWorldPtr;
    typedef WeakPtr<PhysicsWorld> PhysicsWorldWeakPtr;
}

// From Bullet:
class btTriangleMesh;
class btCollisionConfiguration;
class btBroadphaseInterface;
class btConstraintSolver;
class btDiscreteDynamicsWorld;
class btDispatcher;
class btCollisionObject;
class btConvexHullShape;
class btRigidBody;
class btCollisionShape;
class btHeightfieldTerrainShape;

