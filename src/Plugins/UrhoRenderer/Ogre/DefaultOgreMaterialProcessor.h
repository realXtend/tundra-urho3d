// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IOgreMaterialProcessor.h"

namespace Tundra
{

/// Default Ogre material processor. 
class URHORENDERER_API DefaultOgreMaterialProcessor : public IOgreMaterialProcessor
{
    URHO3D_OBJECT(DefaultOgreMaterialProcessor, IOgreMaterialProcessor);

public:
    /// Construct.
    DefaultOgreMaterialProcessor(Urho3D::Context* context);

    /// Return whether can convert a specific Ogre material.
    bool CanConvert(const Ogre::MaterialParser& src) override;

    /// Convert the Ogre material parsing result into the given asset, and fill the per-unit texture refs. Assumed to succeed if CanConvert for the same parsed data already returned true.
    void Convert(const Ogre::MaterialParser& src, OgreMaterialAsset* dest) override;
};

}