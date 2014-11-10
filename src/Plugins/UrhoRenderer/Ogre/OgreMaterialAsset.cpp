// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "OgreMaterialAsset.h"
#include "OgreMaterialDefines.h"
#include "LoggingFunctions.h"

#include "AssetAPI.h"
#include "TextureAsset.h"

#include <Profiler.h>
#include <Graphics/Material.h>
#include <Graphics/Technique.h>
#include <Graphics/Texture.h>
#include <Graphics/Texture2D.h>
#include <Resource/ResourceCache.h>
#include <StringUtils.h>

namespace Tundra
{

OgreMaterialAsset::OgreMaterialAsset(AssetAPI *owner, const String &type_, const String &name_) :
    IMaterialAsset(owner, type_, name_)
{
}

bool OgreMaterialAsset::DeserializeFromData(const u8 *data_, uint numBytes, bool /*allowAsynchronous*/)
{
    PROFILE(OgreMaterialAsset_LoadFromFileInMemory);

    /// Force an unload of previous data first.
    Unload();

    Ogre::MaterialParser parser;
    bool success = parser.Parse((const char*)data_, numBytes);
    if (success)
    {
        material = new Urho3D::Material(GetContext());
        material->SetNumTechniques(1);

        //LogInfo(Name().CString());
        //parser.root->Dump();

        // Pass
        Ogre::MaterialBlock *tech = parser.root->Technique(0);
        Ogre::MaterialBlock *pass = (tech ? tech->Pass(0) : 0);
        if (!tech || !pass)
        {
            LogError("OgreMaterialAsset::DeserializeFromData: No technique with a pass found in " + Name());
            material.Reset();
            assetAPI->AssetLoadFailed(Name());
            return false;
        }
        if (pass->Has(Ogre::Material::Pass::Diffuse))
            material->SetShaderParameter("MatDiffColor", pass->ColorValue(Ogre::Material::Pass::Diffuse, Urho3D::Color::WHITE));

        // Texture unit
        Ogre::MaterialBlock *tu = pass->TextureUnit(0);
        if (tu)
        {
            String textureRef = tu->StringValue(Ogre::Material::TextureUnit::Texture, "");
            if (!textureRef.Empty())
                textures_.Push(AssetReference(assetAPI->ResolveAssetRef(Name(), textureRef), "Texture"));
        }

        // Set fitting techinique
        String techniqueName = (textures_.Size() ? "Diff.xml" : "NoTexture.xml");
        material->SetTechnique(0, GetSubsystem<Urho3D::ResourceCache>()->GetResource<Urho3D::Technique>("Techniques/" + techniqueName));

        // Inform load has finished. Triggering any textures_ to be fetched.
        assetAPI->AssetLoadCompleted(Name());
    }
    else
    {
        LogError("OgreMaterialAsset::DeserializeFromData: " + parser.Error());
        assetAPI->AssetLoadFailed(Name());
    }
    return success;
}

void OgreMaterialAsset::DependencyLoaded(AssetPtr dependee)
{
    TextureAsset *texture = dynamic_cast<TextureAsset*>(dependee.Get());
    if (texture)
    {
        LogDebug("Texture loaded: " + texture->Name());
        material->SetTexture(Urho3D::TU_DIFFUSE, texture->UrhoTexture());
    }

    LoadCompleted();
}

Vector<AssetReference> OgreMaterialAsset::FindReferences() const
{
    return textures_;
}

}
