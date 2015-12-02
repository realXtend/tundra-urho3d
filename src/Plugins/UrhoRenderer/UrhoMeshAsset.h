// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "Math/MathNamespace.h"
#include "IMeshAsset.h"
#include "UrhoRendererApi.h"
#include "UrhoRendererFwd.h"

namespace Tundra
{

/// Represents a mesh asset in Urho native format.
class URHORENDERER_API UrhoMeshAsset : public IMeshAsset
{
    URHO3D_OBJECT(UrhoMeshAsset, IMeshAsset);

public:
    UrhoMeshAsset(AssetAPI *owner, const String &type_, const String &name_);

    /// Load mesh from memory. IAsset override.
    bool DeserializeFromData(const u8 *data_, uint numBytes, bool allowAsynchronous) override;
};

}
