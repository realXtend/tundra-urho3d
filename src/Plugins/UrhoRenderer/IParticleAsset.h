// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "Math/MathNamespace.h"
#include "IAsset.h"
#include "UrhoModuleApi.h"
#include "UrhoModuleFwd.h"

namespace Tundra
{

/// Represents a particle asset. 
class URHO_MODULE_API IParticleAsset : public IAsset
{
    OBJECT(IParticleAsset);

public:
    IParticleAsset(AssetAPI *owner, const String &type_, const String &name_);
    ~IParticleAsset();

    /// Returns the Urho model resource
    Urho3D::ParticleEffect* UrhoParticleEffect() const { return particleEffect_; };

    /// IAsset override.
    bool IsLoaded() const override;

    /// Material and index to the particle system they belong to.
    Vector<Pair<int, AssetReference> > materials_;

protected:
    /// Unload material. IAsset override.
    void DoUnload() override;

    /// Urho3D particle effect resource.
    SharedPtr<Urho3D::ParticleEffect> particleEffect_;
};

}
