// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "Mesh.h"
#include "Framework.h"
#include "GraphicsWorld.h"
#include "Placeable.h"
#include "Scene/Scene.h"
#include "AttributeMetadata.h"
#include "LoggingFunctions.h"
#include "AssetRefListener.h"
#include "IMeshAsset.h"
#include "IMaterialAsset.h"
#include "Ogre/OgreSkeletonAsset.h"

#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Node.h>
#include <Engine/Graphics/AnimatedModel.h>
#include <Engine/Graphics/Material.h>
#include <Engine/Resource/ResourceCache.h>

namespace Tundra
{

Mesh::Mesh(Urho3D::Context* context, Scene* scene) :
    IComponent(context, scene),
    INIT_ATTRIBUTE_VALUE(nodeTransformation, "Transform", Transform(float3(0,0,0),float3(0,0,0),float3(1,1,1))),
    INIT_ATTRIBUTE_VALUE(meshRef, "Mesh ref", AssetReference("", "OgreMesh")),
    INIT_ATTRIBUTE_VALUE(skeletonRef, "Skeleton ref", AssetReference("", "OgreSkeleton")),
    INIT_ATTRIBUTE_VALUE(materialRefs, "Material refs", AssetReferenceList("OgreMaterial")),
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
            LogError("Mesh: World has expired, skipping uninitialization!");
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
    return adjustmentNode_ ? adjustmentNode_->GetChild(name, true) : nullptr;
}

void Mesh::DeserializeFrom(Urho3D::XMLElement& element, AttributeChange::Type change)
{
    if (!BeginDeserialization(element))
        return;

    if (change == AttributeChange::Default)
        change = updateMode;
    assert(change != AttributeChange::Default);

    Urho3D::XMLElement attributeElement = element.GetChild("attribute");
    while(attributeElement)
    {
        if (attributeElement.GetAttribute("id").Empty() && attributeElement.GetAttribute("name").Compare("Mesh materials", false) == 0)
            attributeElement.SetAttribute("name", "Material refs");
        DeserializeAttributeFrom(attributeElement, change);
        attributeElement = attributeElement.GetNext("attribute");
    }
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

    meshRefListener_ = new AssetRefListener();
    skeletonRefListener_ = new AssetRefListener();
    materialRefListListener_ = new AssetRefListListener(framework->Asset());
    
    parent->ComponentAdded.Connect(this, &Mesh::OnComponentStructureChanged);
    parent->ComponentRemoved.Connect(this, &Mesh::OnComponentStructureChanged);

    if (parent->ParentScene())
        world_ = parent->ParentScene()->Subsystem<GraphicsWorld>();

    if (world_ && !mesh_)
    {
        Urho3D::Scene* urhoScene = world_->UrhoScene();
        adjustmentNode_ = urhoScene->CreateChild("AdjustmentNode");
        
        // Make the entity & component links for identifying raycasts
        adjustmentNode_->SetVar(GraphicsWorld::entityLink, Variant(WeakPtr<RefCounted>(parent)));
        adjustmentNode_->SetVar(GraphicsWorld::componentLink, Variant(WeakPtr<RefCounted>(this)));

        mesh_ = adjustmentNode_->CreateComponent<Urho3D::AnimatedModel>();

        // Connect ref listeners
        meshRefListener_->Loaded.Connect(this, &Mesh::OnMeshAssetLoaded);
        skeletonRefListener_->Loaded.Connect(this, &Mesh::OnSkeletonAssetLoaded);
        materialRefListListener_->Changed.Connect(this, &Mesh::OnMaterialAssetRefsChanged);
        materialRefListListener_->Failed.Connect(this, &Mesh::OnMaterialAssetFailed);
        materialRefListListener_->Loaded.Connect(this, &Mesh::OnMaterialAssetLoaded);
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
        LogError("Can not attach mesh: placeable does not have an Urho3D scene node");
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
        const Transform &newTransform = nodeTransformation.Get();
        adjustmentNode_->SetPosition(newTransform.pos);
        adjustmentNode_->SetRotation(newTransform.Orientation());
        adjustmentNode_->SetScale(newTransform.scale);
    }
    if (meshRef.ValueChanged() && meshRefListener_)
    {
        /// @todo Why is this warning here? Pretty normal to clear mesh ref...
        if (meshRef.Get().ref.Trimmed().Empty())
            LogDebug("Warning: Mesh \"" + this->parentEntity->Name() + "\" mesh ref was set to an empty reference!");
        meshRefListener_->HandleAssetRefChange(&meshRef);
    }
    if (skeletonRef.ValueChanged() && skeletonRefListener_)
    {
        skeletonRefListener_->HandleAssetRefChange(&skeletonRef);
    }
    if (materialRefs.ValueChanged() && materialRefListListener_)
    {
        /* Let the listener resolve and cleanup the refs, while us keeping the originals intact.
           Changes are handled in OnMaterialAssetRefsChanged/Failed/Loaded. */
        materialRefListListener_->HandleChange(materialRefs.Get());
    }
    if (skeletonRef.ValueChanged())
    {
        /// \todo Implement
    }
}

