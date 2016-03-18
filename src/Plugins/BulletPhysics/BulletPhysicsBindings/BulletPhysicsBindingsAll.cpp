// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "BulletPhysicsBindings.h"

namespace JSBindings
{

void Expose_PhysicsConstraint(duk_context* ctx);
void Expose_PhysicsMotor(duk_context* ctx);
void Expose_PhysicsRaycastResult(duk_context* ctx);
void Expose_PhysicsWorld(duk_context* ctx);
void Expose_RigidBody(duk_context* ctx);
void Expose_VolumeTrigger(duk_context* ctx);
void Expose_BulletPhysics(duk_context* ctx);

void ExposeBulletPhysicsClasses(duk_context* ctx)
{
    Expose_PhysicsConstraint(ctx);
    Expose_PhysicsMotor(ctx);
    Expose_PhysicsRaycastResult(ctx);
    Expose_PhysicsWorld(ctx);
    Expose_RigidBody(ctx);
    Expose_VolumeTrigger(ctx);
    Expose_BulletPhysics(ctx);
}

}