// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "Renderer.h"
#include "AssetAPI.h"
#include "AssetCache.h"
#include "Profiler.h"
#include "LoggingFunctions.h"
#include "IMeshAsset.h"

#include <Model.h>

namespace Tundra
{

IMeshAsset::IMeshAsset(AssetAPI *owner, const String &type_, const String &name_) :
    IAsset(owner, type_, name_)
{
}

IMeshAsset::~IMeshAsset()
{
    Unload();
}

uint IMeshAsset::NumSubmeshes() const
{
    return model ? model->GetNumGeometries() : 0;
}

void IMeshAsset::DoUnload()
{
    model.Reset();
}

bool IMeshAsset::IsLoaded() const
{
    return model != nullptr;
}

Urho3D::Model* IMeshAsset::UrhoModel() const
{
    return model;
}

}
