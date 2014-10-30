// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include <kNet/SharedPtr.h>
#include <kNet/MessageConnection.h>

#include "CoreTypes.h"
#include "TundraLogicApi.h"
#include "TundraLogicFwd.h"
#include "Signals.h"
#include "SyncState.h"

#include <Object.h>

namespace Tundra
{

class Entity;

/// Protocol versioning for client connections.
enum NetworkProtocolVersion
{
    ProtocolOriginal = 0x1,         // Original
    ProtocolCustomComponents = 0x2, // Adds support for transmitting new static-structured component types without actual C++ implementation, using EC_PlaceholderComponent
    ProtocolHierarchicScene = 0x3   // Adds support for hierarchic scene, ie. entities having child entities,
};

/// Highest supported protocol version in the build. Update this when a new protocol version is added
const NetworkProtocolVersion cHighestSupportedProtocolVersion = ProtocolHierarchicScene;

/// Represents a client connection on the server side. Subclassed by networking implementations.
class TUNDRALOGIC_API UserConnection : public Object, public std::tr1::enable_shared_from_this<UserConnection>
{
    OBJECT(UserConnection);
    //Q_PROPERTY(u32 id READ ConnectionId)
    //Q_PROPERTY(int protocolVersion READ ProtocolVersion)
    //Q_PROPERTY(QString connectionType READ ConnectionType)

public:
    UserConnection(Object* owner);

    /// Returns the connection ID.
    u32 ConnectionId() const { return userID; }
    /// Returns the protocol version.
    NetworkProtocolVersion ProtocolVersion() const { return protocolVersion; }
    /// Returns connection type.
    virtual String ConnectionType() const = 0;

    /// Connection ID
    u32 userID;
    /// Raw xml login data
    String loginData;
    /// Property map
    LoginPropertyMap properties;
    /// Scene sync state, created and used by the SyncManager
    SharedPtr<SceneSyncState> syncState;
    /// Network protocol version in use
    NetworkProtocolVersion protocolVersion;
    /// Map of the unacked entity IDs a user has sent, and the real entity IDs they have been assigned
    std::map<u32, u32> unackedIdsToRealIds;

    /// Queue a network message to be sent to the client. All implementations may not use the reliable, inOrder, priority and contentID parameters.
    virtual void Send(kNet::message_id_t id, const char* data, size_t numBytes, bool reliable, bool inOrder, unsigned long priority = 100, unsigned long contentID = 0) = 0;

    /// Queue a network message to be sent to the client, with the data to be sent in a DataSerializer. All implementations may not use the reliable, inOrder, priority and contentID parameters.
    void Send(kNet::message_id_t id, bool reliable, bool inOrder, kNet::DataSerializer& ds, unsigned long priority = 100, unsigned long contentID = 0);

    /// Queue a typed network message to be sent to the client.
    template<typename SerializableMessage> void Send(const SerializableMessage &data)
    {
        kNet::DataSerializer ds(data.Size());
        data.SerializeTo(ds);
        Send(SerializableMessage::messageID, data.reliable, data.inOrder, ds);
    }

    /// Trigger a network message signal. Called by the networking implementation.
    void EmitNetworkMessageReceived(kNet::packet_id_t packetId, kNet::message_id_t messageId, const char* data, size_t numBytes);

    /// Execute an action on an entity, sent only to the specific user
    void Exec(Entity *entity, const String &action, const StringList &params);
    void Exec(Entity *entity, const String &action, const String &p1 = "", const String &p2 = "", const String &p3 = "");  /**< @overload */

    /// Returns raw login data
    String LoginData() const { return loginData; }

    /// Sets a property
    void SetProperty(const String& key, const String& value);

    /// Returns a property
    Variant Property(const String& key) const;

    /// Check whether has a property
    bool HasProperty(const String& key) const;

    /// Returns all the login properties that were used to login to the server.
    LoginPropertyMap LoginProperties() const { return properties; }

    /// Deny connection. Call as a response to server.UserAboutToConnect() if necessary
    void DenyConnection(const String& reason);

    /// Starts a benign disconnect procedure (one which waits for the peer acknowledge procedure).
    virtual void Disconnect() = 0;

    /// Forcibly kills this connection without notifying the peer.
    virtual void Close() = 0;

    //u32 GetConnectionID() const { return ConnectionId(); }  /**< @deprecated Use ConnectionId or 'id' @todo Add warning print */
    //String GetLoginData() const { return LoginData(); }  /**< @deprecated Use LoginData @todo Add warning print */
    //String GetProperty(const QString& key) const { return Property(key).toString(); } /**< @deprecated Use Property @todo Add warning print */

    // signals:
    
    /// Emitted when action has been triggered for this specific user connection.
    //void ActionTriggered(UserConnection* connection, Entity* entity, const String& action, const StringList& params);
    Signal4<UserConnection*, Entity*, const String&, const StringList&> ActionTriggered;
    /// Emitted when the client has sent a network message. PacketId will be 0 if not supported by the networking implementation.
    //void NetworkMessageReceived(UserConnection* connection, kNet::packet_id_t packetId, kNet::message_id_t messageId, const char* data, size_t numBytes);
    Signal5<UserConnection*, kNet::packet_id_t, kNet::message_id_t, const char*, size_t> NetworkMessageReceived;
};

/// A kNet user connection.
class TUNDRALOGIC_API KNetUserConnection : public UserConnection
{
    OBJECT(KNetUserConnection);

public:
    KNetUserConnection(Object* owner);

    virtual String ConnectionType() const { return "knet"; }

    /// Message connection.
    Ptr(kNet::MessageConnection) connection;

    /// Queue a network message to be sent to the client. 
    virtual void Send(kNet::message_id_t id, const char* data, size_t numBytes, bool reliable, bool inOrder, unsigned long priority = 100, unsigned long contentID = 0);

    /// Starts a benign disconnect procedure (one which waits for the peer acknowledge procedure).
    virtual void Disconnect();

    /// Forcibly kills this connection without notifying the peer.
    virtual void Close();
};

}
