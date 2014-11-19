// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "DefaultOgreMaterialProcessor.h"
#include "OgreMaterialDefines.h"
#include "OgreMaterialAsset.h"
#include "LoggingFunctions.h"
#include "AssetAPI.h"

#include <Graphics/Material.h>
#include <Graphics/Technique.h>
#include <Resource/ResourceCache.h>
#include <StringUtils.h>

namespace Tundra
{

DefaultOgreMaterialProcessor::DefaultOgreMaterialProcessor(Urho3D::Context* context) :
    IOgreMaterialProcessor(context)
{
}

bool DefaultOgreMaterialProcessor::CanConvert(const Ogre::MaterialParser& src)
{
    Ogre::MaterialBlock *tech = src.root->Technique(0);
    Ogre::MaterialBlock *pass = (tech ? tech->Pass(0) : 0);
    return tech && pass; // Valid if has the first technique/pass
}

void DefaultOgreMaterialProcessor::Convert(const Ogre::MaterialParser& src, OgreMaterialAsset* dest)
{
    assert(dest);
    assert(dest->UrhoMaterial());

    Urho3D::Material* urhoMat = dest->UrhoMaterial();
    AssetAPI* assetAPI = dest->GetAssetAPI();

    Ogre::MaterialBlock *tech = src.root->Technique(0);
    Ogre::MaterialBlock *pass = (tech ? tech->Pass(0) : 0);
    assert (tech && pass);

    String blendTypeString;
    bool hasDiffuse = false;
    bool hasNormal = false;

    // Blend type
    if (pass->Has(Ogre::Material::Pass::SceneBlend))
    {
        if (pass->StringValue(Ogre::Material::Pass::SceneBlend, "").StartsWith("alpha", false))
            blendTypeString = "Alpha";
    }

    // Texture units. Textures are set as the asset's dependencies
    for (uint i = 0; i < pass->NumTextureUnits(); ++i)
    {
        Ogre::MaterialBlock *tu = pass->TextureUnit(i);
        if (tu)
        {
            Urho3D::TextureUnit tuIndex = Urho3D::MAX_TEXTURE_UNITS;
            if (tu->id.StartsWith("normal", false))
            {
                hasNormal = true;
                tuIndex = Urho3D::TU_NORMAL;
            }
            // The texture units may have various names. Interpret the first unknown as diffuse
            else if (hasDiffuse == false || tu->id.StartsWith("diffuse", false))
            {
                hasDiffuse = true;
                tuIndex = Urho3D::TU_DIFFUSE;
            }

            if (tuIndex == Urho3D::MAX_TEXTURE_UNITS)
                continue; // Unrecognized texture unit

            String textureRef = tu->StringValue(Ogre::Material::TextureUnit::Texture, "");
            if (!textureRef.Empty())
                dest->textures_.Push(Urho3D::MakePair((int)tuIndex, AssetReference(assetAPI->ResolveAssetRef(dest->Name(), textureRef), "Texture")));
            
            String cubeTextureRef = tu->StringValue(Ogre::Material::TextureUnit::CubicTexture, "");
            if (!cubeTextureRef.Empty())
            {
                StringList refs = cubeTextureRef.Split(' ');
                if (refs.Size() >= 6)
                    for (uint j=0 ; j<6 ; ++j)
                    {
                        ///\todo Try to load as binary, but in some cases the asset may already exist as TextureAsset. This emits a warning.
                        AssetReference assetReference = AssetReference(assetAPI->ResolveAssetRef(dest->Name(), refs[j]), "Binary");
                        dest->textures_.Push(Urho3D::MakePair((int)tuIndex, assetReference));
                    }
            }
        }
    }

    // Misc. shader parameters
    if (pass->Has(Ogre::Material::Pass::Diffuse))
        urhoMat->SetShaderParameter("MatDiffColor", pass->ColorValue(Ogre::Material::Pass::Diffuse, Urho3D::Color::WHITE));

    // Build final technique name
    String techniqueName = "NoTexture";
    if (hasDiffuse)
    {
        techniqueName = "Diff";
        if (hasNormal)
            techniqueName += "Normal";
    }
    techniqueName += blendTypeString;

    urhoMat->SetTechnique(0, GetSubsystem<Urho3D::ResourceCache>()->GetResource<Urho3D::Technique>("Techniques/" + techniqueName + ".xml"));
}

}