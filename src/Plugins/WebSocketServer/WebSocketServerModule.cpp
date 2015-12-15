#include "WebSocketServerModule.h"
#include "WebSocketServer.h"
#include "WebSocketUserConnection.h"

#include "Framework.h"
#include "CoreDefines.h"
#include "EntityAction.h"
#include "LoggingFunctions.h"

#include "SceneAPI.h"
#include "Scene.h"

namespace Tundra
{

WebSocketServerModule::WebSocketServerModule(Framework* framework) :
    IModule("WebSocketServer", framework),
    LC("[WebSocketServer]: ")
{
}

WebSocketServerModule::~WebSocketServerModule()
{
    StopServer();
}

void WebSocketServerModule::Load()
{
    isServer_ = framework->HasCommandLineParameter("--server");
}

void WebSocketServerModule::Initialize()
{
    if (isServer_)
        StartServer();
}

void WebSocketServerModule::Uninitialize()
{
    StopServer();
}

void WebSocketServerModule::Update(float frametime)
{
    if (!isServer_)
        return;
    
    if (server_.Get())
        server_->Update(frametime);
}

bool WebSocketServerModule::IsServer()
{
    return isServer_;
}

const WebSocketServerPtr& WebSocketServerModule::GetServer()
{
    return server_;
}

void WebSocketServerModule::StartServer()
{
    if (!isServer_)
        return;
    if (server_.Get())
    {
        LogWarning(LC + "Server already started.");
        return;
    }

    // Server
    server_ = WebSocketServerPtr(new WebSocket::Server(framework));
    server_->Start();

    ServerStarted.Emit(server_);
}

void WebSocketServerModule::StopServer()
{
    if (server_.Get())
        server_->Stop();
    server_.Reset();
}

}

extern "C"
{

DLLEXPORT void TundraPluginMain(Tundra::Framework *fw)
{
    fw->RegisterModule(new Tundra::WebSocketServerModule(fw));
}

}
