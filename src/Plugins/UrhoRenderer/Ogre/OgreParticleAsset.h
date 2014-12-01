// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "Math/MathNamespace.h"
#include "IParticleAsset.h"
#include "UrhoModuleApi.h"
#include "UrhoModuleFwd.h"

namespace Tundra
{

/// Represents a particle asset loaded from Ogre binary format.
class URHO_MODULE_API OgreParticleAsset : public IParticleAsset
{
    OBJECT(OgreParticleAsset);

public:
    OgreParticleAsset(AssetAPI *owner, const String &type_, const String &name_);

    /// Load mesh from memory. IAsset override.
    bool DeserializeFromData(const u8 *data_, uint numBytes, bool allowAsynchronous) override;

    /// IAsset override.
    Vector<AssetReference> FindReferences() const override;
    /// IAsset override.
    void DependencyLoaded(AssetPtr dependee) override;
};

}
