// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

#include "KristalliProtocol.h"
#include "Server.h"
#include "TundraLogic.h"
#include "TundraLogicUtils.h"
#include "SyncManager.h"
#include "TundraMessages.h"
#include "MsgLogin.h"
#include "MsgLoginReply.h"
#include "MsgClientLeft.h"
#include "MsgClientJoined.h"
#include "UserConnectedResponseData.h"
#include "Scene.h"

#include "Framework.h"
#include "CoreStringUtils.h"
#include "SceneAPI.h"
#include "ConfigAPI.h"
#include "LoggingFunctions.h"

#include <Urho3D/Resource/XMLFile.h>

using namespace kNet;

namespace Tundra
{

Server::Server(TundraLogic* owner) :
    Object(owner->GetContext()),
    owner_(owner),
    framework_(owner->GetFramework()),
    current_port_(-1)
{
}

Server::~Server()
{
}

void Server::Update(float /*frametime*/)
{
}

bool Server::Start(unsigned short port, String protocol)
{
    if (owner_->IsServer())
    {
        LogDebug("[SERVER] Trying to start server but it's already running.");
        return true; // Already started, don't need to do anything.
    }

    // By default should operate over UDP.
    const kNet::SocketTransportLayer defaultProtocol = owner_->KristalliProtocol()->defaultTransport;
    // Protocol is usually defined as a --protocol command line parameter or in config file,
    // but if it's given as a param to this function use it instead.
    if (protocol.Empty() && framework_->HasCommandLineParameter("--protocol"))
    {
        StringList cmdLineParams = framework_->CommandLineParameters("--protocol");
        if (cmdLineParams.Size() > 0)
            protocol = cmdLineParams[0];
        else
            LogError("--protocol specified without a parameter! Using " + SocketTransportLayerToString(defaultProtocol) + " protocol as default.");
    }

    kNet::SocketTransportLayer transportLayer = protocol.Empty() ? defaultProtocol : StringToSocketTransportLayer(protocol.Trimmed().CString());
    if (transportLayer == kNet::InvalidTransportLayer)
    {
        LogError("Invalid server protocol '" + protocol + "' specified! Using " +
            String(SocketTransportLayerToString(defaultProtocol).c_str()) + " protocol as default.");
        transportLayer = defaultProtocol;
    }

    // Start server
    if (!owner_->KristalliProtocol()->StartServer(port, transportLayer))
    {
        LogError("[SERVER] Failed to start server in port " + String(port));
        return false;
    }

    // Store current port and protocol
    current_port_ = (int)port;
    current_protocol_ = (transportLayer == kNet::SocketOverUDP) ? "udp" : "tcp";

    // Create the default server scene
    /// \todo Should be not hard coded like this. Give some unique id (uuid perhaps) that could be returned to the client to make the corresponding named scene in client?
    ScenePtr scene = framework_->Scene()->CreateScene("TundraServer", true, true);
//    framework_->Scene()->SetDefaultScene(scene);
    owner_->SyncManager()->RegisterToScene(scene);

    ServerStarted.Emit();

    KristalliProtocol *kristalli = owner_->KristalliProtocol();
    kristalli->NetworkMessageReceived.Connect(this, &Server::HandleKristalliMessage);
    kristalli->ClientDisconnectedEvent.Connect(this, &Server::HandleUserDisconnected);

    return true;
}

void Server::Stop()
{
    if (owner_->IsServer())
    {
        LogInfo("[SERVER] Stopped Tundra server. Removing TundraServer scene.");

        owner_->KristalliProtocol()->StopServer();
        framework_->Scene()->RemoveScene("TundraServer");
        
        ServerStopped.Emit();

        KristalliProtocol *kristalli = owner_->KristalliProtocol();
        kristalli->NetworkMessageReceived.Disconnect(this, &Server::HandleKristalliMessage);
        kristalli->ClientDisconnectedEvent.Disconnect(this, &Server::HandleUserDisconnected);
    }
}

bool Server::IsRunning() const
{
    return owner_->IsServer();
}

bool Server::IsAboutToStart() const
{
    return framework_->HasCommandLineParameter("--server");
}

int Server::Port() const
{
    return IsRunning() ? current_port_ : -1;
}

String Server::Protocol() const
{
    return IsRunning() ? current_protocol_ : "";
}

UserConnectionList Server::AuthenticatedUsers() const
{
    UserConnectionList ret;
    UserConnectionList& allUsers = UserConnections();
    foreach(const UserConnectionPtr &user, allUsers)
        if (user->properties["authenticated"].GetBool() == true)
            ret.Push(user);
    return ret;
}

UserConnectionPtr Server::UserConnectionById(u32 connectionID) const
{
    UserConnectionList connections = AuthenticatedUsers();
    foreach(const UserConnectionPtr &user, connections)
        if (user->userID == connectionID)
            return user;
    return UserConnectionPtr();
}

UserConnectionList& Server::UserConnections() const
{
    return owner_->KristalliProtocol()->UserConnections();
}

UserConnectionPtr Server::GetUserConnection(kNet::MessageConnection* source) const
{
    return owner_->KristalliProtocol()->UserConnectionBySource(source);
}

void Server::SetActionSender(UserConnection* user)
{
    actionSender = user;
}

UserConnectionPtr Server::ActionSender() const
{
    return actionSender.Lock();
}

kNet::NetworkServer *Server::GetServer() const
{
    return owner_->KristalliProtocol()->NetworkServer();
}

bool Server::AddExternalUser(UserConnectionPtr user)
{
    if (!user)
    {
        LogError("Null UserConnection passed to Server::AddExternalUser()");
        return false;
    }

    // Allocate user connection if not yet allocated
    if (user->userID == 0)
        user->userID = owner_->KristalliProtocol()->AllocateNewConnectionID();

    UserConnectionList& users = owner_->KristalliProtocol()->UserConnections();
    users.Push(user);

    // Try to login
    if (FinalizeLogin(user))
        return true;
    else
    {
        users.Remove(user);
        return false;
    }
}

void Server::RemoveExternalUser(UserConnectionPtr user)
{
    if (!user)
        return;

    UserConnectionList& users = owner_->KristalliProtocol()->UserConnections();

    // If user had zero ID, was not logged in yet and does not need to be reported
    if (user->userID)
    {
        // Tell everyone of the client leaving
        MsgClientLeft left;
        left.userID = user->userID;
        UserConnectionList connections = AuthenticatedUsers();
        foreach(const UserConnectionPtr &u, connections)
            if (u->userID != user->userID)
                u->Send(left);
    
       UserDisconnected.Emit(user->userID, user.Get());
    }

    users.Remove(user);
}


void Server::HandleKristalliMessage(kNet::MessageConnection* source, kNet::packet_id_t packetId, kNet::message_id_t messageId, const char* data, size_t numBytes)
{
    if (!source)
        return;

    if (!owner_->IsServer())
        return;

    UserConnectionPtr user = GetUserConnection(source);
    if (!user)
    {
        LogWarning("Server: dropping message " + String(messageId) + " from unknown connection " + String(source->ToString().c_str()) + ".");
        return;
    }

    // If we are server, only allow the login message from an unauthenticated user
    if (messageId != MsgLogin::messageID && user->properties["authenticated"].GetBool() != true)
    {
        LogWarning("Server: dropping message " + String(messageId) + " from unauthenticated user.");
        /// \todo something more severe, like disconnecting the user
        return;
    }
    else if (messageId == MsgLogin::messageID)
    {
        HandleLogin(source, data, numBytes);
    }

    EmitNetworkMessageReceived(user.Get(), packetId, messageId, data, numBytes);
}

void Server::EmitNetworkMessageReceived(UserConnection* user, kNet::packet_id_t packetId, kNet::message_id_t messageId, const char* data, size_t numBytes)
{
    if (user)
    {
        // Emit both global and user-specific message
        MessageReceived.Emit(user, packetId, messageId, data, numBytes);
        user->EmitNetworkMessageReceived(packetId, messageId, data, numBytes);
    }
    else
        LogWarning("Server::EmitNetworkMessageReceived: null UserConnection pointer");
}

void Server::HandleLogin(kNet::MessageConnection* source, const char* data, size_t numBytes)
{
    UserConnectionPtr user = GetUserConnection(source);
    if (!user)
    {
        LogWarning("[SERVER] Login message from unknown user");
        return;
    }

    DataDeserializer dd(data, numBytes);

    MsgLogin msg;
    // Read login data (all clients)
    msg.DeserializeFrom(dd);
    String loginData = BufferToString(msg.loginData);

    // Read optional protocol version
    if (dd.BytesLeft())
        user->protocolVersion = (NetworkProtocolVersion)dd.ReadVLE<kNet::VLE8_16_32>();
    
    Urho3D::XMLFile xml(context_);
    bool success = xml.FromString(loginData);
    if (!success)
        LogWarning("[SERVER] ID " + String(user->userID) + " client login data xml has malformed data");
    
    // Fill the user's logindata, both in raw format and as keyvalue pairs
    user->loginData = loginData;
    Urho3D::XMLElement rootElem = xml.GetRoot();
    Urho3D::XMLElement keyvalueElem = rootElem.GetChild();
    while(keyvalueElem)
    {
        user->SetProperty(keyvalueElem.GetName(), keyvalueElem.GetAttribute("value"));
        keyvalueElem = keyvalueElem.GetNext();
    }
    
    FinalizeLogin(user);
}

bool Server::FinalizeLogin(UserConnectionPtr user)
{
    String connectedUsername = user->Property("username").GetString();

    // Clamp version if not supported by server
    if (user->protocolVersion > cHighestSupportedProtocolVersion)
        user->protocolVersion = cHighestSupportedProtocolVersion;

    user->properties["authenticated"] = true;
    UserAboutToConnect.Emit(user->userID, user.Get());
    if (user->properties["authenticated"].GetBool() != true)
    {
        if (connectedUsername.Empty())
            LogInfo("[SERVER] ID " + String(user->userID) + " client was denied access");
        else
            LogInfo("[SERVER] ID "+ String(user->userID) + " client '" + connectedUsername + "' was denied access");
            
        MsgLoginReply reply;
        reply.success = 0;
        reply.userID = 0;
        std::vector<s8> responseByteData = StringToBuffer(user->properties["reason"].GetString());
        reply.loginReplyData.insert(reply.loginReplyData.end(), responseByteData.data(), responseByteData.data() + responseByteData.size());
        user->Send(reply);
        return false;
    }
    
    if (connectedUsername.Empty())
        LogInfo("[SERVER] ID " + String(user->userID) + " client connected");
    else
        LogInfo("[SERVER] ID " + String(user->userID) + " client '" + connectedUsername + "' connected");
    
    // Allow entityactions & EC sync from now on
    MsgLoginReply reply;
    reply.success = 1;
    reply.userID = user->userID;
    
    // Tell everyone of the client joining (also the user who joined)
    MsgClientJoined joined;
    joined.userID = user->userID;
    if (user->HasProperty("username"))
        joined.username = user->Property("username").GetString();
    
    UserConnectionList users = AuthenticatedUsers();
    foreach(const UserConnectionPtr &u, users)
        u->Send(joined);
    
    // Advertise the users who already are in the world, to the new user
    foreach(const UserConnectionPtr &u, users)
    {
        if (u->userID != user->userID)
        {
            MsgClientJoined joined2;
            joined2.userID = u->userID;
            if (u->HasProperty("username"))
                joined2.username = u->Property("username").GetString();
            user->Send(joined2);
        }
    }
    
    // Tell syncmanager of the new user
    owner_->SyncManager()->NewUserConnected(user);
    
    // Tell all server-side application code that a new user has successfully connected.
    // Ask them to fill the contents of a UserConnectedResponseData structure. This will
    // be sent to the client so that the scripts and applications on the client systemcd. can configure themselves.
    UserConnectedResponseData responseData;
    responseData.responseDataXml = new Urho3D::XMLFile(context_);
    UserConnected.Emit(user->userID, user.Get(), &responseData);

    bool isNativeConnection = dynamic_cast<KNetUserConnection*>(user.Get()) != 0;
    std::vector<s8> responseByteData;
    /// \todo Native connections still encode reply data to XML. Unify to JSON in the future.
    if (isNativeConnection)
        responseByteData = StringToBuffer(responseData.responseDataXml->ToString());
    else
        responseByteData = StringToBuffer(responseData.responseDataJson.ToString());

    reply.loginReplyData.insert(reply.loginReplyData.end(), responseByteData.data(), responseByteData.data() + responseByteData.size());

    // Send login reply, with protocol version accepted by the server appended
    DataSerializer ds(reply.Size() + 4 + 4);
    reply.SerializeTo(ds);
    ds.AddVLE<kNet::VLE8_16_32>(user->protocolVersion); 
    user->Send(reply.messageID, reply.reliable, reply.inOrder, ds, reply.priority);

    // Successful login
    return true;
}

void Server::HandleUserDisconnected(UserConnection* user)
{
    // Tell everyone of the client leaving
    MsgClientLeft left;
    left.userID = user->userID;
    UserConnectionList connections = AuthenticatedUsers();
    foreach(const UserConnectionPtr &u, connections)
        if (u->userID != user->userID)
            u->Send(left);

    UserDisconnected.Emit(user->userID, user);

    String username = user->Property("username").GetString();
    if (username.Empty())
        LogInfo("[SERVER] ID " + String(user->userID) + " client disconnected");
    else
        LogInfo("[SERVER] ID " + String(user->userID) + " client '" + username + "' disconnected");
}

}
