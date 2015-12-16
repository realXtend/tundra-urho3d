// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "WebSocketServerApi.h"
#include "Win.h"
#include "CoreTypes.h"
#include "CoreDefines.h"
#include "FrameworkFwd.h"
#include "WebSocketFwd.h"
#include "kNetFwd.h"
#include "AssetFwd.h"
#include "AssetReference.h"

#include "SyncState.h"
#include "MsgEntityAction.h"
#include "EntityAction.h"

#include "StdPtr.h"
#include "Signals.h"

#include "kNet/DataSerializer.h"

#include <boost/system/error_code.hpp>
#define _WEBSOCKETPP_NOEXCEPT_TOKEN_ BOOST_SYSTEM_NOEXCEPT

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <Urho3D/Core/Mutex.h>
#include <Urho3D/Core/Thread.h>

namespace WebSocket
{
    typedef boost::shared_ptr<websocketpp::server<websocketpp::config::asio> > ServerPtr;
    typedef websocketpp::server<websocketpp::config::asio>::connection_ptr ConnectionPtr;
    typedef boost::weak_ptr<websocketpp::server<websocketpp::config::asio>::connection_type> ConnectionWeakPtr;
    typedef websocketpp::connection_hdl ConnectionHandle;
    typedef websocketpp::server<websocketpp::config::asio>::message_ptr MessagePtr;
    typedef boost::shared_ptr<kNet::DataSerializer> DataSerializerPtr;
    
    // WebSocket events
    struct SocketEvent
    {
        enum EventType
        {
            None = 0,
            Connected,
            Disconnected,
            Data
        };

        WebSocket::ConnectionPtr connection;
        DataSerializerPtr data;
        EventType type;

        SocketEvent() : type(None) {}
        SocketEvent(WebSocket::ConnectionPtr connection_, EventType type_) : connection(connection_), type(type_) {}
    };

    /// Server run thread
    class ServerThread : public Urho3D::Thread
    {
    public:
        virtual void ThreadFunction();

        WebSocket::ServerPtr server_;
    };

    /// WebSocket server. 
    /** Manages user requestedConnections and receiving/sending out data with them.
        All signals emitted by this object will be in the main thread. */
    class WEBSOCKETSERVER_API Server : public Urho3D::Object
    {
        URHO3D_OBJECT(Server, Object);

    public:
        Server(Tundra::Framework *framework);
        ~Server();
        
        bool Start();
        void Stop();
        void Update(float frametime);
        
        friend class Handler;
        
    public:
        /// Returns client with id, null if not found.
        WebSocket::UserConnectionPtr UserConnection(Tundra::uint connectionId);

        /// Returns client with websocket connection ptr, null if not found.
        WebSocket::UserConnectionPtr UserConnection(WebSocket::ConnectionPtr connection);
        
        /// Mirror the Server object API.
        WebSocket::UserConnectionPtr GetUserConnection(Tundra::uint connectionId) { return UserConnection(connectionId); }

        /// Returns all user connections
        WebSocket::UserConnectionList UserConnections() { return connections_; }

        /// The server has been started
        Tundra::Signal0<void> ServerStarted;

        /// The server has been stopped
        Tundra::Signal0<void> ServerStopped;
        
        /// Network message received from client
        Tundra::Signal4<WebSocket::UserConnection* ARG(source), kNet::message_id_t ARG(id), const char* ARG(data), size_t ARG(numBytesNetworkMessageReceived)> NetworkMessageReceived;

    protected:
        void Reset();

        void OnConnected(WebSocket::ConnectionHandle connection);
        void OnDisconnected(WebSocket::ConnectionHandle connection);
        void OnMessage(WebSocket::ConnectionHandle connection, WebSocket::MessagePtr data);
        void OnHttpRequest(WebSocket::ConnectionHandle connection);
        void OnSocketInit(WebSocket::ConnectionHandle connection, boost::asio::ip::tcp::socket& s);
        
    private:
        Tundra::String LC;
        Tundra::ushort port_;
        
        Tundra::Framework *framework_;
        
        WebSocket::ServerPtr server_;

        // Websocket connections. Once login is finalized, they are also added to TundraProtocolModule's connection list
        WebSocket::UserConnectionList connections_;

        ServerThread thread_;

        Urho3D::Mutex mutexEvents_;
        Tundra::Vector<SocketEvent*> events_;
    };
}
