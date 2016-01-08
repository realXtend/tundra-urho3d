// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "JavaScript.h"
#include "Framework.h"
#include "LoggingFunctions.h"
#include "AssetAPI.h"
#include "SceneAPI.h"

namespace Tundra
{

JavaScript::JavaScript(Framework* owner) :
    IModule("JavaScript", owner)
{
}

JavaScript::~JavaScript()
{
}

void JavaScript::Load()
{
    //SceneAPI* scene = framework->Scene();
    //framework->Asset()->RegisterAssetTypeFactory(AssetTypeFactoryPtr(new GenericAssetFactory<ScriptAsset>("ScriptAsset", ".js")));
}

void JavaScript::Initialize()
{
}

void JavaScript::Uninitialize()
{
}

}

extern "C"
{

DLLEXPORT void TundraPluginMain(Tundra::Framework *fw)
{
    fw->RegisterModule(new Tundra::JavaScript(fw));
}

}
