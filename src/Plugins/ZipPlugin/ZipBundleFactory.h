// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "ZipPluginApi.h"
#include "ZipPluginFwd.h"

#include "IAssetBundleTypeFactory.h"

namespace Tundra
{

/// Factory for zip assets bundles.
class TUNDRA_ZIP_API ZipBundleFactory : public IAssetBundleTypeFactory
{
    
public:
    ZipBundleFactory();

    String Type() const override;
    StringVector TypeExtensions() const override;
    AssetBundlePtr CreateEmptyAssetBundle(AssetAPI *owner, const String &name) override;

private:
    StringVector typesExtensions_;
};

}
