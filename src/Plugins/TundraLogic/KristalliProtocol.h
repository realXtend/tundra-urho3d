// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include <kNet/IMessageHandler.h>
#include <kNet/INetworkServerListener.h>
#include <kNet/Network.h>

#include "TundraLogicApi.h"
#include "TundraLogicFwd.h"
#include "Signals.h"
#include "UserConnection.h"

#include <Urho3D/Core/Object.h>
#include <Urho3D/Container/List.h>


namespace Tundra
{

/// Implements kNet protocol -based server and client functionality.
class TUNDRALOGIC_API KristalliProtocol : public Object, public kNet::IMessageHandler, public kNet::INetworkServerListener
{
    URHO3D_OBJECT(KristalliProtocol, Object);

public:
    KristalliProtocol(TundraLogic* owner);
    ~KristalliProtocol();

    void Load();
    void Initialize();
    void Uninitialize();
    void Update(float frametime);

    /// Connects to the Kristalli server at the given address.
    void Connect(const char *ip, unsigned short port, kNet::SocketTransportLayer transport);

    void Disconnect();

    /// Starts a Kristalli server at the given port/transport.
    /// @return true if successful
    bool StartServer(unsigned short port, kNet::SocketTransportLayer transport);
    
    /// Stops Kristalli server
    void StopServer();
    
    /// Invoked by the Network library for each received network message.
    void HandleMessage(kNet::MessageConnection *source, kNet::packet_id_t packetId, kNet::message_id_t id, const char *data, size_t numBytes);

    /// Invoked by the Network library for each new connection
    void NewConnectionEstablished(kNet::MessageConnection* source);
    
    /// Invoked by the Network library for disconnected client
    void ClientDisconnected(kNet::MessageConnection* source);

    bool Connected() const;

    /// Return message connection, for use by other modules (null if no connection made)
    kNet::MessageConnection *MessageConnection() { return serverConnection.ptr(); }

    /// Return server, for use by other modules (null if not running)
    kNet::NetworkServer* NetworkServer() const { return server; }

    kNet::Network *Network() { return &network; }

    /// Return whether we are a server
    bool IsServer() const { return server != 0; }
    
    /// Returns all user connections for a server
    UserConnectionList& UserConnections() { return connections; }

    /// Gets user by message connection. Returns null if no such connection
    UserConnectionPtr UserConnectionBySource(kNet::MessageConnection* source) const;
    UserConnectionPtr UserConnectionById(u32 id) const; /**< @overload @param id Connection ID. */

    /// What trasport layer to use. Read on startup from "--protocol <udp|tcp>". Defaults to UDP if no start param was given.
    kNet::SocketTransportLayer defaultTransport;

    /// Allocate a connection ID for new connection
    u32 AllocateNewConnectionID() const;

    void OpenKNetLogWindow();

    // signals

    /// Triggered whenever a new message is received rom the network.
    //void NetworkMessageReceived(kNet::MessageConnection *source, kNet::packet_id_t packetId, kNet::message_id_t messageId, const char *data, size_t numBytes);
    Signal5<kNet::MessageConnection* ARG(source), kNet::packet_id_t, kNet::message_id_t ARG(messageId), const char* ARG(data), size_t ARG(numBytes)> NetworkMessageReceived;

    /// Triggered on the server side when a new user connects.
    Signal1<UserConnection* ARG(connection)> ClientConnectedEvent;

    /// Triggered on the server side when a user disconnects.
    Signal1<UserConnection* ARG(connection)> ClientDisconnectedEvent;

    /// Triggered on the client side when a server connection attempt has failed.
    Signal0<void> ConnectionAttemptFailed;

private:
    /// This timer tracks when we perform the next reconnection attempt when the connection is lost.
    kNet::PolledTimer reconnectTimer;

    /// Amount of retries remaining for reconnection. Is low for the initial connection, higher for reconnection
    int reconnectAttempts;

    void PerformConnection();
    
    /// If true, the connection attempt we've started has not yet been established, but is waiting
    /// for a transition to OK state. When this happens, the MsgLogin message is sent.
    bool connectionPending;
    
    /// This variable stores the server ip address we are desiring to connect to.
    /// This is used to remember where we need to reconnect in case the connection goes down.
    std::string serverIp;
    /// Store the port number we are desiring to connect to. Used for reconnecting
    unsigned short serverPort;
    /// Store the transport type. Used for reconnecting
    kNet::SocketTransportLayer serverTransport;
    
    TundraLogic* owner;
    kNet::Network network;
    Ptr(kNet::MessageConnection) serverConnection;
    kNet::NetworkServer *server;
    
    /// Users that are connected to server
    UserConnectionList connections;
};

}
