// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IComponent.h"
#include "CoreDefines.h"
#include "Math/float3.h"
#include "BulletPhysicsFwd.h"

namespace Tundra
{

class Placeable;

/// Physics motor component.
/** <table class="header">
    <tr>
    <td>
    <h2>PhysicsMotor</h2>
    Physics motor component
    Drives a RigidBody by impulses on each physics update, and optionally applies a damping (braking) force.

    Registered by BulletPhysics

    <b>Attributes</b>:
    <ul>
    <li>float3: absoluteMoveForce
    <div>@copydoc absoluteMoveForce</div>
    <li>float3: relativeMoveForce
    <div>@copydoc relativeMoveForce</div>
    <li>float3: dampingForce
    <div>@copydoc dampingForce</div>
    </ul>

    <b>Does not react on any actions.</b>
    </td>
    </tr>

    Does not emit any actions.

    <b>Depends on the components @ref RigidBody "RigidBody" and @ref Placeable "Placeable".</b>.

    </table> */
class PhysicsMotor : public IComponent
{
    COMPONENT_NAME(PhysicsMotor, 43)

public:
    /// @cond PRIVATE
    /// Do not directly allocate new components using operator new, but use the factory-based SceneAPI::CreateComponent functions instead.
    explicit PhysicsMotor(Urho3D::Context* context, Scene* scene);
    /// @endcond
    virtual ~PhysicsMotor();

    /// World-space force applied to rigid body on each physics update.
    Attribute<float3> absoluteMoveForce;

    /// Local-space force applied to rigid body on each physics update.
    Attribute<float3> relativeMoveForce;

    /// Force proportional and opposite to linear velocity that will be applied to rigid body on each physics update to slow it down.
    Attribute<float3> dampingForce;

private:
    /// Called when the parent entity has been set.
    void UpdateSignals();

    /// Called when component has been added or removed from the parent entity. Checks existence of RigidBody & Placeable components.
    void OnComponentStructureChanged(IComponent*, AttributeChange::Type);
    
    /// Apply forces during each physics update.
    void OnPhysicsUpdate(float timeStep);
    
    /// Cached rigidbody pointer
    WeakPtr<RigidBody> rigidBody_;
    /// Cached placeable pointer
    WeakPtr<Placeable> placeable_;
};
COMPONENT_TYPEDEFS(PhysicsMotor);

}
