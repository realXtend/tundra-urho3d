// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "HttpServerApi.h"

#include "IModule.h"
#include "CoreTypes.h"
#include "CoreDefines.h"
#include "Signals.h"
#include "HttpRequest.h"

#include <boost/system/error_code.hpp>
#ifdef BOOST_SYSTEM_NOEXCEPT
#define _WEBSOCKETPP_NOEXCEPT_TOKEN_ BOOST_SYSTEM_NOEXCEPT
#endif

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

namespace Tundra
{

class JavaScriptInstance;

class HTTPSERVER_API HttpServer : public IModule
{
    URHO3D_OBJECT(HttpServer, IModule);

public:
    typedef boost::shared_ptr<websocketpp::server<websocketpp::config::asio> > ServerPtr;
    typedef websocketpp::connection<websocketpp::config::asio> Connection;
    typedef websocketpp::server<websocketpp::config::asio>::connection_ptr ConnectionPtr;
    typedef boost::weak_ptr<websocketpp::server<websocketpp::config::asio>::connection_type> ConnectionWeakPtr;
    typedef websocketpp::connection_hdl ConnectionHandle;
    typedef websocketpp::server<websocketpp::config::asio>::message_ptr MessagePtr;

    HttpServer(Framework* framework);
    virtual ~HttpServer();

    void Load();
    void Initialize();
    void Uninitialize();
    
    void Update(float frametime);

    /// The HTTP server was started.
    Signal0<void> ServerStarted;
    /// The server was stopped.
    Signal0<void> ServerStopped;
    /// A request was received. Handle this signal to set response headers, body and status code. Do not hold on to the HTTP request object; it will be invalid after the signal handling is complete.
    Signal1<HttpRequest*> HttpRequestReceived;

private:
    /// Handle http request from websocketpp internally.
    void OnHttpRequest(ConnectionHandle connection);
    void StartServer();
    void StopServer();
    
    /// Handles script engine creation (register HttpServer classes)
    void OnScriptInstanceCreated(JavaScriptInstance* instance);

    bool isServer_;
    ServerPtr server_;
};

}