void Mesh::ApplyMesh()
{
    assert(mesh_);

    IMeshAsset* mAsset = dynamic_cast<IMeshAsset*>(meshRefListener_->Asset().Get());
    OgreSkeletonAsset* sAsset = dynamic_cast<OgreSkeletonAsset*>(skeletonRefListener_->Asset().Get());

    if (!mAsset)
        return;

    if (mesh_->GetModel())
    {
        // Signal destruction of the old model. Eg. bone attachments need to be removed now
        MeshAboutToBeDestroyed.Emit();
    }

    // Clear existing skeletal model if any
    skeletalModel.Reset();

    Urho3D::Model* baseModel = mAsset->UrhoModel();
    // If a skeleton asset not defined, use the mesh asset as is
    if (!sAsset)
    {
        mesh_->SetModel(baseModel);
        MeshChanged.Emit();
        return;
    }

    // If a skeleton asset is defined, clone the model and add the bones from the skeleton
    // We don't call Model::Clone() directly, as that would deep copy the vertex data, which we do not want
    skeletalModel = new Urho3D::Model(context_);
    skeletalModel->SetNumGeometries(baseModel->GetNumGeometries());
    for (uint i = 0; i < baseModel->GetNumGeometries(); ++i)
        for (uint j = 0; j < baseModel->GetNumGeometryLodLevels(i); ++j)
            skeletalModel->SetGeometry(i, j, baseModel->GetGeometry(i, j));
    /// \todo Set bone bounds
    skeletalModel->SetSkeleton(sAsset->UrhoSkeleton());
    skeletalModel->SetGeometryBoneMappings(baseModel->GetGeometryBoneMappings());
    skeletalModel->SetBoundingBox(baseModel->GetBoundingBox());

    mesh_->SetModel(skeletalModel);

    MeshChanged.Emit();
    SkeletonChanged.Emit();
}

void Mesh::OnMeshAssetLoaded(AssetPtr asset)
{
    IMeshAsset* mAsset = dynamic_cast<IMeshAsset*>(asset.Get());
    if (!mAsset)
    {
        LogErrorF("Mesh: Mesh asset load finished for '%s', but downloaded asset was not of type IMeshAsset!", asset->Name().CString());
        return;
    }

    if (mesh_)
    {
        ApplyMesh();
        // Apply default material first to every submesh to match Tundra behavior
        Urho3D::ResourceCache* cache = GetSubsystem<Urho3D::ResourceCache>();
        for (uint gi=0; gi<mesh_->GetNumGeometries(); ++gi)
            mesh_->SetMaterial(gi, cache->GetResource<Urho3D::Material>("Materials/DefaultGrey.xml"));

        // Apply all materials that have been loaded so far.
        // OnMaterialAssetLoaded will do the right thing once model has been set.
        Vector<AssetPtr> materialAssets = materialRefListListener_->Assets();
        for(uint mi=0; mi<materialAssets.Size(); ++mi)
        {
            AssetPtr &materialAssetPtr = materialAssets[mi];
            IMaterialAsset *materialAsset = dynamic_cast<IMaterialAsset*>(materialAssetPtr.Get());
            if (materialAsset && materialAsset->IsLoaded())
            {
                if (mi < mesh_->GetNumGeometries())
                {
                    mesh_->SetMaterial(mi, materialAsset->UrhoMaterial());
                    MaterialChanged.Emit(mi, materialAsset->Name());
                }
                else
                    LogWarningF("Mesh: Illegal submesh index %d for material %s. Target mesh %s has %d submeshes.", mi, materialAsset->Name().CString(), meshRef.Get().ref.CString(), mesh_->GetNumGeometries());
            }
        }
    }
    else
        LogWarningF("Mesh: Model asset loaded but target mesh has not been created yet in %s", ParentEntity()->ToString().CString());
}

void Mesh::OnSkeletonAssetLoaded(AssetPtr asset)
{
    OgreSkeletonAsset* sAsset = dynamic_cast<OgreSkeletonAsset*>(asset.Get());
    if (!sAsset)
    {
        LogErrorF("Mesh: Skeleton asset load finished for '%s', but downloaded asset was not of type IMeshAsset!", asset->Name().CString());
        return;
    }
    if (mesh_)
        ApplyMesh();
}

void Mesh::OnMaterialAssetRefsChanged(const AssetReferenceList &mRefs)
{
    if (!mesh_ || !mesh_->GetModel())
        return;

    Urho3D::ResourceCache* cache = GetSubsystem<Urho3D::ResourceCache>();

    for (uint gi=0; gi<mesh_->GetNumGeometries(); ++gi)
    {
        if (gi >= (uint)mRefs.Size() || mRefs.refs[gi].ref.Empty())
        {
            mesh_->SetMaterial(gi, cache->GetResource<Urho3D::Material>("Materials/DefaultGrey.xml"));
        }
    }
}

void Mesh::OnMaterialAssetFailed(uint index, IAssetTransfer* /*transfer*/, String /*error*/)
{
    Urho3D::ResourceCache* cache = GetSubsystem<Urho3D::ResourceCache>();

    // Don't log an warning on load failure if index is out of submesh range.
    if (mesh_ && mesh_->GetModel() && index < mesh_->GetNumGeometries())
        mesh_->SetMaterial(index, cache->GetResource<Urho3D::Material>("Materials/AssetLoadError.xml"));
}

void Mesh::OnMaterialAssetLoaded(uint index, AssetPtr asset)
{
    IMaterialAsset* mAsset = dynamic_cast<IMaterialAsset*>(asset.Get());
    if (!mAsset)
    {
        LogErrorF("Mesh: Material asset load finished for '%s', but downloaded asset was not of type IMaterialAsset!", asset->Name().CString());
        return;
    }

    if (mesh_ && mesh_->GetModel())
    {
        if (index < mesh_->GetNumGeometries())
        {
            mesh_->SetMaterial(index, mAsset->UrhoMaterial());
            MaterialChanged.Emit(index, mAsset->Name());
        }
        else
            LogWarningF("Mesh: Illegal submesh index %d for material %s. Target mesh %s has %d submeshes.", index, mAsset->Name().CString(), meshRef.Get().ref.CString(), mesh_->GetNumGeometries());
    }
}

}
