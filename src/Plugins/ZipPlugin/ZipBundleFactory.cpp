// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "ZipBundleFactory.h"
#include "ZipAssetBundle.h"

namespace Tundra
{

ZipBundleFactory::ZipBundleFactory()
{
    typesExtensions_.Push(".zip");
}

String ZipBundleFactory::Type() const
{
    return "Zip";
}

StringVector ZipBundleFactory::TypeExtensions() const
{
    return typesExtensions_;
}

AssetBundlePtr ZipBundleFactory::CreateEmptyAssetBundle(AssetAPI *owner, const String &name)
{
    if (name.EndsWith(".zip", false))
        return AssetBundlePtr(new ZipAssetBundle(owner, Type(), name));
    return AssetBundlePtr();
}

}
