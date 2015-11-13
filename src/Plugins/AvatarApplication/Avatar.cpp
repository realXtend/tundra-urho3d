// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "Avatar.h"
#include "Mesh.h"
#include "Placeable.h"
#include "AssetAPI.h"
#include "IAssetTransfer.h"
#include "AssetRefListener.h"
#include "AvatarDescAsset.h"
#include "Entity.h"
#include "Scene/Scene.h"
#include "Framework.h"
#include "LoggingFunctions.h"

#include <Urho3D/Core/Profiler.h>

namespace Tundra
{

Avatar::Avatar(Urho3D::Context* context, Scene* scene) :
    IComponent(context, scene),
    INIT_ATTRIBUTE_VALUE(appearanceRef, "Appearance ref", AssetReference("", "Avatar"))
{
    avatarAssetListener_ = new AssetRefListener();
    avatarAssetListener_->Loaded.Connect(this, &Avatar::OnAvatarAppearanceLoaded);
    avatarAssetListener_->TransferFailed.Connect(this, &Avatar::OnAvatarAppearanceFailed);
}

Avatar::~Avatar()
{
}

void Avatar::OnAvatarAppearanceFailed(IAssetTransfer* /*transfer*/, String reason)
{
    LogError("OnAvatarAppearanceFailed, reason " + reason);
}

void Avatar::OnAvatarAppearanceLoaded(AssetPtr asset)
{
    if (!asset)
        return;

    Entity* entity = ParentEntity();
    if (!entity)
        return;

    AvatarDescAssetPtr avatarAsset = Urho3D::DynamicCast<AvatarDescAsset>(asset);
    if (!avatarAsset)
        return;
        
    // Disconnect old change signals, connect new
    AvatarDescAsset* oldDesc = avatarAsset_.Get();
    AvatarDescAsset* newDesc = avatarAsset.Get();
    if (oldDesc != newDesc)
    {
        if (oldDesc)
        {
            oldDesc->AppearanceChanged.Disconnect(this, &Avatar::SetupAppearance);
            oldDesc->DynamicAppearanceChanged.Disconnect(this, &Avatar::SetupDynamicAppearance);
        }
        newDesc->AppearanceChanged.Connect(this, &Avatar::SetupAppearance);
        newDesc->DynamicAppearanceChanged.Connect(this, &Avatar::SetupDynamicAppearance);
    }
    
    avatarAsset_ = avatarAsset;
    
    // Create components the avatar needs, with network sync disabled, if they don't exist yet
    // Note: the mesh is created non-syncable on purpose, as each client's Avatar component should execute this code upon receiving the appearance
    MeshPtr mesh = entity->GetOrCreateComponent<Mesh>("", AttributeChange::LocalOnly, false);
    if (mesh->IsReplicated())
        LogWarning("Warning! Mesh component is replicated and may not work as intended for the Avatar component");

    SetupAppearance();
}

void Avatar::AttributesChanged()
{
    if (appearanceRef.ValueChanged())
    {
        String ref = appearanceRef.Get().ref.Trimmed();
        if (ref.Empty())
            return;
        
        avatarAssetListener_->HandleAssetRefChange(&appearanceRef, "Avatar");
    }
}

void Avatar::SetupAppearance()
{
    URHO3D_PROFILE(Avatar_SetupAppearance);
    
    Entity* entity = ParentEntity();
    AvatarDescAssetPtr desc = AvatarDesc();
    if (!desc || !entity)
        return;
    
    Mesh* mesh = entity->Component<Mesh>().Get();
    if (!mesh)
    {
        LogWarning("Avatar::SetupAppearance: No Mesh component in entity, can not setup appearance");
        return;
    }

    // If mesh ref is empty, no need to go further
    if (!desc->mesh_.Length())
    {
        LogWarning("Avatar::SetupAppearance: AvatarDescAsset contains empty mesh ref, can not setup appearance");
        return;
    }
    
    SetupMeshAndMaterials();
    SetupDynamicAppearance();
    SetupAttachments();
}

void Avatar::SetupDynamicAppearance()
{
    /// \todo Implement if possible and needed
}

AvatarDescAssetPtr Avatar::AvatarDesc() const
{
    return avatarAsset_.Lock();
}

String Avatar::AvatarProperty(const String& name) const
{
    AvatarDescAssetPtr desc = AvatarDesc();
    if (!desc)
        return String();
    else
        return desc->properties_[name];
}

void Avatar::AdjustHeightOffset()
{
    Entity* entity = ParentEntity();
    AvatarDescAssetPtr desc = AvatarDesc();
    if (!desc || !entity)
        return;
    Mesh* mesh = entity->Component<Mesh>().Get();
    if (!mesh)
        return;
    
    float3 offset = float3::zero;

    if (desc->HasProperty("baseoffset"))
    {
        offset = float3::FromString(desc->GetProperty("baseoffset").CString());
    }
    
    /// \todo Implement fully
    Transform t = mesh->nodeTransformation.Get();
    t.pos = offset - float3(0.0f, desc->height_ * 0.5f, 0.0f);
    mesh->nodeTransformation.Set(t, AttributeChange::LocalOnly);
}

void Avatar::SetupMeshAndMaterials()
{
    Entity* entity = ParentEntity();
    AvatarDescAssetPtr desc = AvatarDesc();
    if (!desc || !entity)
        return;
    Mesh* mesh = entity->Component<Mesh>().Get();
    if (!mesh)
        return;

    String meshName = LookupAsset(desc->mesh_);
    String skeletonName = LookupAsset(desc->skeleton_);

    mesh->skeletonRef.Set(AssetReference(skeletonName, "OgreSkeleton"), AttributeChange::LocalOnly);
    mesh->meshRef.Set(AssetReference(meshName, "OgreMesh"), AttributeChange::LocalOnly);

    AssetReferenceList materials("OgreMaterial");
    for (uint i = 0; i < desc->materials_.Size(); ++i)
        materials.Append(AssetReference(LookupAsset(desc->materials_[i]), "OgreMaterial"));
    mesh->materialRefs.Set(materials, AttributeChange::LocalOnly);
    mesh->castShadows.Set(true, AttributeChange::LocalOnly);
    AdjustHeightOffset();
}

void Avatar::SetupAttachments()
{
    Entity* entity = ParentEntity();
    AvatarDescAssetPtr desc = AvatarDesc();
    if (!desc || !entity)
        return;
    Mesh* mesh = entity->Component<Mesh>().Get();
    if (!mesh)
        return;
    
    // Remove existing attachments
    while (attachmentEntities_.Size())
    {
        entity->RemoveChild(attachmentEntities_.Back().Lock());
        attachmentEntities_.Pop();
    }
    /// \todo Support linked bone animation
    const Vector<AvatarAttachment>& attachments = desc->attachments_;
    for (uint i = 0; i < attachments.Size(); ++i)
    {
        const AvatarAttachment& attach = attachments[i];
        StringVector components;
        components.Push("Mesh");
        components.Push("Placeable");
        Entity* childEntity = entity->CreateChild(0, components, AttributeChange::LocalOnly, false, false, true);
        if (!childEntity)
        {
            LogError("Could not create attachment entity for avatar");
            continue;
        }
        childEntity->SetName("Attachment_" + attach.name_);
        attachmentEntities_.Push(EntityWeakPtr(childEntity));
        Mesh* childMesh = childEntity->Component<Mesh>();
        Placeable* childPlaceable = childEntity->Component<Placeable>();
        if (!childMesh || !childPlaceable)
        {
            LogError("Could not create Mesh & Placeable components to attachment entity");
            continue;
        }

        Transform t;
        t.pos = attach.transform_.position_;
        t.SetOrientation(attach.transform_.orientation_);
        t.scale = attach.transform_.scale_;
        childPlaceable->transform.Set(t);
        childMesh->meshRef.Set(AssetReference(LookupAsset(attach.mesh_), "OgreMesh"));
        AssetReferenceList materials("OgreMaterial");
        for (uint j = 0; j < attach.materials_.Size(); ++j)
            materials.Append(AssetReference(LookupAsset(attach.materials_[j]), "OgreMaterial"));
        childMesh->materialRefs.Set(materials);
        if (!attach.bone_name_.Trimmed().Empty())
            childPlaceable->parentBone.Set(attach.bone_name_);
    }
}

String Avatar::LookupAsset(const String& ref)
{
    String descName;
    AvatarDescAssetPtr desc = AvatarDesc();
    if (desc)
        descName = desc->Name();
    
    return framework->Asset()->ResolveAssetRef(descName, ref);
}

};

