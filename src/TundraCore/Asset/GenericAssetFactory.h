// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IAssetTypeFactory.h"
#include "BinaryAsset.h"

#include <cassert>

namespace Tundra
{

/// A factory for instantiating assets of a templated type T.
/** GenericAssetFactory is a predefined concrete factory type anyone defining a new asset type can use
    to create new assets of any type. */
template<typename AssetType>
class GenericAssetFactory : public IAssetTypeFactory
{
public:
    explicit GenericAssetFactory(const String &assetType_, const String &assetTypeExtension) :
        assetType(assetType_.Trimmed())
    {
        assert(!assetType.Empty() && "Must specify an asset type for asset factory!");
        // assetTypeExtension can be empty in the case of BinaryAsset, don't assert it.
        assetTypeExtensions.Push(assetTypeExtension);
    }

    explicit GenericAssetFactory(const String &assetType_, const StringVector &assetTypeExtensions_) :
        assetType(assetType_.Trimmed()),
        assetTypeExtensions(assetTypeExtensions_)
    {
        assert(!assetType.Empty() && "Must specify an asset type for asset factory!");
        assert(!assetTypeExtensions.Empty() && "Must specify at least one asset type extension for asset factory!");
    }

    virtual const String &Type() const { return assetType; }
    
    virtual const StringVector &TypeExtensions() const { return assetTypeExtensions; }

    virtual AssetPtr CreateEmptyAsset(AssetAPI *owner, const String &name) { return AssetPtr(new AssetType(owner, Type(), name)); }

private:
    const String assetType;
    StringVector assetTypeExtensions;
};

/// For simple asset types the client wants to parse, we define the BinaryAssetFactory type.
typedef GenericAssetFactory<BinaryAsset> BinaryAssetFactory;

}

