// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IMaterialAsset.h"
#include "UrhoRendererApi.h"
#include "UrhoRendererFwd.h"

namespace Tundra
{

/// Represents a material asset loaded from Ogre material script
class URHORENDERER_API OgreMaterialAsset : public IMaterialAsset
{
    URHO3D_OBJECT(OgreMaterialAsset, IMaterialAsset);

public:
    OgreMaterialAsset(AssetAPI *owner, const String &type_, const String &name_);

    /// IAsset override.
    bool DeserializeFromData(const u8 *data_, uint numBytes, bool allowAsynchronous) override;
    /// IAsset override.
    Vector<AssetReference> FindReferences() const override;
    /// IAsset override.
    void DependencyLoaded(AssetPtr dependee) override;
};

}
