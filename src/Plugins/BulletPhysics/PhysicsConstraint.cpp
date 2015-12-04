// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#define MATH_BULLET_INTEROP

#include "PhysicsWorld.h"
#include "PhysicsConstraint.h"
#include "RigidBody.h"

#include "Framework.h"
#include "FrameAPI.h"
#include "Scene.h"
#include "Entity.h"
#include "Math/MathFunc.h"
#include "Placeable.h"
#include "AttributeMetadata.h"

#include <BulletDynamics/ConstraintSolver/btTypedConstraint.h>
#include <BulletDynamics/ConstraintSolver/btHingeConstraint.h>
#include <BulletDynamics/ConstraintSolver/btPoint2PointConstraint.h>
#include <BulletDynamics/ConstraintSolver/btSliderConstraint.h>
#include <BulletDynamics/ConstraintSolver/btConeTwistConstraint.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>

namespace Tundra
{

Quat FromEulerDegToQuat(float3 degEuler)
{
    float3 radEuler = DegToRad(degEuler);
    return Quat::FromEulerXYZ(radEuler.x, radEuler.y, radEuler.z);
}

PhysicsConstraint::PhysicsConstraint(Urho3D::Context* context, Scene* scene):
    IComponent(context, scene),
    constraint_(0),
    checkForRigidBodies(false),
    INIT_ATTRIBUTE_VALUE(enabled, "Enabled", false),
    INIT_ATTRIBUTE_VALUE(disableCollision, "Disable collision", false),
    INIT_ATTRIBUTE_VALUE(type, "Constraint type", 0),
    INIT_ATTRIBUTE_VALUE(otherEntity, "Other entity", EntityReference()),
    INIT_ATTRIBUTE_VALUE(position, "Position", float3::zero),
    INIT_ATTRIBUTE_VALUE(otherPosition, "Other position", float3::zero),
    INIT_ATTRIBUTE_VALUE(rotation, "Rotation", float3::zero),
    INIT_ATTRIBUTE_VALUE(otherRotation, "Other rotation", float3::zero), 
    INIT_ATTRIBUTE_VALUE(linearLimit, "Linear limit", float2::zero),
    INIT_ATTRIBUTE_VALUE(angularLimit, "Angular limit", float2::zero)
{
    static AttributeMetadata constraintTypeMetadata;
    static bool metadataInitialized = false;
    if(!metadataInitialized)
    {
        constraintTypeMetadata.enums[Hinge] = "Hinge";
        constraintTypeMetadata.enums[PointToPoint] = "Point to point";
        constraintTypeMetadata.enums[Slider] = "Slider";
        constraintTypeMetadata.enums[ConeTwist] = "Cone twist";
        metadataInitialized = true;
    }
    type.SetMetadata(&constraintTypeMetadata);

    ParentEntitySet.Connect(this, &PhysicsConstraint::UpdateSignals);
}

PhysicsConstraint::~PhysicsConstraint()
{
    Remove();
}

void PhysicsConstraint::UpdateSignals()
{
    Entity *parentEntity = ParentEntity();
    if (!parentEntity)
        return;

    Scene* scene = parentEntity->ParentScene();
    physicsWorld_ = scene->Subsystem<PhysicsWorld>();

    GetFramework()->Frame()->Updated.Connect(this, &PhysicsConstraint::CheckForBulletRigidBody);
    parentEntity->ComponentAdded.Connect(this, &PhysicsConstraint::OnComponentAdded);
    parentEntity->ComponentRemoved.Connect(this, &PhysicsConstraint::OnComponentRemoved);
}

void PhysicsConstraint::OnEntityRemoved(Entity* /*entity*/, AttributeChange::Type /*change*/)
{
    Remove();
}

void PhysicsConstraint::OnComponentAdded(IComponent *component, AttributeChange::Type /*change*/)
{
    if (component->TypeId() == RigidBody::TypeIdStatic())
        Create();
}

void PhysicsConstraint::OnComponentRemoved(IComponent *component, AttributeChange::Type /*change*/)
{
    if (component->TypeId() == RigidBody::TypeIdStatic())
        Remove();
}

void PhysicsConstraint::CheckForBulletRigidBody(float /*frameTime*/)
{
    if (!checkForRigidBodies)
        return;

    RigidBody *rigidBody = rigidBody_.Get();
    RigidBody *otherRigidBody = otherRigidBody_.Get();
    const bool rigidBodyCreated = rigidBody && rigidBody->BulletRigidBody();
    const bool otherBodyCreated = otherRigidBody && otherRigidBody->BulletRigidBody();
    if (rigidBodyCreated || otherBodyCreated)
    {
        Create();
        if (constraint_)
            checkForRigidBodies = false;
    }
}

void PhysicsConstraint::AttributesChanged()
{
    bool recreate = false;
    bool applyAttributes = false;
    bool applyLimits = false;

    if (enabled.ValueChanged())
    {
        if (constraint_)
            constraint_->setEnabled(enabled.Get());
        else
            recreate = true;
    }

    if (disableCollision.ValueChanged())
        recreate = true;
    if (type.ValueChanged())
        recreate = true;
    if (otherEntity.ValueChanged())
        recreate = true;
    if (position.ValueChanged())
        applyAttributes = true;
    if (otherPosition.ValueChanged())
        applyAttributes = true;
    if (rotation.ValueChanged())
        applyAttributes = true;
    if (otherRotation.ValueChanged())
        applyAttributes = true;
    if (linearLimit.ValueChanged())
        applyLimits = true;
    if (angularLimit.ValueChanged())
        applyLimits = true;

    if (recreate)
        Create();
    if (!recreate && applyAttributes)
        ApplyAttributes();
    if (!recreate && applyLimits)
        ApplyLimits();
}

void PhysicsConstraint::Create()
{
    if (!ParentEntity() || physicsWorld_.Expired())
        return;

    Remove();

    rigidBody_ = ParentEntity()->Component<RigidBody>();

    /// \todo If the other entity is not yet loaded, the constraint will be mistakenly created as a static one
    /// \todo Add warning logging if the other entity is not found, or for other error situations
    Entity *otherEnt = 0;
    if (!otherEntity.Get().IsEmpty())
    {
        otherEnt = otherEntity.Get().Lookup(ParentScene()).Get();
        if (otherEnt)
        {
            otherRigidBody_ = otherEnt->Component<RigidBody>();
            /// \todo Disconnect these signals at constraint removal time, in case the other entity ID is changed at runtime
            otherEnt->EntityRemoved.Connect(this, &PhysicsConstraint::OnEntityRemoved);
            otherEnt->ComponentAdded.Connect(this, &PhysicsConstraint::OnComponentAdded);
            otherEnt->ComponentRemoved.Connect(this, &PhysicsConstraint::OnComponentRemoved);
        }
        else
            otherRigidBody_.Reset();
    }
    else
        otherRigidBody_.Reset();

    RigidBody *rigidBodyComp = rigidBody_.Get();
    RigidBody *otherRigidBodyComp = otherRigidBody_.Get();
    btRigidBody *ownBody = rigidBodyComp ? rigidBodyComp->BulletRigidBody() : 0;
    btRigidBody *otherBody = otherRigidBodyComp ? otherRigidBodyComp->BulletRigidBody() : 0;

    if (!ownBody && !rigidBodyComp)
        return;

    else if (!ownBody && rigidBodyComp)
    {
        checkForRigidBodies = true;
        return;
    }

    if (!otherBody && !otherRigidBodyComp)
        otherBody = &btTypedConstraint::getFixedBody();
    else if (!otherBody && otherRigidBodyComp)
    {
        checkForRigidBodies = true;
        return;
    }

    float3 worldScale(1,1,1);
    float3 otherWorldScale(1,1,1);
    
    Placeable *placeable = ParentEntity()->Component<Placeable>().Get();
    Placeable *otherPlaceable = 0;
    if (otherEnt)
        otherPlaceable = otherEnt->Component<Placeable>().Get();
    
    if (placeable)
        worldScale = placeable->WorldScale();
    if (otherPlaceable)
        otherWorldScale = otherPlaceable->WorldScale();

    btTransform ownTransform(FromEulerDegToQuat(rotation.Get()), position.Get().Mul(worldScale));
    btTransform otherTransform(FromEulerDegToQuat(otherRotation.Get()), otherPosition.Get().Mul(otherWorldScale));

    switch(type.Get())
    {
        case PointToPoint:
            constraint_ = new btPoint2PointConstraint(*ownBody, *otherBody, position.Get().Mul(worldScale), otherPosition.Get().Mul(otherWorldScale));
            break;

        case Hinge:
            constraint_ = new btHingeConstraint(*ownBody, *otherBody, ownTransform, otherTransform);
            break;

        case Slider:
            constraint_ = new btSliderConstraint(*ownBody, *otherBody, ownTransform, otherTransform, false);
            break;

        case ConeTwist:
            constraint_ = new btConeTwistConstraint(*ownBody, *otherBody, ownTransform, otherTransform);
            break;

        default:
            break;
    }

    if (constraint_)
    {
        constraint_->setUserConstraintPtr(this);
        constraint_->setEnabled(enabled.Get());
        ApplyLimits();

        PhysicsWorld *world = physicsWorld_.Get();
        world->BulletWorld()->addConstraint(constraint_, disableCollision.Get());
    }
}

void PhysicsConstraint::Remove()
{
    if (constraint_)
    {
        RigidBody *ownRigidComp = rigidBody_.Get();
        RigidBody *otherRigidComp = otherRigidBody_.Get();
        if (otherRigidComp && otherRigidComp->BulletRigidBody())
            otherRigidComp->BulletRigidBody()->removeConstraintRef(constraint_);
        if (ownRigidComp && ownRigidComp->BulletRigidBody())
            ownRigidComp->BulletRigidBody()->removeConstraintRef(constraint_);

        PhysicsWorld *world = physicsWorld_.Get();
        if (world && world->BulletWorld())
            world->BulletWorld()->removeConstraint(constraint_);

        rigidBody_.Reset();
        otherRigidBody_.Reset();
        delete constraint_;
        constraint_ = 0;
    }
}

void PhysicsConstraint::ApplyAttributes()
{
    if (!constraint_)
        return;

    float3 worldScale(1,1,1);
    float3 otherWorldScale(1,1,1);

    Placeable *placeable = ParentEntity()->Component<Placeable>().Get();
    Entity *entity = otherEntity.Get().Lookup(ParentScene()).Get();
    Placeable *otherPlaceable = 0;
    if (entity)
        otherPlaceable = entity->Component<Placeable>().Get();

    if (placeable)
        worldScale = placeable->WorldScale();
    if (otherPlaceable)
        otherWorldScale = otherPlaceable->WorldScale();

    btTransform ownTransform(FromEulerDegToQuat(rotation.Get()), position.Get().Mul(worldScale));
    btTransform otherTransform(FromEulerDegToQuat(otherRotation.Get()), otherPosition.Get().Mul(otherWorldScale));

    switch (constraint_->getConstraintType())
    {
        case POINT2POINT_CONSTRAINT_TYPE:
        {
            btPoint2PointConstraint* pointConstraint = static_cast<btPoint2PointConstraint*>(constraint_);
            pointConstraint->setPivotA(position.Get().Mul(worldScale));
            pointConstraint->setPivotB(otherPosition.Get().Mul(otherWorldScale));
        }
            break;
            
        case HINGE_CONSTRAINT_TYPE:
        {
            btHingeConstraint* hingeConstraint = static_cast<btHingeConstraint*>(constraint_);
            hingeConstraint->setFrames(ownTransform, otherTransform);
        }
            break;
            
        case SLIDER_CONSTRAINT_TYPE:
        {
            btSliderConstraint* sliderConstraint = static_cast<btSliderConstraint*>(constraint_);
            sliderConstraint->setFrames(ownTransform, otherTransform);
        }
            break;
            
        case CONETWIST_CONSTRAINT_TYPE:
        {
            btConeTwistConstraint* coneTwistConstraint = static_cast<btConeTwistConstraint*>(constraint_);
            coneTwistConstraint->setFrames(ownTransform, otherTransform);
        }
            break;

        default:
            break;
    }
}

void PhysicsConstraint::ApplyLimits()
{
    if (!constraint_)
        return;

    switch (constraint_->getConstraintType())
    {
        case HINGE_CONSTRAINT_TYPE:
        {
            btHingeConstraint* hingeConstraint = static_cast<btHingeConstraint*>(constraint_);
            hingeConstraint->setLimit(DegToRad(angularLimit.Get().x), DegToRad(angularLimit.Get().y));
        }
            break;
            
        case SLIDER_CONSTRAINT_TYPE:
        {
            btSliderConstraint* sliderConstraint = static_cast<btSliderConstraint*>(constraint_);
            
            sliderConstraint->setLowerLinLimit(linearLimit.Get().x);
            sliderConstraint->setUpperLinLimit(linearLimit.Get().y);

            sliderConstraint->setLowerAngLimit(DegToRad(angularLimit.Get().x));
            sliderConstraint->setUpperAngLimit(DegToRad(angularLimit.Get().y));
        }
            break;
            
        case CONETWIST_CONSTRAINT_TYPE:
        {
            btConeTwistConstraint* coneTwistConstraint = static_cast<btConeTwistConstraint*>(constraint_);
            coneTwistConstraint->setLimit(DegToRad(angularLimit.Get().y), DegToRad(angularLimit.Get().y), DegToRad(linearLimit.Get().y));
        }
            break;
            
        default:
            break;
    }
}

}
