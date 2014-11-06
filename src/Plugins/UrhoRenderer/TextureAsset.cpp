// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "Renderer.h"
#include "AssetAPI.h"
#include "AssetCache.h"
#include "Profiler.h"
#include "LoggingFunctions.h"
#include "TextureAsset.h"

#include <MemoryBuffer.h>
#include <Texture2D.h>
#include <Material.h>

namespace Tundra
{

TextureAsset::TextureAsset(AssetAPI *owner, const String &type_, const String &name_) :
    IAsset(owner, type_, name_)
{
}

TextureAsset::~TextureAsset()
{
    Unload();
}

bool TextureAsset::DeserializeFromData(const u8 *data_, uint numBytes, bool /*allowAsynchronous*/)
{
    PROFILE(TextureAsset_LoadFromFileInMemory);

    bool success = false;

    /// Force an unload of previous data first.
    Unload();
    
    Urho3D::MemoryBuffer imageBuffer(data_, numBytes);
    
    texture = new Urho3D::Texture2D(GetContext());
    if (texture->Load(imageBuffer))
    {
        assetAPI->AssetLoadCompleted(Name());
        success = true;
    } else
    {
        LogError("TextureAsset::DeserializeFromData: Failed to load texture asset " + Name());
        texture.Reset();
    }

    return success;
}

void TextureAsset::DoUnload()
{
    texture.Reset();
}

bool TextureAsset::IsLoaded() const
{
    return texture != nullptr;
}

Urho3D::Texture2D* TextureAsset::UrhoTexture() const
{
    return texture;
}

size_t TextureAsset::Height() const
{
    if (!IsLoaded())
        return 0;

    return texture->GetHeight();
}

size_t TextureAsset::Width() const
{
    if (!IsLoaded())
        return 0;

    return texture->GetWidth();
}

}
