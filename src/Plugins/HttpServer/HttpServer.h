// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "HttpServerApi.h"

#include "IModule.h"
#include "CoreTypes.h"
#include "CoreDefines.h"
#include "Signals.h"

#include <boost/system/error_code.hpp>
#ifdef BOOST_SYSTEM_NOEXCEPT
#define _WEBSOCKETPP_NOEXCEPT_TOKEN_ BOOST_SYSTEM_NOEXCEPT
#endif

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

namespace Tundra
{

class HTTPSERVER_API HttpServer : public IModule
{
    typedef boost::shared_ptr<websocketpp::server<websocketpp::config::asio> > ServerPtr;
    typedef websocketpp::server<websocketpp::config::asio>::connection_ptr ConnectionPtr;
    typedef boost::weak_ptr<websocketpp::server<websocketpp::config::asio>::connection_type> ConnectionWeakPtr;
    typedef websocketpp::connection_hdl ConnectionHandle;
    typedef websocketpp::server<websocketpp::config::asio>::message_ptr MessagePtr;

public:
    HttpServer(Framework* framework);
    virtual ~HttpServer();

    void Load();
    void Initialize();
    void Uninitialize();
    
    void Update(float frametime);

    Signal0<void> ServerStarted;
    Signal0<void> ServerStopped;

private:
    bool isServer_;

    void StartServer();
    void StopServer();
    
    ServerPtr server_;
};

}
