// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "UrhoModuleApi.h"
#include "UrhoModuleFwd.h"
#include "AssetFwd.h"
#include "CoreTypes.h"
#include "AssetReference.h"

#include <Urho3D/Core/Object.h>

namespace Tundra
{

class OgreMaterialAsset;

/// Interface for Ogre material processing classes which can convert a subset of Ogre materials to Urho materials, using eg. specific shaders.
class URHO_MODULE_API IOgreMaterialProcessor : public Object
{
public:
    /// Construct.
    IOgreMaterialProcessor(Urho3D::Context* context) :
        Object(context)
    {
    }

    /// Return whether can convert a specific Ogre material.
    virtual bool CanConvert(const Ogre::MaterialParser& src) = 0;

    /// Convert the Ogre material parsing result into the given asset, and fill the per-unit texture refs. Assumed to succeed if CanConvert for the same parsed data already returned true.
    /** Note: the OgreMaterialAsset in question must already have created the Urho material to be filled. */
    virtual void Convert(const Ogre::MaterialParser& src, OgreMaterialAsset* dest) = 0;
};

}