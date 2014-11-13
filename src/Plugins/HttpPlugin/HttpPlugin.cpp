// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "HttpPlugin.h"
#include "HttpClient.h"
#include "HttpAsset/HttpAssetProvider.h"

#include "Framework.h"
#include "AssetAPI.h"

#include <Engine/Container/Ptr.h>

namespace Tundra
{

HttpPlugin::HttpPlugin(Framework* owner) :
    IModule("HttpPlugin", owner)
{
}

HttpPlugin::~HttpPlugin()
{
}

void HttpPlugin::Load()
{
    client_ = new HttpClient(framework);
    provider_ = new HttpAssetProvider(framework, client_);

    framework->Asset()->RegisterAssetProvider(Urho3D::StaticCast<IAssetProvider>(provider_));
}

void HttpPlugin::Initialize()
{
    client_->Initialize();
}

void HttpPlugin::Uninitialize()
{
    provider_.Reset();
    client_.Reset();
}

void HttpPlugin::Update(float frametime)
{
    if (client_)
        client_->Update(frametime);
}

extern "C"
{

DLLEXPORT void TundraPluginMain(Tundra::Framework *fw)
{
    fw->RegisterModule(new Tundra::HttpPlugin(fw));
}

}

}
