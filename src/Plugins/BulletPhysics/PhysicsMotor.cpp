// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

#include "PhysicsMotor.h"
#include "RigidBody.h"
#include "Placeable.h"
#include "Entity.h"
#include "Scene/Scene.h"
#include "BulletPhysics.h"
#include "PhysicsWorld.h"
#include "PhysicsUtils.h"
#include "LoggingFunctions.h"

#include <Urho3D/Core/Profiler.h>

namespace Tundra
{

PhysicsMotor::PhysicsMotor(Urho3D::Context* context, Scene* scene) :
    IComponent(context, scene),
    INIT_ATTRIBUTE_VALUE(absoluteMoveForce, "Absolute Move Force", float3::zero),
    INIT_ATTRIBUTE_VALUE(relativeMoveForce, "Relative Move Force", float3::zero),
    INIT_ATTRIBUTE_VALUE(dampingForce, "Damping Force", float3::zero)
{
    ParentEntitySet.Connect(this, &PhysicsMotor::UpdateSignals);
}

PhysicsMotor::~PhysicsMotor()
{
}

void PhysicsMotor::UpdateSignals()
{
    Entity* parent = ParentEntity();
    if (!parent)
        return;
    Scene* scene = parent->ParentScene();
    PhysicsWorld* world = scene->Subsystem<PhysicsWorld>().Get();
    if (world)
        world->Updated.Connect(this, &PhysicsMotor::OnPhysicsUpdate);
    
    parent->ComponentAdded.Connect(this, &PhysicsMotor::OnComponentStructureChanged);
    parent->ComponentRemoved.Connect(this, &PhysicsMotor::OnComponentStructureChanged);
    OnComponentStructureChanged(this, AttributeChange::Default); // Check for Rigidbody & Placeable immediately
}

void PhysicsMotor::OnComponentStructureChanged(IComponent*, AttributeChange::Type)
{
    Entity* parent = ParentEntity();
    if (!parent)
        return;
    rigidBody_ = parent->Component<RigidBody>();
    placeable_ = parent->Component<Placeable>();
}

void PhysicsMotor::OnPhysicsUpdate(float /*timeStep*/)
{
    URHO3D_PROFILE(PhysicsMotor_OnPhysicsUpdate);
    
    RigidBody* body = rigidBody_.Get();
    Placeable* placeable = placeable_.Get();
    
    if (body && placeable)
    {
        float3 absolute = absoluteMoveForce.Get();
        if (!absolute.Equals(float3::zero))
            body->ApplyImpulse(absolute);
        
        float3 relative = relativeMoveForce.Get();
        if (!relative.Equals(float3::zero))
        {
            float3 translate, scale;
            Quat rot;
            
            placeable->LocalToWorld().Decompose(translate, rot, scale);
            relative = rot * relative;
            body->ApplyImpulse(relative);
        }
        
        float3 damping = dampingForce.Get();
        if (!damping.Equals(float3::zero))
        {
            damping = -body->GetLinearVelocity().Mul(damping);
            body->ApplyImpulse(damping);
        }
    }
}

}
