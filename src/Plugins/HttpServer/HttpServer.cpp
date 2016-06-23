// For conditions of distribution and use, see copyright notice in LICENSE

#include "HttpServer.h"
#include "HttpRequest.h"

#include "Framework.h"
#include "CoreDefines.h"
#include "EntityAction.h"
#include "LoggingFunctions.h"

#include "SceneAPI.h"
#include "Scene.h"

#include <Urho3D/Core/Profiler.h>

namespace Tundra
{

HttpServer::HttpServer(Framework* framework) :
    IModule("HttpServer", framework)
{
}

HttpServer::~HttpServer()
{
    StopServer();
}

void HttpServer::Load()
{
    // Allow starting http server also without scene server
    isServer_ = framework->HasCommandLineParameter("--httpPort");
}

void HttpServer::Initialize()
{
    if (isServer_)
        StartServer();
}

void HttpServer::Uninitialize()
{
    StopServer();
}

void HttpServer::Update(float frametime)
{
    if (server_)
    {
        // Update server in main thread so that scenes can be accessed safely in HTTP requests
        URHO3D_PROFILE(PollHTTPServer);
        server_->get_io_service().poll_one();
    }
}

void HttpServer::StartServer()
{
    int port = 0;
    StringList portParam = framework->CommandLineParameters("--httpPort");
    if (!portParam.Empty())
    {
        port = ToInt(portParam.Front());
    }
    if (!port)
    {
        LogWarning("No valid --httpPort parameter given; can not start http server");
        return;
    }

    try
    {
        server_ = ServerPtr(new websocketpp::server<websocketpp::config::asio>());

        // Initialize ASIO transport
        server_->init_asio();

        // Register handler callbacks
        server_->set_http_handler(boost::bind(&HttpServer::OnHttpRequest, this, ::_1));

        // Setup logging
        server_->get_alog().clear_channels(websocketpp::log::alevel::all);
        server_->get_elog().clear_channels(websocketpp::log::elevel::all);
        server_->get_elog().set_channels(websocketpp::log::elevel::rerror);
        server_->get_elog().set_channels(websocketpp::log::elevel::fatal);

        server_->listen(port);

        // Start the server accept loop
        server_->start_accept();
    } 
    catch (std::exception &e) 
    {
        LogError(String(e.what()));
        return;
    }
    
    LogInfo("HTTP server started on port " + String(port));
    ServerStarted.Emit();
}

void HttpServer::StopServer()
{
    try
    {
        if (server_)
        {
            server_->stop();
            ServerStopped.Emit();
            server_.reset();
        }
    }
    catch (std::exception &e) 
    {
        LogError("Error while closing HTTP server: " + String(e.what()));
    }
}

void HttpServer::OnHttpRequest(ConnectionHandle connection)
{
    ConnectionPtr connectionPtr = server_->get_con_from_hdl(connection);
    HttpRequest request(connectionPtr.get());
    HttpRequestReceived.Emit(&request);
}

}

extern "C"
{

DLLEXPORT void TundraPluginMain(Tundra::Framework *fw)
{
    fw->RegisterModule(new Tundra::HttpServer(fw));
}

}
