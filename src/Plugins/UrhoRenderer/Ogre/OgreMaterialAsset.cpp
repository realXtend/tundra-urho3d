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
        material->SetNumTechniques(1); // parser.root->NumTechniques();

        /** @todo Support multiple techs/passes. This is initial code to get something to render.
            Might make most sense to only support single tech/pass/texture fixed pipeline via "Techniques/Diff.xml"
            and rex/meshmoon shaders with proper Urho shaders (these source materials are never multi pass/tech) */
        //for (uint ti=0, tinum=parser.root->NumTechniques(); ti<tinum; ++ti) etc..
        
        Ogre::MaterialBlock *tech = parser.root->Technique(0);
        Ogre::MaterialBlock *pass = (tech ? tech->Pass(0) : 0);
        // Nothing to push to Urho
        if (!tech || !pass)
        {
            LogError("OgreMaterialAsset: No technique with a pass found in " + Name());
            material.Reset();
            assetAPI->AssetLoadFailed(Name());
            return false;
        }
        Ogre::MaterialBlock *tu = pass->TextureUnit(0);
        if (tu)
        {
            String textureRef = tu->StringValue(Ogre::Material::TextureUnit::Texture, "");
            if (!textureRef.Empty())
                textures_.Push(AssetReference(assetAPI->ResolveAssetRef(Name(), textureRef), "Texture"));
        }
        String diffuse = pass->StringValue(Ogre::Material::Pass::Diffuse, "");
        if (!diffuse.Empty())
            material->SetShaderParameter("MatDiffColor", pass->ColorValue(Ogre::Material::Pass::Diffuse, Urho3D::Color::WHITE));

        String techniqueName = textures_.Size() ? "Diff.xml" : "NoTexture.xml";
        material->SetTechnique(0, GetSubsystem<Urho3D::ResourceCache>()->GetResource<Urho3D::Technique>("Techniques/" + techniqueName));
        
        assetAPI->AssetLoadCompleted(Name());
    }
    else
    {
        assetAPI->AssetLoadFailed(Name());
        LogError("OgreMaterialAsset: " + parser.Error());
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
