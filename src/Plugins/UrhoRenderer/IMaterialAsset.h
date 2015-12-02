// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "Math/MathNamespace.h"
#include "IAsset.h"
#include "UrhoRendererApi.h"
#include "UrhoRendererFwd.h"

namespace Tundra
{

/// Represents a material asset. 
/** Base class for varying format implementations (Urho material, Ogre material, Assimp material etc.) */
class URHORENDERER_API IMaterialAsset : public IAsset
{
    URHO3D_OBJECT(IMaterialAsset, IAsset);

public:
    IMaterialAsset(AssetAPI *owner, const String &type_, const String &name_);
    ~IMaterialAsset();

    /// Returns the Urho material resource
    Urho3D::Material* UrhoMaterial() const;

    /// IAsset override.
    bool IsLoaded() const override;

    /// Textures and the units they belong to.
    Vector<Pair<int, AssetReference> > textures_;

protected:
    /// Unload material. IAsset override.
    void DoUnload() override;

    /// Urho material resource. Filled by the loading implementations in subclasses.
    SharedPtr<Urho3D::Material> material;
};

}
