// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "Math/MathNamespace.h"
#include "IAsset.h"
#include "UrhoModuleApi.h"
#include "UrhoModuleFwd.h"

namespace Tundra
{

/// Represents a particle asset. May contain several particle effects.
class URHO_MODULE_API IParticleAsset : public IAsset
{
    OBJECT(IParticleAsset);

public:
    IParticleAsset(AssetAPI *owner, const String &type_, const String &name_);
    ~IParticleAsset();

    /// IAsset override.
    bool IsLoaded() const override;

    /// Material and index to the particle system they belong to.
    Vector<Pair<int, AssetReference> > materials_;

    /// Urho3D particle effect resources.
    Vector<SharedPtr<Urho3D::ParticleEffect>> particleEffects_;

protected:
    /// Unload material. IAsset override.
    void DoUnload() override;
};

}
