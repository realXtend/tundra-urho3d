// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "Mesh.h"
#include "Placeable.h"
#include "Scene/Scene.h"
#include "GraphicsWorld.h"
#include "AttributeMetadata.h"
#include "Math/MathUtilities.h"

#include <Log.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Node.h>
#include <Engine/Graphics/AnimatedModel.h>
#include <Engine/Resource/ResourceCache.h>

namespace Tundra
{

Mesh::Mesh(Urho3D::Context* context, Scene* scene) :
    IComponent(context, scene),
    INIT_ATTRIBUTE_VALUE(nodeTransformation, "Transform", Transform(float3(0,0,0),float3(0,0,0),float3(1,1,1))),
    INIT_ATTRIBUTE_VALUE(meshRef, "Mesh ref", AssetReference("", "OgreMesh")),
    INIT_ATTRIBUTE_VALUE(skeletonRef, "Skeleton ref", AssetReference("", "OgreSkeleton")),
    INIT_ATTRIBUTE_VALUE(materialRefs, "Mesh materials", AssetReferenceList("OgreMaterial")), /**< @todo 24.10.2013 Rename name to "Material refs" or similar. */
    INIT_ATTRIBUTE_VALUE(drawDistance, "Draw distance", 0.0f),
    INIT_ATTRIBUTE_VALUE(castShadows, "Cast shadows", false),
    INIT_ATTRIBUTE_VALUE(useInstancing, "Use instancing", false)
{
    if (scene)
        world_ = scene->Subsystem<GraphicsWorld>();

    static AttributeMetadata drawDistanceData("", "0", "10000");
    drawDistance.SetMetadata(&drawDistanceData);

    static AttributeMetadata materialMetadata;
    materialMetadata.elementType = "AssetReference";
    materialRefs.SetMetadata(&materialMetadata);

    ParentEntitySet.Connect(this, &Mesh::UpdateSignals);
}

Mesh::~Mesh()
{
    if (world_.Expired())
    {
        if (mesh_)
            LOGERROR("Mesh: World has expired, skipping uninitialization!");
        return;
    }

    if (mesh_)
    {
        MeshAboutToBeDestroyed.Emit();
        
        mesh_.Reset();
        // The mesh component will be destroyed along with the adjustment node
        adjustmentNode_->Remove();
        adjustmentNode_.Reset();
    }
}

Urho3D::Node* Mesh::BoneNode(const String& name) const
{
    // When a skeletal mesh is created, the bone hierarchy will be under the adjustment node
    return adjustmentNode_ ? adjustmentNode_->GetChild(name, true) : (Urho3D::Node*)0;
}

Urho3D::AnimatedModel* Mesh::UrhoMesh() const
{
    return mesh_;
}

void Mesh::UpdateSignals()
{
    Entity* parent = ParentEntity();
    if (!parent)
        return;

    // If scene is not view-enabled, no further action
    if (!ViewEnabled())
        return;
    
    parent->ComponentAdded.Connect(this, &Mesh::OnComponentStructureChanged);
    parent->ComponentRemoved.Connect(this, &Mesh::OnComponentStructureChanged);

    if (parent->ParentScene())
        world_ = parent->ParentScene()->Subsystem<GraphicsWorld>();

    if (world_ && !mesh_)
    {
        Urho3D::Scene* urhoScene = world_->UrhoScene();
        adjustmentNode_ = urhoScene->CreateChild("AdjustmentNode");
        mesh_ = adjustmentNode_->CreateComponent<Urho3D::AnimatedModel>();

        // Until we have proper asset support, just render something
        mesh_->SetModel(GetSubsystem<Urho3D::ResourceCache>()->GetResource<Urho3D::Model>("Models/Box.mdl"));
    }

    // Make sure we attach to the Placeable if exists.
    AttachMesh();
}

void Mesh::DetachMesh()
{
    if (!mesh_ || world_.Expired())
        return;

    if (placeable_)
    {
        Urho3D::Scene* urhoScene = world_->UrhoScene();
        // When removed from the placeable, attach to scene root to avoid being removed from scene
        adjustmentNode_->SetParent(urhoScene);
        placeable_.Reset();
        mesh_->SetEnabled(false); // We should not render while detached
    }
}

void Mesh::AttachMesh()
{
    if (!mesh_ || world_.Expired())
        return;

    // Detach first, in case the original placeable no longer exists
    DetachMesh();

    Entity *entity = ParentEntity();
    if (!entity)
        return;
    placeable_ = entity->Component<Placeable>();
    if (!placeable_)
        return;

    Urho3D::Node* placeableNode = placeable_->UrhoSceneNode();
    if (!placeableNode)
    {
        LOGERROR("Can not attach mesh: placeable does not have an Urho3D scene node");
        return;
    }
    adjustmentNode_->SetParent(placeableNode);
    mesh_->SetEnabled(true);
}

void Mesh::OnComponentStructureChanged(IComponent*, AttributeChange::Type)
{
    // No-op if attached to the same placeable already
    if (placeable_ == parentEntity->Component<Placeable>())
        return;

    AttachMesh();
}

void Mesh::AttributesChanged()
{
    // None of the attributes have an effect when the scene is not viewenabled and there is no actual mesh
    if (!mesh_)
        return;

    if (drawDistance.ValueChanged())
        mesh_->SetDrawDistance(drawDistance.Get());
    if (castShadows.ValueChanged())
        mesh_->SetCastShadows(castShadows.Get());
    if (nodeTransformation.ValueChanged())
    {
        Transform newTransform = nodeTransformation.Get();
        adjustmentNode_->SetPosition(ToVector3(newTransform.pos));
        adjustmentNode_->SetRotation(ToQuaternion(newTransform.Orientation()));
        adjustmentNode_->SetScale(ToVector3(newTransform.scale));
    }
    if (meshRef.ValueChanged())
    {
        /// \todo Implement
    }
    if (materialRefs.ValueChanged())
    {
        /// \todo Implement
    }
    if (skeletonRef.ValueChanged())
    {
        /// \todo Implement
    }
}

}
