// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "OgreMaterialAsset.h"
#include "OgreMaterialDefines.h"
#include "IOgreMaterialProcessor.h"
#include "LoggingFunctions.h"
#include "Framework.h"
#include "AssetAPI.h"
#include "TextureAsset.h"
#include "UrhoRenderer.h"

#include <Profiler.h>
#include <Graphics/Material.h>
#include <Graphics/Texture2D.h>
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
    if (parser.Parse((const char*)data_, numBytes))
    {
        material = new Urho3D::Material(GetContext());
        material->SetNumTechniques(1);

        UrhoRenderer* renderer = static_cast<UrhoRenderer*>(assetAPI->GetFramework()->Renderer());
        IOgreMaterialProcessor* proc = renderer->FindOgreMaterialProcessor(parser);
        if (proc)
        {
            proc->Convert(parser, this);
            // Inform load has finished. Triggering any textures_ to be fetched.
            assetAPI->AssetLoadCompleted(Name());
            return true;
        }

        LogError("OgreMaterialAsset::DeserializeFromData: no material processor found that could handle data in " + Name());
        material.Reset();
        return false;
    }

    LogError("OgreMaterialAsset::DeserializeFromData: parse failed for " + Name() + ": " + parser.Error());
    material.Reset();
    return false;
}

void OgreMaterialAsset::DependencyLoaded(AssetPtr dependee)
{
    TextureAsset *texture = dynamic_cast<TextureAsset*>(dependee.Get());
    if (texture)
    {
        LogDebug("Texture loaded: " + texture->Name());
        bool found = false;
        for (uint i = 0; i < textures_.Size(); ++i)
        {
            /// \todo Is this ref compare reliable?
            if (!textures_[i].second_.ref.Compare(texture->Name(), false))
            {
                material->SetTexture((Urho3D::TextureUnit)textures_[i].first_, texture->UrhoTexture());
                found = true;
            }
        }

        if (!found)
            LogWarning("OgreMaterialAsset::DependencyLoaded: texture " + texture->Name() + " was not found as ref in any of the texture units in " + Name());
    }

    LoadCompleted();
}

Vector<AssetReference> OgreMaterialAsset::FindReferences() const
{
    Vector<AssetReference> ret;
    for (uint i = 0; i < textures_.Size(); ++i)
        ret.Push(textures_[i].second_);
    return ret;
}

}
