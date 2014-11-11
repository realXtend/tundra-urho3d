// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "Renderer.h"
#include "AssetAPI.h"
#include "AssetCache.h"
#include "Profiler.h"
#include "LoggingFunctions.h"
#include "IMaterialAsset.h"

#include <Engine/Graphics/Material.h>

namespace Tundra
{

IMaterialAsset::IMaterialAsset(AssetAPI *owner, const String &type_, const String &name_) :
    IAsset(owner, type_, name_)
{
}

IMaterialAsset::~IMaterialAsset()
{
    Unload();
}

void IMaterialAsset::DoUnload()
{
    material.Reset();
    textures_.Clear();
}

bool IMaterialAsset::IsLoaded() const
{
    return material != nullptr;
}

Urho3D::Material* IMaterialAsset::UrhoMaterial() const
{
    return material;
}

}
