// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IMaterialAsset.h"
#include "UrhoModuleApi.h"
#include "UrhoModuleFwd.h"

namespace Tundra
{

/// Represents a material asset loaded from Ogre material script
class URHO_MODULE_API OgreMaterialAsset : public IMaterialAsset
{
    OBJECT(OgreMaterialAsset);

public:
    OgreMaterialAsset(AssetAPI *owner, const String &type_, const String &name_);

    /// IAsset override.
    bool DeserializeFromData(const u8 *data_, uint numBytes, bool allowAsynchronous) override;
    /// IAsset override.
    Vector<AssetReference> FindReferences() const override;
    /// IAsset override.
    void DependencyLoaded(AssetPtr dependee) override;

private:
    Vector<AssetReference> textures_;
};

}
