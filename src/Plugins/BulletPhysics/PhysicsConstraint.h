// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IComponent.h"
#include "Math/Quat.h"
#include "Math/float3.h"
#include "Math/float2.h"
#include "EntityReference.h"
#include "PhysicsWorld.h"

class btTypedConstraint;

namespace Tundra
{

class RigidBody;

/// Physics constraint entity-component
/** <table class="header">
    <tr>
    <td>
    <h2>PhysicsConstraint</h2>
    Physics constraint entity-component
 
    Registered by BulletPhysics.
 
    <b>Attributes</b>:
    <ul>
    <li>bool: enabled
    <div>@copydoc enabled</div>
    <li>EntityReference: otherEntity
    <div>@copydoc @copydoc otherEntity</div>
    <li>enum: type
    <div>@copydoc type</div>
    <li>float3: position
    <div>@copydoc position</div>
    <li>float3: otherPosition
    <div>@copydoc otherPosition</div>
    <li>float3: rotation
    <div>@copydoc rotation</div>
    <li>float3: otherRotation
    <div>@copydoc otherRotation</div>
    <li>float2: lowerLimit
    <div>@copydoc lowerLimit</div>
    <li>float2: upperLimit
    <div>@copydoc upperLimit</div>
    </ul>

    <b>Reacts on the following actions:</b>
    <ul>
    <li>None.
    </ul>
    </td>
    </tr>
 
    Does not emit any actions.
 
    <b>Depends on the component @ref RigidBody "RigidBody", and @ref Placeable "Placeable"</b>.
    </table> */
class PhysicsConstraint : public IComponent
{
    COMPONENT_NAME(PhysicsConstraint, 53)

public:
    /// @cond PRIVATE
    /// Do not directly allocate new components using operator new, but use the factory-based SceneAPI::CreateComponent functions instead.
    explicit PhysicsConstraint(Urho3D::Context* context, Scene* scene);
    /// @endcond
    virtual ~PhysicsConstraint();

    enum ConstraintType
    {
        PointToPoint = 0, ///< Point to point constraint
        Hinge, ///< Hinge constraint
        Slider, ///< Slider constraint
        ConeTwist, ///< Cone twist constraint
    };

    /// Enables / disables the constraint on this entity.
    Attribute<bool> enabled;

    /// Disables collision between the bodies this constraint connects.
    Attribute<bool> disableCollision;

    /// Applies this constraint on otherEntity. If this attribute is empty or invalid, the constraint will be static i.e. to an invisible fixed body.
    Attribute<EntityReference> otherEntity;

    /// Constraint type
    /** @see ConstraintType */
    Attribute<uint> type;

    /// Pivot point A.
    /** @note: These coordinates are in this entity's local space */
    Attribute<float3> position;

    /// Pivot point B.
    /** @note: If otherEntity points to a valid entity that has RigidBody, these cordinates are in this entity's local space; otherwise they are in world space. */
    Attribute<float3> otherPosition;

    /// Orientation of pivot point A represented in euler angles in degrees.
    /** @note: These cordinates are in this entity's local space. This attribute has no effect on PointToPoint constraint type. */
    Attribute<float3> rotation;

    /// Orientation of pivot point B represented in euler angles in degrees.
    /** @note: If otherEntity points to a valid entity, these cordinates are in this entity's local space; otherwise they are in world space. This attribute has no effect on PointToPoint constraint type. */
    Attribute<float3> otherRotation;

    /// Linear limit, ranging from x to y.
    /** @note: Affects only Slider constraint type, and the 'y' component is needed for ConeTwist constraint to set the twist span.*/
    Attribute<float2> linearLimit;

    /// Angular limit, ranging from x to y. The angular limit is in degrees.
    /** @note: This attribute has no effect on PointToPoint constraint type. */
    Attribute<float2> angularLimit;

private:
    /// Creates or re-creates this constraint. The parent entity must have RigidBody component in advance
    void Create();
    /// Removes this constraint
    void Remove();
    /// Called when parent entity has been set
    void UpdateSignals();
    /// Called when the other entity is removed. This will remove the constraint
    void OnEntityRemoved(Entity *entity, AttributeChange::Type change);
    /// Called when a component had been added to the parent entity
    void OnComponentAdded(IComponent *component, AttributeChange::Type change);
    /// Called when a component had been removed to the parent entity
    void OnComponentRemoved(IComponent *component, AttributeChange::Type change);
    /// A helper function that checks for btRigidBody pointers in cases when RigidBody is present but not yet created a btRigidBody
    void CheckForBulletRigidBody(float frameTime);

    /// Called when some of the attributes are changed
    void AttributesChanged();
    /// Applies constraint attributes
    void ApplyAttributes();
    /// Applies constraint limits
    void ApplyLimits();

    /// Enables checking for btRigidBody pointers
    bool checkForRigidBodies;

    /// Cached pointer of this entity's rigid body
    WeakPtr<RigidBody> rigidBody_;
    /// Cached pointer of the other entity's rigid body
    WeakPtr<RigidBody> otherRigidBody_;
    /// Cached physics world pointer
    WeakPtr<PhysicsWorld> physicsWorld_;
    /// Constraint pointer
    btTypedConstraint *constraint_;
};
COMPONENT_TYPEDEFS(PhysicsConstraint);

}
