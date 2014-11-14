// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "Sky.h"
#include "GraphicsWorld.h"
#include "TextureAsset.h"
#include "IMaterialAsset.h"
#include "AttributeMetadata.h"
#include "AssetAPI.h"
#include "Framework.h"
#include "Scene/Scene.h"
#include "LoggingFunctions.h"
#include "IAssetTransfer.h"


#include <Engine/Scene/Scene.h>
#include <Node.h>
#include <Skybox.h>
#include <Model.h>
#include <Material.h>
#include <Technique.h>
#include <ResourceCache.h>
#include <Texture2D.h>
#include <TextureCube.h>
#include <MemoryBuffer.h>
#include <Image.h>
#include <Graphics.h>
#include <Technique.h>
#include <GraphicsDefs.h>



namespace
{

/// \todo default skybox textures not used currently
const unsigned int cSkyBoxTextureCount = 6;
const char * const cDefaultSkyBoxTextures[cSkyBoxTextureCount] =
{
    "sky_front.dds",
    "sky_back.dds",
    "sky_left.dds",
    "sky_right.dds",
    "sky_top.dds",
    "sky_bot.dds"
};

} // ~unnamed namespace

namespace Tundra
{

Sky::Sky(Urho3D::Context* context, Scene* scene) :
    IComponent(context, scene),
    INIT_ATTRIBUTE_VALUE(materialRef, "Material", AssetReference("", "Material")),
    INIT_ATTRIBUTE_VALUE(textureRefs, "Texture", AssetReferenceList("Texture")),
    INIT_ATTRIBUTE_VALUE(distance, "Distance", 500),
    INIT_ATTRIBUTE_VALUE(orientation, "Orientation", Quat::identity),
    INIT_ATTRIBUTE_VALUE(drawFirst, "Draw first", true),
    INIT_ATTRIBUTE_VALUE(enabled, "Enabled", true)
{
    materialAsset_ = new AssetRefListener();

    static AttributeMetadata texturesMetadata;
    texturesMetadata.elementType = "AssetReference";
    textureRefs.SetMetadata(&texturesMetadata);

    ParentEntitySet.Connect(this, &Sky::UpdateSignals);   
}

Sky::~Sky()
{
    if (urhoNode_ != nullptr)
    {
        urhoNode_->Remove();
        urhoNode_.Reset();
    }
}

void Sky::UpdateSignals()
{
    Entity* parent = ParentEntity();
    if (!parent)
        return;

    // If scene is not view-enabled, no further action
    if (!ViewEnabled())
        return;
    
    materialAsset_ = new AssetRefListener();
    textureRefListListener_ = new AssetRefListListener(framework->Asset());

    if (parent->ParentScene())
        world_ = parent->ParentScene()->Subsystem<GraphicsWorld>();

    if (world_)
    {
        materialAsset_->Loaded.Connect(this, &Sky::OnMaterialAssetLoaded);

        textureRefListListener_->Changed.Connect(this, &Sky::OnTextureAssetRefsChanged);
        textureRefListListener_->Failed.Connect(this, &Sky::OnTextureAssetFailed);
        textureRefListListener_->Loaded.Connect(this, &Sky::OnTextureAssetLoaded);
    }
}

void Sky::AttributesChanged()
{
    if (!ViewEnabled() || !ParentScene())
        return;

    if (materialRef.ValueChanged() && materialAsset_)
        materialAsset_->HandleAssetRefChange(&materialRef);

    if (textureRefs.ValueChanged() && textureRefListListener_)
        textureRefListListener_->HandleChange(textureRefs.Get());

    if (distance.ValueChanged() || drawFirst.ValueChanged() || orientation.ValueChanged() || enabled.ValueChanged())
        Update();
}

void Sky::CreateSkyboxNode()
{
    if (world_.Expired() || urhoNode_)
        return;

    Urho3D::Scene* urhoScene = world_->UrhoScene();

    urhoNode_ = urhoScene->CreateChild("Skybox");
    Urho3D::Skybox* skybox = urhoNode_->CreateComponent<Urho3D::Skybox>();

    Urho3D::ResourceCache* cache = GetSubsystem<Urho3D::ResourceCache>();
    skybox->SetModel(cache->GetResource<Urho3D::Model>("Models/Box.mdl"));
}

void Sky::Update()
{
    if (!urhoNode_)
        CreateSkyboxNode();

    if (!urhoNode_)
        return;

    urhoNode_->SetScale(distance.Get());
    
    // Set material
    IMaterialAsset *material = dynamic_cast<IMaterialAsset*>(materialAsset_->Asset().Get());
    if (material && material->IsLoaded())
        urhoNode_->GetComponent<Urho3D::Skybox>()->SetMaterial(material->UrhoMaterial());
    else
    {
        Vector<SharedPtr<Urho3D::Image>> images(6);
        Vector<AssetPtr> textureAssets = textureRefListListener_->Assets();
        int numLoadedImages = 0;
        for(uint mi=0; mi<textureAssets.Size(); ++mi)
        {
            AssetPtr &TextureAssetPtr = textureAssets[mi];
            TextureAsset *textureAsset = dynamic_cast<TextureAsset*>(TextureAssetPtr.Get());
            if (textureAsset && textureAsset->IsLoaded())
            {
                SharedPtr<Urho3D::Image> image = SharedPtr<Urho3D::Image>(new Urho3D::Image(GetContext()));
                Vector<u8> data;
                if (!LoadFileToVector(textureAsset->DiskSource(), data))
                    continue;

                Urho3D::MemoryBuffer imageBuffer(&data[0], data.Size());
                if (!image->Load(imageBuffer))
                    continue;

                images[mi] = image;
                numLoadedImages++;
            }
        }

        if (numLoadedImages == 6)
        {
            SharedPtr<Urho3D::TextureCube> textureCube = SharedPtr<Urho3D::TextureCube>(new Urho3D::TextureCube(GetContext()));
            const Urho3D::CubeMapFace faces[6] = { Urho3D::FACE_POSITIVE_X, Urho3D::FACE_NEGATIVE_X, Urho3D::FACE_POSITIVE_Y, Urho3D::FACE_NEGATIVE_Y, Urho3D::FACE_POSITIVE_Z, Urho3D::FACE_NEGATIVE_Z };
            const int faceOrder[6] = { 3, 2, 4, 5, 0, 1 };

            for (size_t i=0 ; i<images.Size() ; ++i)
                if (images[faceOrder[i]] != nullptr)
                    textureCube->SetData(faces[i], images[faceOrder[i]]);
            
            SharedPtr<Urho3D::Technique> technique = SharedPtr<Urho3D::Technique>(new Urho3D::Technique(GetContext()));
            Urho3D::Pass* pass = technique->CreatePass("postopaque");
            pass->SetDepthWrite(false);
            pass->SetVertexShader("Skybox");
            pass->SetPixelShader("Skybox");
            SharedPtr<Urho3D::Material> material = SharedPtr<Urho3D::Material>(new Urho3D::Material(GetContext()));
            material->SetCullMode(Urho3D::CULL_NONE);
            material->SetTechnique(0, technique);
            material->SetTexture(Urho3D::TU_DIFFUSE, textureCube);
        
            urhoNode_->GetComponent<Urho3D::Skybox>()->SetMaterial(material);
        }
    }
}

void Sky::OnMaterialAssetLoaded(AssetPtr asset)
{
    IMaterialAsset* mAsset = dynamic_cast<IMaterialAsset*>(asset.Get());

    if (mAsset)
        Update();
}

void Sky::OnTextureAssetRefsChanged(const AssetReferenceList &/*tRefs*/)
{
    Update();
}

void Sky::OnTextureAssetFailed(uint /*index*/, IAssetTransfer* /*transfer*/, String /*error*/)
{
    Update();
}

void Sky::OnTextureAssetLoaded(uint /*index*/, AssetPtr asset)
{
    Update();
}

}
