// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"
#include "IAssetTypeFactory.h"

#include <cassert>

namespace Tundra
{

/// A factory that always returns a null pointer for creating assets.
/** This factory is used to ignore certain asset types when they are supposed to never be used in the system.
    Another way to disable the use of certain assets is to not register a factory at all for that type.
    However, that will log out an error message. Instead, by using the Null Factory we can signal that 
    we are ok that the assets of the given type are not loaded. */
class TUNDRACORE_API NullAssetFactory : public IAssetTypeFactory
{
public:
    explicit NullAssetFactory(const String &assetType_, const String &assetTypeExtension) :
        assetType(assetType_.Trimmed())
    {
        assert(!assetType.Empty() && "Must specify an asset type for null asset factory!");
        assert(!assetTypeExtension.Trimmed().Empty() && "Asset type extension cannot be empty for null asset factory!");
        assetTypeExtensions.Push(assetTypeExtension);
    }
    
    explicit NullAssetFactory(const String &assetType_, const StringVector &assetTypeExtensions_) :
        assetType(assetType_.Trimmed()),
        assetTypeExtensions(assetTypeExtensions_)
    {
        assert(!assetType.Empty() && "Must specify an asset type for null asset factory!");
        assert(!assetTypeExtensions.Empty() && "Must specify at least one asset type extension for null asset factory!");
    }

    virtual const String &Type() const { return assetType; }
    
    virtual const StringVector &TypeExtensions() const { return assetTypeExtensions; }

    virtual AssetPtr CreateEmptyAsset(AssetAPI *, const String &) { return AssetPtr(); }

private:
    const String assetType;
    StringVector assetTypeExtensions;
};

}
