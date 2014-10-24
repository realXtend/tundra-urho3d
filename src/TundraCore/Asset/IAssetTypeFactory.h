// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"
#include "AssetFwd.h"

#include <Str.h>

namespace Tundra
{

/// A common interface for factories which instantiate assets of different types.
class TUNDRACORE_API IAssetTypeFactory : public RefCounted
{
public:
    virtual ~IAssetTypeFactory() {}

    /// Returns the type of assets this asset type factory can create.
    virtual const String &Type() const = 0;

    /// Returns the file extension of assets that this asset type factory can create.
    virtual const StringVector &TypeExtensions() const = 0;
    
    /// Creates a new asset of the given type that is initialized to the "empty" asset of this type.
    /// @param name The name to give for this asset.
    virtual AssetPtr CreateEmptyAsset(AssetAPI *owner, const String &name) = 0;
};

}
