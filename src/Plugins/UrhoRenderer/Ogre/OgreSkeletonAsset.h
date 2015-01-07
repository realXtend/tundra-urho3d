// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "Math/MathNamespace.h"
#include "IMeshAsset.h"
#include "UrhoModuleApi.h"
#include "UrhoModuleFwd.h"
#include "CoreTypes.h"

#include <Urho3D/Graphics/Skeleton.h>

namespace Tundra
{

/// Represents an Ogre skeleton and the animations it contains
class URHO_MODULE_API OgreSkeletonAsset : public IAsset
{
    OBJECT(OgreSkeletonAsset);

public:
    OgreSkeletonAsset(AssetAPI *owner, const String &type_, const String &name_);
    ~OgreSkeletonAsset();

    /// Load skeleton from memory. IAsset override.
    bool DeserializeFromData(const u8 *data_, uint numBytes, bool allowAsynchronous) override;

    /// Returns the Urho skeleton
    const Urho3D::Skeleton& UrhoSkeleton() const { return skeleton; }

    /// Returns all skeletal animations contained by this asset, keyed by their names.
    const HashMap<String, SharedPtr<Urho3D::Animation> >& UrhoAnimations() const { return animations; }

    /// Return an animation by name or null if not found.
    Urho3D::Animation* AnimationByName(const String& name) const;

    /// IAsset override.
    bool IsLoaded() const override;

protected:
    /// Unload skeleton and animations. IAsset override.
    void DoUnload() override;

private:
    Urho3D::Skeleton skeleton;
    HashMap<String, SharedPtr<Urho3D::Animation> > animations;
};

}
