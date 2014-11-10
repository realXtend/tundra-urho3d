// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "Placeable.h"
#include "GraphicsWorld.h"
#include "Renderer.h"
#include "Mesh.h"
#include "AttributeMetadata.h"
#include "Entity.h"
#include "Scene/Scene.h"
#include "LoggingFunctions.h"

#include <Math/Quat.h>
#include <Math/float3x3.h>
#include <Math/float3x4.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Node.h>
#include <Engine/Core/Profiler.h>

namespace Tundra
{

Placeable::Placeable(Urho3D::Context* context, Scene* scene) :
    IComponent(context, scene),
    attached_(false),
    INIT_ATTRIBUTE(transform, "Transform"),
    INIT_ATTRIBUTE_VALUE(drawDebug, "Show bounding box", false),
    INIT_ATTRIBUTE_VALUE(visible, "Visible", true),
    INIT_ATTRIBUTE_VALUE(selectionLayer, "Selection layer", 1),
    INIT_ATTRIBUTE_VALUE(parentRef, "Parent entity ref", EntityReference()),
    INIT_ATTRIBUTE_VALUE(parentBone, "Parent bone name", "")
{
    // Enable network interpolation for the transform
    static AttributeMetadata transAttrData;
    static AttributeMetadata nonDesignableAttrData;
    static bool metadataInitialized = false;
    if(!metadataInitialized)
    {
        transAttrData.interpolation = AttributeMetadata::Interpolate;
        nonDesignableAttrData.designable = false;
        metadataInitialized = true;
    }
    transform.SetMetadata(&transAttrData);

    ParentEntitySet.Connect(this, &Placeable::RegisterActions);
}

Placeable::~Placeable()
{
    if (world_.Expired())
    {
        if (sceneNode_)
            LogError("Placeable: World has expired, skipping uninitialization!");
        return;
    }

    if (sceneNode_)
    {
        // Emit signal now so that children can detach themselves back to scene root
        AboutToBeDestroyed.Emit();

        DetachNode();
        sceneNode_->Remove();
        sceneNode_.Reset();
    }
    else
        AboutToBeDestroyed.Emit();
}

void Placeable::SetPosition(float x, float y, float z)
{
    Transform newtrans = transform.Get();
    newtrans.SetPos(x, y, z); // SetPos contains assume(IsFinite()) checks for all values.
    transform.Set(newtrans, AttributeChange::Default);
}

void Placeable::SetPosition(const float3 &pos)
{
    SetPosition(pos.x, pos.y, pos.z);
}

void Placeable::SetOrientation(const Quat &q)
{
    assume(q.IsNormalized());
    Transform newtrans = transform.Get();
    newtrans.SetOrientation(q);
    transform.Set(newtrans, AttributeChange::Default);
}

void Placeable::SetScale(float x, float y, float z)
{
    SetScale(float3(x,y,z));
}

void Placeable::SetScale(const float3 &scale)
{
    Transform newtrans = transform.Get();
    newtrans.SetScale(scale);
    transform.Set(newtrans, AttributeChange::Default);
}

void Placeable::SetOrientationAndScale(const float3x3 &tm)
{
    assume(tm.IsColOrthogonal());
    assume(!tm.HasNegativeScale());
    Transform newtrans = transform.Get();
    newtrans.SetRotationAndScale(tm);
    transform.Set(newtrans, AttributeChange::Default);
}

void Placeable::SetOrientationAndScale(const Quat &q, const float3 &scale)
{
    SetOrientationAndScale(q.ToFloat3x3() * float3x3::Scale(scale));
}

void Placeable::SetTransform(const float3x3 &tm, const float3 &pos)
{
    SetTransform(float3x4(tm, pos));
}

void Placeable::SetTransform(const float3x4 &tm)
{
    assume(tm.IsColOrthogonal());
    assume(!tm.HasNegativeScale());
    transform.Set(Transform(tm), AttributeChange::Default);
}

void Placeable::SetTransform(const Quat &orientation, const float3 &pos)
{
    SetTransform(float3x4(orientation, pos));
}

void Placeable::SetTransform(const Quat &orientation, const float3 &pos, const float3 &scale)
{
    SetTransform(float3x4::FromTRS(pos, orientation, scale));
}

void Placeable::SetTransform(const float4x4 &tm)
{
    assume(tm.Row(3).Equals(float4(0,0,0,1)));
    SetTransform(tm.Float3x4Part());
}

void Placeable::SetWorldTransform(const float3x3 &tm, const float3 &pos)
{
    SetWorldTransform(float3x4(tm, pos));
}

void Placeable::SetWorldTransform(const float3x4 &tm)
{
    assume(tm.IsColOrthogonal());
    assume(!tm.HasNegativeScale());
    if (!parentPlaceable_) // No parent, the local->parent transform equals the local->world transform.
    {
        SetTransform(tm);
        return;
    }

    float3x4 parentWorldTransform = parentPlaceable_->LocalToWorld();

    bool success = parentWorldTransform.Inverse();
    if (!success)
    {
        if (parentEntity)
            LogError("Parent for entity " + parentEntity->ToString() + " has an invalid world transform!");
        else
            LogError("Parent for a detached entity has an invalid world transform!");
        return;
    }
        
    SetTransform(parentWorldTransform * tm);
}

void Placeable::SetWorldTransform(const float4x4 &tm)
{
    assume(tm.Row(3).Equals(float4(0,0,0,1)));
    SetWorldTransform(tm.Float3x4Part());
}

void Placeable::SetWorldTransform(const Quat &orientation, const float3 &pos)
{
    assume(orientation.IsNormalized());
    SetWorldTransform(float3x4(orientation, pos));
}

void Placeable::SetWorldTransform(const Quat &orientation, const float3 &pos, const float3 &scale)
{
    assume(orientation.IsNormalized());
    SetWorldTransform(float3x4::FromTRS(pos, orientation, scale));
}

float3 Placeable::WorldPosition() const
{
    return LocalToWorld().TranslatePart();
}

Quat Placeable::WorldOrientation() const
{
    float3 translate;
    Quat rotate;
    float3 scale;
    LocalToWorld().Decompose(translate, rotate, scale);
    return rotate;
}

float3 Placeable::WorldScale() const
{
    float3 translate;
    Quat rotate;
    float3 scale;
    LocalToWorld().Decompose(translate, rotate, scale);
    return scale;
}

float3 Placeable::Position() const
{
    return transform.Get().pos;
}

Quat Placeable::Orientation() const
{
    return transform.Get().Orientation();
}

float3 Placeable::Scale() const
{
    return transform.Get().scale;
}

float3x4 Placeable::LocalToWorld() const
{
    // The Urho3D scene node world transform is always up to date with the transform attribute
    if (sceneNode_)
        return sceneNode_->GetWorldTransform();
    
    // Otherwise, if scene node is not available, use theoretical derived transform from Tundra scene structure
    Placeable *parentPlaceable = ParentPlaceableComponent();
    assert(parentPlaceable != this);
    float3x4 localToWorld = parentPlaceable ? (parentPlaceable->LocalToWorld() * LocalToParent()) : LocalToParent();

    return localToWorld;
}

float3x4 Placeable::WorldToLocal() const
{
    float3x4 tm = LocalToWorld();
#ifndef MATH_SILENT_ASSUME
    bool success = 
#endif
    tm.Inverse();
#ifndef MATH_SILENT_ASSUME
    assume(success);
#endif
    return tm;
}

float3x4 Placeable::LocalToParent() const
{
    return transform.Get().ToFloat3x4();
}

float3x4 Placeable::ParentToLocal() const
{
    float3x4 tm = transform.Get().ToFloat3x4();
#ifndef MATH_SILENT_ASSUME
    bool success = 
#endif
    tm.Inverse();
#ifndef MATH_SILENT_ASSUME
    assume(success);
#endif
    return tm;
}

Entity *Placeable::ParentPlaceableEntity() const
{
    return parentRef.Get().Lookup(ParentScene()).Get();
}

Placeable *Placeable::ParentPlaceableComponent() const
{
    return parentPlaceable_;
}

bool Placeable::IsGrandparentOf(Entity *entity) const
{
    if (!entity)
    {
        LogError("Placeable::IsGrandParentOf: called with null pointer.");
        return false;
    }
    if (!ParentEntity())
        return false;
    if (entity == ParentEntity())
        return true;

    EntityVector allChildren = Grandchildren(ParentEntity());
    EntityVector::ConstIterator iter = allChildren.Find(EntityPtr(entity));
    return iter != allChildren.End();
}

bool Placeable::IsGrandparentOf(Placeable *placeable) const
{
    assert(placeable->ParentEntity());
    return IsGrandparentOf(placeable->ParentEntity());
}

bool Placeable::IsGrandchildOf(Entity *entity) const
{
    if (!entity)
    {
        LogError("Placeable::IsGrandChildOf: called with null pointer.");
        return false;
    }
    if (!ParentEntity())
        return false;
    Entity *parentPlaceableEntity = ParentPlaceableEntity();
    if (!parentPlaceableEntity)
        return false;
    if (entity == ParentEntity())
        return true;

    EntityVector allChildren = Grandchildren(ParentEntity());
    EntityVector::ConstIterator iter = allChildren.Find(EntityPtr(ParentEntity()));
    return iter != allChildren.End();
}

bool Placeable::IsGrandchildOf(Placeable *placeable) const
{
    assert(placeable->ParentEntity());
    return IsGrandchildOf(placeable->ParentEntity());
}

EntityVector Placeable::Grandchildren(Entity *entity) const
{
    EntityVector ret;
    if (!entity)
        return ret;
    if (!entity->Component<Placeable>())
        return ret;
    EntityVector children = entity->Component<Placeable>()->Children();

    foreach(const EntityPtr &e, children)
    {
        EntityVector grandchildren = Grandchildren(e.Get());
        ret.Push(grandchildren);
    }
    return ret;
}

EntityVector Placeable::Children() const
{
    EntityVector children;
    if (!ParentEntity() || !ParentEntity()->ParentScene())
        return children;

    for (u32 i=0, len=childPlaceables_.Size(); i<len; ++i)
    {
        const ComponentWeakPtr &weak = childPlaceables_[i];
        if (!weak.Expired())
            children.Push(EntityPtr(weak->ParentEntity()));
    }
    return children;
}

void Placeable::AttributesChanged()
{
    if (!sceneNode_)
        return; // we're not initialized properly, do not react to attribute changes internally.

    // If parent ref or parent bone changed, reattach node to scene hierarchy
    if (parentRef.ValueChanged() || parentBone.ValueChanged())
        AttachNode();
    
    if (transform.ValueChanged())
    {
        transform.ClearChangedFlag();

        const Transform& trans = transform.Get();
        if (trans.pos.IsFinite())
            sceneNode_->SetPosition(trans.pos);

        Quat orientation = trans.Orientation();
        if (orientation.IsFinite())
            sceneNode_->SetRotation(orientation);
        else
            LogError("Placeable: transform attribute changed, but orientation not valid!");

        sceneNode_->SetScale(trans.scale);

        TransformChanged.Emit();
    }

    /// \todo Implement DrawDebug

    if (visible.ValueChanged())
    {
        if (visible.Get())
            Show();
        else
            Hide();
    }
}

void Placeable::AttachNode()
{
    if (!sceneNode_)
        return;
    
    if (world_.Expired())
    {
        LogError("Placeable::AttachNode: No GraphicsWorld available to call this function!");
        return;
    }
    GraphicsWorldPtr world = world_.Lock();
    // Scene root node is same as the Urho scene itself
    Urho3D::Scene* root_node = world->UrhoScene();

    PROFILE(Placeable_AttachNode);

    // If already attached, detach first
    if (attached_)
        DetachNode();
        
    // Three possible cases
    // 1) attach to scene root node
    // 2) attach to another Placeable's scene node
    // 3) attach to a bone on a skeletal mesh
    // Disconnect from the EntityCreated & ComponentAdded signals, as responding to them might not be needed anymore.
    // We will reconnect signals as necessary
    Entity* ownEntity = ParentEntity();
    Scene* scene = ownEntity ? ownEntity->ParentScene() : 0;
    if (scene)
        scene->EntityCreated.Disconnect(this, &Placeable::CheckParentEntityCreated);
    if (ownEntity)
        ownEntity->ComponentAdded.Disconnect(this, &Placeable::OnComponentAdded);

    // Try to attach to another entity if the parent ref is non-empty
    // Make sure we're not trying to attach to ourselves as the parent
    const EntityReference& parent = parentRef.Get();
    if (!parent.IsEmpty() || (ownEntity && ownEntity->Parent()))
    {
        if (!ownEntity || !scene)
            return;
        
        Entity* parentEntity = parent.LookupParent(ownEntity).Get();
        if (parentEntity == ownEntity)
        {
            // If we refer to self, attach to the root
            sceneNode_->SetParent(root_node);
            attached_ = true;
            return;
        }
        if (parentEntity)
        {
            parentEntity->ComponentAdded.Disconnect(this, &Placeable::OnComponentAdded);

            String boneName = parentBone.Get();
            if (!boneName.Empty())
            {
                Mesh* parentMesh = parentEntity->Component<Mesh>().Get();
                if (parentMesh)
                {
                    Urho3D::Node* bone = parentMesh->BoneNode(boneName);

                    if (bone)
                    {
                        sceneNode_->SetParent(bone);
                        parentMesh_ = parentMesh;
                        parentMesh->MeshAboutToBeDestroyed.Connect(this, &Placeable::OnParentMeshDestroyed);

                        // Connect also to parent placeable
                        parentPlaceable_ = parentEntity->Component<Placeable>().Get();
                        if (parentPlaceable_)
                        {
                            parentPlaceable_->ChildAttached(ComponentPtr(this));
                            // Connect to destruction of the placeable to be able to detach gracefully
                            parentPlaceable_->AboutToBeDestroyed.Connect(this, &Placeable::OnParentPlaceableDestroyed);
                        }

                        attached_ = true;
                        return;
                    }
                    else
                    {
                        // Could not find the bone. Connect to the parent mesh MeshChanged signal to wait for the proper mesh to be assigned.
                        if (ViewEnabled())
                            LogWarning("Placeable::AttachNode: Could not find bone " + boneName + " to attach to, attaching to the parent scene node instead.");
                        parentMesh->MeshChanged.Connect(this, &Placeable::OnParentMeshChanged);
                        // While we wait, fall through to attaching to the scene node instead
                    }
                }
                else
                {
                    // If can't find the mesh component yet, wait for it to be created
                    parentEntity->ComponentAdded.Connect(this, &Placeable::OnComponentAdded);
                    return;
                }
            }
            
            Placeable* parentPlaceable = parentEntity->Component<Placeable>().Get();
            if (parentPlaceable)
            {
                // If we have a cyclic parenting attempt, attach to the root instead
                Placeable* parentCheck = parentPlaceable;
                while (parentCheck)
                {
                    if (parentCheck == this)
                    {
                        LogWarning("Placeable::AttachNode: Cyclic scene node parenting attempt detected! Parenting to the scene root node instead.");
                        sceneNode_->SetParent(root_node);
                        attached_ = true;
                        return;
                    }
                    parentCheck = parentCheck->parentPlaceable_;
                }

                parentPlaceable_ = parentPlaceable;
                sceneNode_->SetParent(parentPlaceable_->UrhoSceneNode());
                parentPlaceable_->ChildAttached(ComponentPtr(this));
                
                // Connect to destruction of the placeable to be able to detach gracefully
                parentPlaceable_->AboutToBeDestroyed.Connect(this, &Placeable::OnParentPlaceableDestroyed);
                attached_ = true;
                return;
            }
            else
            {
                // If can't find the placeable component yet, wait for it to be created
                parentEntity->ComponentAdded.Connect(this, &Placeable::OnComponentAdded);
                return;
            }
        }
        else
        {
            // Could not find parent entity. Check for it later, when new entities are created into the scene
            scene->EntityCreated.Connect(this, &Placeable::CheckParentEntityCreated);
            return;
        }
    }
        
    sceneNode_->SetParent(root_node);
    attached_ = true;
}

void Placeable::DetachNode()
{
    if (!sceneNode_)
        return;

    if (world_.Expired())
    {
        LogError("Placeable::DetachNode: No GraphicsWorld available to call this function!");
        return;
    }

    GraphicsWorldPtr world = world_.Lock();
    
    if (!attached_)
        return;
    
    // Three possible cases
    // 1) attached to scene root node
    // 2) attached to another scene node
    // 3) attached to a bone via manual tracking
    if (parentMesh_)
    {
        parentMesh_->MeshAboutToBeDestroyed.Disconnect(this, &Placeable::OnParentMeshDestroyed);
        parentMesh_.Reset();
    }
    if (parentPlaceable_)
    {
        parentPlaceable_->AboutToBeDestroyed.Disconnect(this, &Placeable::OnParentPlaceableDestroyed);
        parentPlaceable_->ChildDetached(this);
        parentPlaceable_.Reset();
    }

    /// \todo Cannot actually detach from scene as that would cause destruction of the scene node, just move to scene root
    sceneNode_->SetParent(world->UrhoScene());

    attached_ = false;
}

void Placeable::CleanExpiredChildren()
{
    for (u32 i=0; i<childPlaceables_.Size(); ++i)
    {
        ComponentWeakPtr &weak = childPlaceables_[i];
        if (weak.Expired())
        {
            weak.Reset();
            childPlaceables_.Erase(i);
            i--;
        }
    }
}

bool Placeable::HasAttachedChild(IComponent *placeable)
{
    for (u32 i=0, len=childPlaceables_.Size(); i<len; ++i)
    {
        const ComponentWeakPtr &weak = childPlaceables_[i];
        if (!weak.Expired() && weak.Get() == placeable)
            return true;
    }
    return false;
}

void Placeable::ChildAttached(ComponentPtr placeable)
{
    if (!placeable.Get() || HasAttachedChild(placeable.Get()))
        return;

    CleanExpiredChildren();
    childPlaceables_.Push(ComponentWeakPtr(placeable));
}

void Placeable::ChildDetached(IComponent *placeable)
{
    if (placeable)
    {
        for (u32 i=0, len=childPlaceables_.Size(); i<len; ++i)
        {
            ComponentWeakPtr &weak = childPlaceables_[i];
            if (!weak.Expired() && weak.Get() == placeable)
            {
                weak.Reset();
                childPlaceables_.Erase(i);
                break;
            }
        }
    }
    CleanExpiredChildren();
}

void Placeable::OnParentMeshDestroyed()
{
    MeshWeakPtr meshWeak(parentMesh_);
    DetachNode();
    if (meshWeak)
    {
        // Connect to the mesh component setting a new mesh; we might (re)find the proper bone then
        meshWeak->MeshChanged.Connect(this, &Placeable::OnParentMeshChanged);
    }
}

void Placeable::OnParentPlaceableDestroyed()
{
    DetachNode();
}

void Placeable::CheckParentEntityCreated(Entity* entity, AttributeChange::Type)
{
    if (!attached_ && entity)
    {
        PROFILE(Placeable_CheckParentEntityCreated);
        /** @note EntityReference::Lookup was replaced here by Matches that does
            not iterate the whole scene (worst case scenario, parented by name).
            This freezes Tundra for quite badly if there are thousands of Entities
            and the parent has a late id (as in arrives ~last from network). */
        if (parentRef.Get().Matches(entity))
            AttachNode();
    }
}

void Placeable::OnParentMeshChanged()
{
    if (!attached_ || !parentBone.Get().Trimmed().Empty())
        AttachNode();
}

void Placeable::OnComponentAdded(IComponent* component, AttributeChange::Type)
{
    if (!attached_ && component && (component->TypeId() == Placeable::TypeIdStatic() || component->TypeId() == Mesh::TypeIdStatic()))
        AttachNode();
}

void Placeable::OnEntityParentChanged(Entity*, Entity*, AttributeChange::Type)
{
    // The entity's parent change matters only when the parent ref is empty (ie. we are not overriding the parent)
    if (parentRef.Get().IsEmpty())
        AttachNode();
}

void Placeable::RegisterActions()
{
    Entity *entity = ParentEntity();
    assert(entity);
    if (entity && entity->ParentScene())
    {
        // Get the GraphicsWorld pointer (if applicable) and create scenenode now
        world_ = entity->ParentScene()->Subsystem<GraphicsWorld>();
        GraphicsWorldPtr world = world_.Lock();
        if (world)
        {
            sceneNode_ = world->UrhoScene()->CreateChild();
            AttachNode();
        }

        // Generic actions
        /// \todo Can not connect the parameter-less Show(), Hide() functions directly
        entity->Action("ShowEntity")->Triggered.Connect(this, &Placeable::OnShowTriggered);
        entity->Action("HideEntity")->Triggered.Connect(this, &Placeable::OnHideTriggered);
        entity->Action("ToggleEntity")->Triggered.Connect(this, &Placeable::OnToggleVisibilityTriggered);

        // Connect to entity reparenting to switch the parent in the graphical scene
        entity->ParentChanged.Connect(this, &Placeable::OnEntityParentChanged);
    }
}

void Placeable::Show()
{
    if (sceneNode_)
    {
        /* Show with recurse, but hide child placeables that
           have visible attribute as false. Recurse is left here
           because there might be billboards, manual
           objects etc. attached to this node manually. */
        sceneNode_->SetEnabledRecursive(true);

        EntityVector childEnts = Children();
        for (EntityVector::Iterator iter = childEnts.Begin(); iter != childEnts.End(); ++iter)
        {
            EntityPtr childEnt = (*iter);
            if (childEnt.Get())
            {
                PlaceablePtr childPlaceable = childEnt->Component<Placeable>();
                if (childPlaceable.Get() && !childPlaceable->visible.Get())
                    childPlaceable->Hide();
            }
        }
    }
}

void Placeable::Hide()
{
    if (sceneNode_)
    {
        /* Hiding always recurses. All children no matter their visible
           attribute should be hidden if the parent hides. */
        sceneNode_->SetEnabledRecursive(false);
    }
}

void Placeable::ToggleVisibility()
{
    if (sceneNode_)
    {
        if (sceneNode_->IsEnabled())
            Hide();
        else
            Show();
    }
}

void Placeable::OnShowTriggered(const StringVector&)
{
    Show();
}

void Placeable::OnHideTriggered(const StringVector&)
{
    Hide();
}

void Placeable::OnToggleVisibilityTriggered(const StringVector&)
{
    ToggleVisibility();
}

}
