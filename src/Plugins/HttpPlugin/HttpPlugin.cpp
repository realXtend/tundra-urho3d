// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "HttpPlugin.h"
#include "HttpClient.h"

#include "Framework.h"

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
}

void HttpPlugin::Initialize()
{
}

void HttpPlugin::Uninitialize()
{
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

