// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "AssetAPI.h"
#include "AssetCache.h"
#include <Urho3D/Core/Profiler.h>
#include "LoggingFunctions.h"
#include "UrhoMeshAsset.h"

#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Core/Profiler.h>
#include <Urho3D/IO/MemoryBuffer.h>

namespace Tundra
{

UrhoMeshAsset::UrhoMeshAsset(AssetAPI *owner, const String &type_, const String &name_) :
    IMeshAsset(owner, type_, name_)
{
}

bool UrhoMeshAsset::DeserializeFromData(const u8 *data_, uint numBytes, bool /*allowAsynchronous*/)
{
    PROFILE(UrhoMeshAsset_LoadFromFileInMemory);

    /// Force an unload of previous data first.
    Unload();

    Urho3D::MemoryBuffer buffer(data_, numBytes);
    model = new Urho3D::Model(GetContext());
    if (model->Load(buffer))
    {
        assetAPI->AssetLoadCompleted(Name());
        return true;
    }
    else
    {
        LogError("MeshAsset::DeserializeFromData: Failed to load Urho format asset " + Name());
        model.Reset();
        return false;
    }
}

}
