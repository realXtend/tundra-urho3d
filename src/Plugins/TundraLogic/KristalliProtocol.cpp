// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "KristalliProtocol.h"
#include "TundraLogic.h"

#include "Framework.h"
#include "CoreStringUtils.h"
#include "ConsoleAPI.h"
#include "LoggingFunctions.h"

#include <Urho3D/Core/Profiler.h>

#include <kNet.h>
#include <kNet/UDPMessageConnection.h>

namespace Tundra
{
static const int cInitialAttempts = 0;
static const int cReconnectAttempts = 0;

KristalliProtocol::KristalliProtocol(TundraLogic* owner) :
    Object(owner->GetContext()),
    owner(owner),
    server(0),
    serverPort(0),
    serverConnection(0),
    connectionPending(false),
    reconnectAttempts(0)
{
}

KristalliProtocol::~KristalliProtocol()
{
    Disconnect();
}

void KristalliProtocol::Load()
{
    Framework* framework = owner->GetFramework();

    // Reflect --loglevel to kNet
    if (framework->HasCommandLineParameter("--loglevel") || framework->HasCommandLineParameter("--loglevelnetwork"))
    {
        LogLevel level = framework->Console()->CurrentLogLevel();
        if (framework->HasCommandLineParameter("--loglevelnetwork"))
        {
            // --loglevelnetwork overrides --loglevel.
            StringVector llNetwork = framework->CommandLineParameters("--loglevelnetwork");
            level = (!llNetwork.Empty() ? ConsoleAPI::LogLevelFromString(llNetwork.Front()) : level);
        }
        if (level == LogLevelNone || level == LogLevelWarning || level == LogLevelError)
            kNet::SetLogChannels(kNet::LogError);
        else if (level == LogLevelInfo)
            kNet::SetLogChannels(kNet::LogError | kNet::LogUser);
        else if (level == LogLevelDebug)
            kNet::SetLogChannels(kNet::LogInfo | kNet::LogError | kNet::LogUser | kNet::LogVerbose); 
        else
            kNet::SetLogChannels(kNet::LogInfo | kNet::LogError | kNet::LogUser); 
    }
    // Enable all non verbose channels.
    else
        kNet::SetLogChannels(kNet::LogInfo | kNet::LogError | kNet::LogUser);

    /* There seems to be no quiet mode for kNet logging, set to errors only.
       This needs to be set explicitly as kNet does not log via Urho, which
       will automatically suppress anything below LogLevelError. */
    if (framework->HasCommandLineParameter("--quiet"))
        kNet::SetLogChannels(kNet::LogError);
}

void KristalliProtocol::Initialize()
{
    Framework* framework = owner->GetFramework();

    defaultTransport = kNet::SocketOverUDP;
    StringVector cmdLineParams = framework->CommandLineParameters("--protocol");
    if (cmdLineParams.Size() > 0)
    {
        kNet::SocketTransportLayer transportLayer = kNet::StringToSocketTransportLayer(cmdLineParams.Front().Trimmed().CString());
        if (transportLayer != kNet::InvalidTransportLayer)
            defaultTransport = transportLayer;
    }
}

void KristalliProtocol::Uninitialize()
{
    Disconnect();
}

void KristalliProtocol::OpenKNetLogWindow()
{
    LogError("Cannot open kNet logging window - kNet was not built with Qt enabled!");
}

void KristalliProtocol::Update(float /*frametime*/)
{
    // Pulls all new inbound network messages and calls the message handler we've registered
    // for each of them.
    if (serverConnection)
    {
        URHO3D_PROFILE(KristalliProtocolModule_kNet_client_Process);
        serverConnection->Process();
    }

    // Note: Calling the above serverConnection->Process() may set serverConnection to null if the connection gets disconnected.
    // Therefore, in the code below, we cannot assume serverConnection is non-null, and must check it again.

    // Our client->server connection is never kept half-open.
    // That is, at the moment the server write-closes the connection, we also write-close the connection.
    // Check here if the server has write-closed, and also write-close our end if so.
    if (serverConnection && !serverConnection->IsReadOpen() && serverConnection->IsWriteOpen())
        serverConnection->Disconnect(0);
    
    // Process server incoming connections & messages if server up
    if (server)
    {
        URHO3D_PROFILE(KristalliProtocolModule_kNet_server_Process);

        server->Process();

        // In Tundra, we *never* keep half-open server->client connections alive. 
        // (the usual case would be to wait for a file transfer to complete, but Tundra messaging mechanism doesn't use that).
        // So, bidirectionally close all half-open connections.
        kNet::NetworkServer::ConnectionMap connections = server->GetConnections();
        for(kNet::NetworkServer::ConnectionMap::iterator iter = connections.begin(); iter != connections.end(); ++iter)
            if (!iter->second->IsReadOpen() && iter->second->IsWriteOpen())
                iter->second->Disconnect(0);
    }
    
    if ((!serverConnection || serverConnection->GetConnectionState() == kNet::ConnectionClosed ||
        serverConnection->GetConnectionState() == kNet::ConnectionPending) && serverIp.length() != 0)
    {
        const float cReconnectTimeout = 5 * 1000.0f;
        if (reconnectTimer.Test())
        {
            if (reconnectAttempts)
            {
                PerformConnection();
                --reconnectAttempts;
            }
            else
            {
                LogInfo("Failed to connect to " + String(serverIp.c_str()) + String(":") + String(serverPort));
               
                ConnectionAttemptFailed.Emit();

                reconnectTimer.Stop();
                serverIp = "";
            }
        }
        else if (!reconnectTimer.Enabled())
            reconnectTimer.StartMSecs(cReconnectTimeout);
    }

    // If connection was made, enable a larger number of reconnection attempts in case it gets lost
    if (serverConnection && serverConnection->GetConnectionState() == kNet::ConnectionOK)
        reconnectAttempts = cReconnectAttempts;
}

void KristalliProtocol::Connect(const char *ip, unsigned short port, kNet::SocketTransportLayer transport)
{
    if (Connected() && serverConnection->RemoteEndPoint().IPToString() != serverIp)
        Disconnect();
    
    serverIp = ip;
    serverPort = port;
    serverTransport = transport;
    reconnectAttempts = cInitialAttempts; // Initial attempts when establishing connection
    
    if (!Connected())
        PerformConnection(); // Start performing a connection attempt to the desired address/port/transport
}

void KristalliProtocol::PerformConnection()
{
    if (serverConnection) // Make sure connection is fully closed before doing a potential reconnection.
        serverConnection->Close();

    // Connect to the server.
    serverConnection = network.Connect(serverIp.c_str(), serverPort, serverTransport, this);
    if (!serverConnection)
    {
        LogInfo("Unable to connect to " + String(serverIp.c_str()) + String(":") + String(serverPort));
        return;
    }

    if (serverTransport == kNet::SocketOverUDP)
        static_cast<kNet::UDPMessageConnection*>(serverConnection.ptr())->SetDatagramSendRate(500);

    // For TCP mode sockets, set the TCP_NODELAY option to improve latency for the messages we send.
    if (serverConnection->GetSocket() && serverConnection->GetSocket()->TransportLayer() == kNet::SocketOverTCP)
        serverConnection->GetSocket()->SetNaglesAlgorithmEnabled(false);
}

void KristalliProtocol::Disconnect()
{
    // Clear the remembered destination server ip address so that the automatic connection timer will not try to reconnect.
    serverIp = "";
    reconnectTimer.Stop();
    
    if (Connected())
        serverConnection->Disconnect();
}

bool KristalliProtocol::StartServer(unsigned short port, kNet::SocketTransportLayer transport)
{
    StopServer();
    
    const bool allowAddressReuse = true;
    server = network.StartServer(port, transport, this, allowAddressReuse);
    if (!server)
    {
        const String error = "Failed to start server on port " + String(port) + ".";
        LogError(error);
        LogError("Please make sure that the port is free and not used by another application.");
        return false;
    }

    Framework* framework = owner->GetFramework();
    
    std::cout << std::endl;
    LogInfo("Server started");
    LogInfo("* Port     : " + String(port));
    LogInfo("* Protocol : " + SocketTransportLayerToString(transport));
    LogInfo("* Headless : " + String(framework->IsHeadless()));
    return true;
}

void KristalliProtocol::StopServer()
{
    if (server)
    {
        network.StopServer();
        // We may have connections registered by other server modules. Only clear native connections
        for(auto iter = connections.Begin(); iter != connections.End();)
        {
            if (dynamic_cast<KNetUserConnection*>(iter->Get()))
                iter = connections.Erase(iter);
            else
                ++iter;
        }
        LogInfo("Server stopped");
        server = 0;
    }
}

void KristalliProtocol::NewConnectionEstablished(kNet::MessageConnection *source)
{
    assert(source);
    if (!source)
        return;

    if (dynamic_cast<kNet::UDPMessageConnection*>(source))
        static_cast<kNet::UDPMessageConnection*>(source)->SetDatagramSendRate(500);

    source->RegisterInboundMessageHandler(this);
    
    UserConnectionPtr connection = UserConnectionPtr(new KNetUserConnection(context_));
    connection->userID = AllocateNewConnectionID();
    Urho3D::StaticCast<KNetUserConnection>(connection)->connection = source;
    connections.Push(connection);

    // For TCP mode sockets, set the TCP_NODELAY option to improve latency for the messages we send.
    if (source->GetSocket() && source->GetSocket()->TransportLayer() == kNet::SocketOverTCP)
        source->GetSocket()->SetNaglesAlgorithmEnabled(false);

    LogInfo(String("User connected from ") + String(source->RemoteEndPoint().ToString().c_str()) + String(", connection ID ") + String(connection->userID));

    ClientConnectedEvent.Emit(connection.Get());
}

void KristalliProtocol::ClientDisconnected(kNet::MessageConnection *source)
{
    // Delete from connection list if it was a known user
    for(auto iter = connections.Begin(); iter != connections.End(); ++iter)
    {
        if ((*iter)->ConnectionType() == "knet" && Urho3D::StaticCast<KNetUserConnection>(*iter)->connection == source)
        {
            ClientDisconnectedEvent.Emit(iter->Get());
            
            LogInfo("User disconnected, connection ID " + String((*iter)->userID));
            connections.Erase(iter);
            return;
        }
    }
    LogInfo("Unknown user disconnected");
}

bool KristalliProtocol::Connected() const
{
    return serverConnection != 0 && serverConnection->Connected();
}

void KristalliProtocol::HandleMessage(kNet::MessageConnection *source, kNet::packet_id_t packetId, kNet::message_id_t messageId, const char *data, size_t numBytes)
{
    assert(source);
    assert(data || numBytes == 0);

    try
    {
        NetworkMessageReceived.Emit(source, packetId, messageId, data, numBytes);
    }
    catch(std::exception &e)
    {
        LogError("KristalliProtocolModule: Exception \"" + String(e.what()) + "\" thrown when handling network message id " +
            String(messageId) + " size " + String(numBytes) + " from client " + source->ToString().c_str());

        // Kill the connection. For debugging purposes, don't disconnect the client if the server is running a debug build.
#ifndef _DEBUG
        source->Disconnect(0);
        source->Close(0);
        // kNet will call back to KristalliProtocolModule::ClientDisconnected() to clean up the high-level Tundra UserConnection object.
#endif
    }
}

u32 KristalliProtocol::AllocateNewConnectionID() const
{
    u32 newID = 1;
    for(auto iter = connections.Begin(); iter != connections.End(); ++iter)
        newID = Max(newID, (*iter)->userID+1);
    
    return newID;
}

UserConnectionPtr KristalliProtocol::UserConnectionBySource(kNet::MessageConnection* source) const
{
    for(auto iter = connections.Begin(); iter != connections.End(); ++iter)
        if ((*iter)->ConnectionType() == "knet" && Urho3D::StaticCast<KNetUserConnection>(*iter)->connection == source)
            return *iter;

    return UserConnectionPtr();
}

UserConnectionPtr KristalliProtocol::UserConnectionById(u32 id) const
{
    for(auto iter = connections.Begin(); iter != connections.End(); ++iter)
        if ((*iter)->userID == id)
            return *iter;

    return UserConnectionPtr();
}

}
