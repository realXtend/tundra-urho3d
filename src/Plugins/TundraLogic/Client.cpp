// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "Client.h"
#include "UserConnectedResponseData.h"
#include "TundraLogic.h"
#include "KristalliProtocol.h"
#include "SyncManager.h"
#include "SyncState.h"
#include "MsgLogin.h"
#include "MsgLoginReply.h"
#include "Framework.h"
#include "Scene.h"
#include "SceneAPI.h"
#include "AssetAPI.h"
#include "LoggingFunctions.h"
#include "TundraLogicUtils.h"
#include "TundraMessages.h"

#include "Placeable.h"
#include "UrhoRenderer.h"

#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Resource/XMLElement.h>
#include <Urho3D/Core/StringUtils.h>

using namespace kNet;

namespace Tundra
{

Client::Client(TundraLogic* owner) : 
    Object(owner->GetContext()),
    owner_(owner),
    framework_(owner->GetFramework()),
    loginstate_(NotConnected),
    reconnect_(false),
    client_id_(0)
{
    // Create "virtual" client->server connection & syncstate. Used by SyncManager
    serverUserConnection_ = KNetUserConnectionPtr(new KNetUserConnection(context_));
    serverUserConnection_->properties["authenticated"] = true;
    serverUserConnection_->syncState = SharedPtr<SceneSyncState>(new SceneSyncState(0, false));
}

Client::~Client()
{
}

void Client::Update(float)
{
    // If we aren't a server, check pending login
    if (!owner_->IsServer())
        CheckLogin();
}

void Client::Login(const String& address, unsigned short port, const String& username, const String& password, const String &protocol)
{
    if (IsConnected())
        DoLogout();

    // Set properties that the "lower" overload wont be adding.
    SetLoginProperty("username", username);
    SetLoginProperty("password", password);

    String p = protocol.Trimmed().ToLower();
    kNet::SocketTransportLayer transportLayer = kNet::StringToSocketTransportLayer(p.CString());
    if (transportLayer == kNet::InvalidTransportLayer && !p.Empty())
    {
        LogError("Client::Login: Cannot log to server using unrecognized protocol: " + p);
        return;
    }
    Login(address, port, transportLayer);
}

void Client::Login(const String& address, unsigned short port, kNet::SocketTransportLayer protocol)
{
    if (owner_->IsServer())
    {
        LogError("Already running a server, cannot login to a world as a client");
        return;
    }

    reconnect_ = false;
    
    KristalliProtocol *kristalli = owner_->KristalliProtocol();

    if (protocol == kNet::InvalidTransportLayer)
    {
        LogInfo("Client::Login: No protocol specified, using the default value.");
        protocol = kristalli->defaultTransport;
    }
    // Set all login properties we have knowledge of. 
    // Others may have been added before calling this function.
    SetLoginProperty("protocol", String(SocketTransportLayerToString(protocol).c_str()).ToLower());
    SetLoginProperty("address", address);
    SetLoginProperty("port", port);
    SetLoginProperty("client-version", "2.5.4.1");
    SetLoginProperty("client-name", "Name");
    SetLoginProperty("client-organization", "Org");

    kristalli->NetworkMessageReceived.Connect(this, &Client::HandleKristalliMessage);

    kristalli->ConnectionAttemptFailed.Connect(this, &Client::OnConnectionAttemptFailed);

    kristalli->Connect(address.CString(), port, protocol);
    loginstate_ = ConnectionPending;
    client_id_ = 0;
}

void Client::Logout()
{
    //QTimer::singleShot(1, this, SLOT(DelayedLogout()));
    DelayedLogout();
}

void Client::DelayedLogout()
{
    DoLogout(false);
}

void Client::DoLogout(bool fail)
{
    if (loginstate_ != NotConnected)
    {
        if (MessageConnection())
        {
            owner_->KristalliProtocol()->Disconnect();
            LogInfo("Disconnected");
        }
        
        serverUserConnection_->connection = 0;
        loginstate_ = NotConnected;
        client_id_ = 0;
        
        framework_->Scene()->RemoveScene("TundraClient");
        framework_->Asset()->ForgetAllAssets();
        
        Disconnected.Emit();
    }
    
    if (fail)
    {
        String failreason = LoginProperty("LoginFailed").GetString();
        LoginFailed.Emit(failreason);
    }
    else // An user deliberately disconnected from the world, and not due to a connection error.
    {
        // Clear all the login properties we used for this session, so that the next login session will start from an
        // empty set of login properties (just-in-case).
        properties_.Clear();
    }

    SharedPtr<KristalliProtocol> kristalli = owner_->KristalliProtocol();
    kristalli->NetworkMessageReceived.Disconnect(this, &Client::HandleKristalliMessage);

    kristalli->ConnectionAttemptFailed.Disconnect(this, &Client::OnConnectionAttemptFailed);

    LogInfo("Client logged out.");
}

bool Client::IsConnected() const
{
    return loginstate_ == LoggedIn;
}

void Client::SetLoginProperty(String key, Urho3D::Variant value)
{
    key = key.Trimmed();
    if (value.IsEmpty())
        properties_.Erase(properties_.Find(key));
    else
        properties_[key] = value;
}

Urho3D::Variant Client::LoginProperty(String key) const
{
    key = key.Trimmed();
    auto i = properties_.Find(key);
    if (i != properties_.End())
        return i.ptr_;
    else
        return Urho3D::Variant();
}

bool Client::HasLoginProperty(String key) const
{
    auto i = properties_.Find(key);
    if (i == properties_.End())
        return false;
    else
        return !i->second_.IsEmpty();
}

String Client::LoginPropertiesAsXml() const
{
    Urho3D::XMLFile xml(owner_->GetContext());
    Urho3D::XMLElement rootElem = xml.CreateRoot("login");
    for(auto iter = properties_.Begin(); iter != properties_.End(); ++iter)
    {
        Urho3D::XMLElement element = rootElem.CreateChild(iter->first_);
        element.SetAttribute("value", iter->second_.ToString());
    }

    return xml.ToString();
}

void Client::CheckLogin()
{
    kNet::MessageConnection* connection = MessageConnection();
    switch(loginstate_)
    {
    case ConnectionPending:
        if (connection && connection->GetConnectionState() == kNet::ConnectionOK)
        {
            loginstate_ = ConnectionEstablished;
            MsgLogin msg;
            AboutToConnect.Emit(); // This signal is used as a 'function call'. Any interested party can fill in

            // new content to the login properties of the client object, which will then be sent out on the line below.
            msg.loginData = StringToBuffer(LoginPropertiesAsXml());
            DataSerializer ds(msg.Size() + 4);
            msg.SerializeTo(ds);
            // Add requested protocol version
            ds.AddVLE<kNet::VLE8_16_32>(cHighestSupportedProtocolVersion);
            connection->SendMessage(msg.messageID, msg.reliable, msg.inOrder, msg.priority, 0, ds.GetData(), ds.BytesFilled());
        }
        break;
    case LoggedIn:
        // If we have logged in, but connection dropped, prepare to resend login
        if (!connection || connection->GetConnectionState() != kNet::ConnectionOK)
            loginstate_ = ConnectionPending;
        break;
    }
}

kNet::MessageConnection* Client::MessageConnection()
{
    return owner_->KristalliProtocol()->MessageConnection();
}

KNetUserConnectionPtr Client::ServerUserConnection() const
{
    return serverUserConnection_;
}

void Client::OnConnectionAttemptFailed()
{
    // Provide a reason why the connection failed.
    String address = LoginProperty("address").GetString();
    String port = LoginProperty("port").GetString();
    String protocol = LoginProperty("protocol").GetString();

    String failReason = "Could not connect to host";
    if (!address.Empty())
    {
        failReason.Append(" " + address);
        if (!port.Empty())
            failReason.Append(":" + port);
        if (!protocol.Empty())
            failReason.Append(" with " + protocol.ToUpper());
    }

    SetLoginProperty("LoginFailed", failReason);
    DoLogout(true);
}

void Client::HandleKristalliMessage(kNet::MessageConnection* source, kNet::packet_id_t packetId,
        kNet::message_id_t messageId, const char* data, size_t numBytes)
{
    if (source != MessageConnection())
    {
        LogWarning("Client: dropping message " + String(messageId) + " from unknown source");
        return;
    }
    
    switch(messageId)
    {
    case MsgLoginReply::messageID:
        {
            HandleLoginReply(source, data, numBytes);
        }
        break;
    }
    
    NetworkMessageReceived.Emit(packetId, messageId, data, numBytes);
    // SyncManager uses this route to handle messages
    serverUserConnection_->EmitNetworkMessageReceived(packetId, messageId, data, numBytes);
}

void Client::HandleLoginReply(kNet::MessageConnection* /*source*/, const char *data, size_t numBytes)
{
    DataDeserializer dd(data, numBytes);
    MsgLoginReply msg;
    msg.DeserializeFrom(dd);

    // Read optional protocol version
    // Server can downgrade what we sent, but never upgrade
    serverUserConnection_->protocolVersion = ProtocolOriginal;
    if (dd.BytesLeft())
        serverUserConnection_->protocolVersion = (NetworkProtocolVersion)dd.ReadVLE<kNet::VLE8_16_32>();

    if (msg.success)
    {
        loginstate_ = LoggedIn;
        client_id_ = msg.userID;
        LogInfo("Logged in successfully");
        
        // Note: create scene & send info of login success only on first connection, not on reconnect
        if (!reconnect_)
        {
            // The connection is now live for use by eg. SyncManager
            serverUserConnection_->connection = MessageConnection();

            // Create a non-authoritative scene for the client
            ScenePtr scene = framework_->Scene()->CreateScene("TundraClient", true, false);

            owner_->SyncManager()->RegisterToScene(scene);
            
            UserConnectedResponseData responseData;
            if (msg.loginReplyData.size() > 0)
            {
                String responseDataStr((const char *)&msg.loginReplyData[0], (int)msg.loginReplyData.size());
                responseData.responseDataXml = new Urho3D::XMLFile(context_);
                responseData.responseDataXml->FromString(responseDataStr);
            }

            Connected.Emit(&responseData);
        }
        else
        {
            // If we are reconnecting, empty the scene, as the server will send everything again anyway
            // Note: when we move to unordered communication, we must guarantee that the server does not send
            // any scene data before the login reply

            ScenePtr scene = framework_->Scene()->SceneByName("TundraClient");
            if (scene)
                scene->RemoveAllEntities(true, AttributeChange::LocalOnly);
        }
        reconnect_ = true;
    }
    else
    {
        String response((const char *)&msg.loginReplyData[0], (int)msg.loginReplyData.size());
        if (!response.Empty())
            SetLoginProperty("LoginFailed", response);
        DoLogout(true);
    }
}

void Client::HandleClientJoined(kNet::MessageConnection* /*source*/, const MsgClientJoined& /*msg*/)
{
}

void Client::HandleClientLeft(kNet::MessageConnection* /*source*/, const MsgClientLeft& /*msg*/)
{
}

}