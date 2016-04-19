// For conditions of distribution and use, see copyright notice in LICENSE

#include "WebSocketServer.h"
#include "WebSocketUserConnection.h"
#include "TundraLogic.h"
#include "Framework.h"
#include "CoreDefines.h"
#include "CoreStringUtils.h"
#include "LoggingFunctions.h"
#include "UniqueIdGenerator.h"
#include "TundraMessages.h"
#include "Server.h"

#include "kNet/DataDeserializer.h"

#include <websocketpp/frame.hpp>

#include "AssetAPI.h"
#include "IAssetStorage.h"
#include "IAsset.h"
#include "JSON.h"

#include <algorithm>

#ifdef WIN32
#include "Win.h"
#else
#include <sys/stat.h>
#include <utime.h>
#endif

#include <Urho3D/Core/StringUtils.h>

#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif

using namespace Tundra;

namespace WebSocket
{
// ServerThread


void ServerThread::ThreadFunction()
{
    if (!server_)
        return;
    
    try
    {
        server_->run();
    } 
    catch (const std::exception & e) 
    {
        LogError("Exception while running websocket server: " + String(e.what()));
    } 
    catch (websocketpp::lib::error_code e) 
    {
        LogError("Exception while running websocket server: " + String(e.message().c_str()));
    } 
    catch (...) 
    {
        LogError("Exception while running websocket server: other exception");
    }
}

// Server

Server::Server(Framework *framework) :
    Object(framework->GetContext()),
    LC("[WebSocketServer]: "),
    framework_(framework),
    port_(2345)
{
    // Port
    StringList portParam = framework->CommandLineParameters("--port");
    if (!portParam.Empty())
    {
        port_ = ToUInt(portParam.Front());
        if (!port_)
        {
            port_ = 2345;
            LogWarning(LC + "Failed to parse int from --port, using default port 2345.");
        }
    }
}

Server::~Server()
{
    Reset();
}

void Server::Update(float frametime)
{
    TundraLogic* tundraLogic = framework_->Module<TundraLogic>();
    ::Server* tundraServer = tundraLogic->Server();

    // Clean dead requestedConnections
    if (!connections_.Empty())
    {
        WebSocket::UserConnectionList::Iterator cleanupIter = connections_.Begin();
        while (cleanupIter != connections_.End())
        {
            WebSocket::UserConnectionPtr connection = *cleanupIter;
            if (!connection)
            {
                cleanupIter = connections_.Erase(cleanupIter);
            }
            else if (connection->webSocketConnection.expired())
            {
                // If user was already registered to the Tundra server, remove from there
                tundraServer->RemoveExternalUser(Urho3D::StaticCast<::UserConnection>(connection));
                if (!connection->userID)
                    LogDebug(LC + "Removing non-logged in WebSocket connection.");
                else
                    LogInfo(LC + "Removing expired WebSocket connection with ID " + String(connection->userID));
                cleanupIter = connections_.Erase(cleanupIter);
            }
            else
            {
                ++cleanupIter;
            }
        }
    }
    
    Vector<SocketEvent*> processEvents;
    {
        Urho3D::MutexLock lockEvents(mutexEvents_);
        if (events_.Size() == 0)
            return;
        // Make copy of current event queue for processing
        processEvents = events_;
        events_.Clear();
    }

    Vector<UserConnectionPtr> toDisconnect;

    // Process events pushed from the websocket thread(s)
    for (uint i=0; i < processEvents.Size(); ++i)
    {
        SocketEvent *event = processEvents[i];
        if (!event)
            continue;

        // User connected
        if (event->type == SocketEvent::Connected)
        {
            if (!UserConnection(event->connection))
            {
                WebSocket::UserConnectionPtr userConnection(new WebSocket::UserConnection(context_, event->connection));
                connections_.Push(userConnection);

                // The connection does not yet have an ID assigned. Tundra server will assign on login
                LogDebug(LC + String("New WebSocket connection."));
            }
        }
        // User disconnected
        else if (event->type == SocketEvent::Disconnected)
        {
            for(UserConnectionList::Iterator iter = connections_.Begin(); iter != connections_.End(); ++iter)
            {
                if ((*iter) && (*iter)->WebSocketConnection() == event->connection)
                {
                    tundraServer->RemoveExternalUser(Urho3D::StaticCast<::UserConnection>(*iter));
                    if (!(*iter)->userID)
                        LogDebug(LC + "Removing non-logged in WebSocket connection.");
                    else
                        LogInfo(LC + "Removing WebSocket connection with ID " + String((*iter)->userID));
                    connections_.Erase(iter);
                    break;
                }
            }
        }
        // Data message
        else if (event->type == SocketEvent::Data && event->data.get())
        {
            WebSocket::UserConnectionPtr userConnection = UserConnection(event->connection);
            if (userConnection)
            {
                kNet::DataDeserializer dd(event->data->GetData(), event->data->BytesFilled());
                u16 messageId = dd.Read<u16>();

                // LoginMessage
                if (messageId == cLoginMessage)
                {
                    bool ok = false;
                    String loginDataString = ReadUtf8String(dd);
                    // Read optional protocol version
                    if (dd.BytesLeft())
                        userConnection->protocolVersion = (NetworkProtocolVersion)dd.ReadVLE<kNet::VLE8_16_32>();

                    JSONValue json;
                    ok = json.FromString(loginDataString);
                    if (ok)
                    {
                        JSONObject jsonObj = json.GetObject();
                        for (JSONObject::ConstIterator i = jsonObj.Begin(); i != jsonObj.End(); ++i)
                            userConnection->properties[i->first_] = i->second_.ToVariant();
                        userConnection->properties["authenticated"] = true;
                        bool success = tundraServer->AddExternalUser(Urho3D::StaticCast<::UserConnection>(userConnection));
                        if (!success)
                        {
                            LogInfo(LC + "Connection ID " + String(userConnection->userID) + " login refused");
                            toDisconnect.Push(userConnection);
                        }
                        else
                            LogInfo(LC + "Connection ID " + String(userConnection->userID) + " login successful");
                    }
                }
                else
                {
                    // Only signal messages from authenticated users
                    if (userConnection->properties["authenticated"].GetBool() == true)
                    {
                        // Signal network message. As per kNet tradition the message ID is given separately in addition with the rest of the data
                        NetworkMessageReceived.Emit(userConnection.Get(), messageId, event->data->GetData() + sizeof(u16), event->data->BytesFilled() - sizeof(u16));
                        // Signal network message on the Tundra server so that it can be globally picked up
                        tundraServer->EmitNetworkMessageReceived(userConnection.Get(), 0, messageId, event->data->GetData() + sizeof(u16), event->data->BytesFilled() - sizeof(u16));
                    }
                }
            }
            else
                LogError(LC + "Received message from unauthorized connection, ignoring.");

            event->data.reset();
        }
        else
            event->data.reset();
        
        SAFE_DELETE(event);
    }

    for (uint i = 0; i < toDisconnect.Size(); ++i)
    {
        if (toDisconnect[i])
            toDisconnect[i]->Disconnect();
    }
}

WebSocket::UserConnectionPtr Server::UserConnection(uint connectionId)
{
    for(UserConnectionList::Iterator iter = connections_.Begin(); iter != connections_.End(); ++iter)
        if ((*iter)->userID == connectionId)
            return (*iter);

    return WebSocket::UserConnectionPtr();
}

WebSocket::UserConnectionPtr Server::UserConnection(ConnectionPtr connection)
{
    if (!connection.get())
        return WebSocket::UserConnectionPtr();

    for(UserConnectionList::Iterator iter = connections_.Begin(); iter != connections_.End(); ++iter)
        if ((*iter)->WebSocketConnection().get() == connection.get())
            return (*iter);
    return WebSocket::UserConnectionPtr();
}

bool Server::Start()
{
    Reset();
    
    try
    {
        server_ = WebSocket::ServerPtr(new websocketpp::server<websocketpp::config::asio>());

        // Initialize ASIO transport
        server_->init_asio();

        // Register handler callbacks
        server_->set_open_handler(boost::bind(&Server::OnConnected, this, ::_1));
        server_->set_close_handler(boost::bind(&Server::OnDisconnected, this, ::_1));
        server_->set_message_handler(boost::bind(&Server::OnMessage, this, ::_1, ::_2));
        server_->set_socket_init_handler(boost::bind(&Server::OnSocketInit, this, ::_1, ::_2));

        // Setup logging
        server_->get_alog().clear_channels(websocketpp::log::alevel::all);
        server_->get_elog().clear_channels(websocketpp::log::elevel::all);
        server_->get_elog().set_channels(websocketpp::log::elevel::rerror);
        server_->get_elog().set_channels(websocketpp::log::elevel::fatal);

        server_->listen(boost::asio::ip::tcp::v4(), port_);

        // Start the server accept loop
        server_->start_accept();

        // Start the server polling thread
        thread_.server_ = server_;
        thread_.Run();

    } 
    catch (std::exception &e) 
    {
        LogError(LC + String(e.what()));
        return false;
    }
    
    LogInfo(LC + "WebSocket server started to port " + String(port_));

    ServerStarted.Emit();
    
    return true;
}

void Server::Stop()
{    
    try
    {
        if (server_)
        {
            server_->stop();
            thread_.Stop();
            ServerStopped.Emit();
        }
    }
    catch (std::exception &e) 
    {
        LogError(LC + "Error while closing server: " + String(e.what()));
        return;
    }
    
    LogDebug(LC + "Stopped"); 
    
    Reset();
}

void Server::Reset()
{
    connections_.Clear();

    server_.reset();
}

void Server::OnConnected(ConnectionHandle connection)
{
    Urho3D::MutexLock lock(mutexEvents_);

    ConnectionPtr connectionPtr = server_->get_con_from_hdl(connection);

    // Find existing events and remove them if we got duplicates 
    // that were not synced to main thread yet.
    Vector<SocketEvent*> removeItems;
    for (uint i = 0; i < events_.Size(); ++i)
    {
        // Remove any and all messages from this connection, 
        // if there are any data messages for this connection.
        // Which is not possible in theory.
        SocketEvent *existing = events_[i];
        if (existing->connection == connectionPtr)
            removeItems.Push(existing);
    }
    if (!removeItems.Empty())
    {
        for (uint i = 0; i < removeItems.Size(); ++i)
            events_.Remove(removeItems[i]);
    }

    events_.Push(new SocketEvent(connectionPtr, SocketEvent::Connected));
}

void Server::OnDisconnected(ConnectionHandle connection)
{
    Urho3D::MutexLock lock(mutexEvents_);

    ConnectionPtr connectionPtr = server_->get_con_from_hdl(connection);

    // Find existing events and remove them if we got duplicates 
    // that were not synced to main thread yet.
    Vector<SocketEvent*> removeItems;
    for (uint i = 0; i < events_.Size(); ++i)
    {
        // Remove any and all messages from this connection,
        // as it's disconnecting
        SocketEvent *existing = events_[i];
        if (existing->connection == connectionPtr)
            removeItems.Push(existing);
    }
    if (!removeItems.Empty())
    {
        for (uint i = 0; i < removeItems.Size(); ++i)
            events_.Remove(removeItems[i]);
    }

    events_.Push(new SocketEvent(connectionPtr, SocketEvent::Disconnected));
}

void Server::OnMessage(ConnectionHandle connection, MessagePtr data)
{   
    Urho3D::MutexLock lock(mutexEvents_);

    ConnectionPtr connectionPtr = server_->get_con_from_hdl(connection);

    if (data->get_opcode() == websocketpp::frame::opcode::TEXT)
    {
        String textMsg(data->get_payload().c_str());
        LogInfo("[WebSocketServer]: on_utf8_message: size=" + String(textMsg.Length()) + " msg=" + textMsg);
    }
    else if (data->get_opcode() == websocketpp::frame::opcode::BINARY)
    {
        const std::string &payload = data->get_payload();
        if (payload.size() == 0)
        {
            LogError("[WebSocketServer]: Received 0 sized payload, ignoring");
            return;
        }
        SocketEvent *event = new SocketEvent(connectionPtr, SocketEvent::Data);
        event->data = DataSerializerPtr(new kNet::DataSerializer(payload.size()));
        event->data->AddAlignedByteArray(&payload[0], payload.size());

        events_.Push(event);
    }
}

/// \todo Implement actual registering of http handlers, for now disabled
/*
void Server::OnHttpRequest(WebSocket::ConnectionHandle connection)
{
    QByteArray resourcePath = QString::fromStdString(connection->get_resource()).toUtf8();

    qDebug() << "OnHttpRequest" << resourcePath;
        
    QByteArray data("Hello World to " + resourcePath);
    std::string payload;
    payload.resize(data.size());
    memcpy((void*)payload.data(), (void*)data.data(), data.size());
    
    connection->set_status(websocketpp::http::status_code::ok);
    connection->set_body(payload);
    connection->replace_header("Content-Length", QString::number(data.size()).toStdString());
}
*/

void Server::OnSocketInit(ConnectionHandle connection, boost::asio::ip::tcp::socket& s)
{
    // Disable Nagle's algorithm from each connection to avoid delays in sync
    boost::asio::ip::tcp::no_delay option(true);
    s.set_option(option);
}

}
