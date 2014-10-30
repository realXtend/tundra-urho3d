// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "Renderer.h"
#include "AssetAPI.h"
#include "AssetCache.h"
#include "Profiler.h"
#include "LoggingFunctions.h"
#include "MeshAsset.h"

#include <Model.h>
#include <Profiler.h>
#include <MemoryBuffer.h>

namespace Tundra
{

MeshAsset::MeshAsset(AssetAPI *owner, const String &type_, const String &name_) :
    IAsset(owner, type_, name_)
{
}

MeshAsset::~MeshAsset()
{
    Unload();
}

bool MeshAsset::DeserializeFromData(const u8 *data_, uint numBytes, bool /*allowAsynchronous*/)
{
    PROFILE(MeshAsset_LoadFromFileInMemory);

    /// Force an unload of previous data first.
    Unload();

    Urho3D::MemoryBuffer buffer(data_, numBytes);
    // Check if data is an Urho model (easy case)
    if (buffer.ReadFileID() == "UMDL")
    {
        buffer.Seek(0);
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

    /// \todo Ogre / Assimp loading

    LogError("MeshAsset::DeserializeFromData: unrecognized data format in asset " + Name());
    return false;
}

size_t MeshAsset::NumSubmeshes() const
{
    return model ? model->GetNumGeometries() : 0;
}

void MeshAsset::DoUnload()
{
    model.Reset();
}

bool MeshAsset::IsLoaded() const
{
    return model != nullptr;
}

Urho3D::Model* MeshAsset::UrhoModel() const
{
    return model;
}

}

/*
bool MeshAsset::IsAssimpFileType() const
{
    const char * const openAssImpFileTypes[] = { ".3d", ".b3d", ".blend", ".dae", ".bvh", ".3ds", ".ase", ".obj", ".ply", ".dxf",
        ".nff", ".smd", ".vta", ".mdl", ".md2", ".md3", ".mdc", ".md5mesh", ".x", ".q3o", ".q3s", ".raw", ".ac",
        ".stl", ".irrmesh", ".irr", ".off", ".ter", ".mdl", ".hmp", ".ms3d", ".lwo", ".lws", ".lxo", ".csm",
        ".ply", ".cob", ".scn", ".fbx" };

    for(size_t i = 0; i < NUMELEMS(openAssImpFileTypes); ++i)
        if (this->Name().endsWith(openAssImpFileTypes[i], Qt::CaseInsensitive))
            return true;

    return false;
}
*/
