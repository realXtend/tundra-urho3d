// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "Framework.h"
#include "AssetAPI.h"
#include "ZipBundleFactory.h"

namespace Tundra
{

extern "C"
{

DLLEXPORT void TundraPluginMain(Tundra::Framework *fw)
{
    fw->Asset()->RegisterAssetBundleTypeFactory(AssetBundleTypeFactoryPtr(new ZipBundleFactory()));
}

}

}
