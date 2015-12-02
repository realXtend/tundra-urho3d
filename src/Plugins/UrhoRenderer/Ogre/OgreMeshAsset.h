// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "Math/MathNamespace.h"
#include "IMeshAsset.h"
#include "UrhoRendererApi.h"
#include "UrhoRendererFwd.h"

namespace Tundra
{

/// Represents a mesh asset loaded from Ogre binary format
class URHORENDERER_API OgreMeshAsset : public IMeshAsset
{
    URHO3D_OBJECT(OgreMeshAsset, IMeshAsset);

public:
    OgreMeshAsset(AssetAPI *owner, const String &type_, const String &name_);

    /// Load mesh from memory. IAsset override.
    bool DeserializeFromData(const u8 *data_, uint numBytes, bool allowAsynchronous) override;
};

}
