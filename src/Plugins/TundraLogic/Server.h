// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include <kNetFwd.h>

#include "TundraLogicApi.h"
#include "TundraLogicFwd.h"
#include "FrameworkFwd.h"

#include <Object.h>

namespace Tundra
{
/// Implements Tundra server functionality.
/// \todo Implement
class TUNDRALOGIC_API Server : public Object
{
    OBJECT(Server)
    

public:
    explicit Server(TundraLogic* owner);
    ~Server() {};

    /// Perform any per-frame processing
    void Update(float frametime) { };

    /// Get matching userconnection from a messageconnection, or null if unknown
    /// @todo Rename to UserConnection(ForMessageConnection) or similar.
    UserConnectionPtr GetUserConnection(kNet::MessageConnection* source) const { UNREFERENCED_PARAM(source); return UserConnectionPtr(); }

    /// Get all connected users
    UserConnectionList& UserConnections() const;

    /// Set current action sender. Called by SyncManager
    void SetActionSender(const UserConnectionPtr &user) { UNREFERENCED_PARAM(user); };

    /// Returns the backend server object.
    /** Use this object to Broadcast messages to all currently connected clients.
        @todo Rename to (KNet)NetworkServer or similar. */
    kNet::NetworkServer *GetServer() const;

    /// Returns server's port.
    /** @return Server port number, or -1 if server is not running. */
    int Port() const;

    /// Returns server's protocol.
    /** @return 'udp', tcp', or an empty string if server is not running. */
    String Protocol() const;

    /// Add a user from external networking subsystem such as WebSocket. Return true if the user was successfully authenticated according to the login properties.
    bool AddExternalUser(UserConnectionPtr user);
    /// Remove a user from external networking subsystem
    void RemoveExternalUser(UserConnectionPtr user);
    /// Emit network message received on behalf of an external networking subsystem. Packet id should be left zero if not supported.
    void EmitNetworkMessageReceived(UserConnection *connection, kNet::packet_id_t, kNet::message_id_t id, const char* data, size_t numBytes);

    // slots:

    /// Create server scene & start server
    /** @param protocol The server protocol to use, either "tcp" or "udp". If not specified, the default UDP will be used.
        @return True if successful, false otherwise. No scene will be created if starting the server fails. */
    bool Start(unsigned short port, String protocol = "");

    /// Stop server & delete server scene
    void Stop();

    /// Returns whether server is running
    bool IsRunning() const;

    /// Returns whether server is about to start.
    bool IsAboutToStart() const;

    /// Returns all authenticated users.
    UserConnectionList AuthenticatedUsers() const { return UserConnectionList(); }

    /// Returns connection corresponding to a connection ID.
    UserConnectionPtr UserConnectionById(u32 connectionID) const { UNREFERENCED_PARAM(connectionID); return UserConnectionPtr(); }

    /// Returns current sender of an action.
    /** Valid (non-null) only while an action packet is being handled. Null if it was invoked by server */
    UserConnectionPtr ActionSender() const { return UserConnectionPtr(); }

    // signals

    /// A user is connecting. This is your chance to deny access.
    /** Call user->Disconnect() to deny access and kick the user out.
        @todo the connectionID parameter is unnecessary as it can be retrieved from connection. */
    void UserAboutToConnect(u32 connectionID, UserConnection* connection);

    /// A user has connected (and authenticated)
    /** @param responseData The handler of this signal can add his own application-specific data to this structure.
        This data is sent to the client and the applications on the client computer can read them as needed.
        @todo the connectionID parameter is unnecessary as it can be retrieved from connection. */
    void UserConnected(u32 connectionID, UserConnection* connection, UserConnectedResponseData *responseData);

    void MessageReceived(UserConnection *connection, kNet::packet_id_t, kNet::message_id_t id, const char* data, size_t numBytes);

    /// A user has disconnected
    /** @todo the connectionID parameter is unnecessary as it can be retrieved from connection. */
    void UserDisconnected(u32 connectionID, UserConnection* connection);

    /// The server has been started
    void ServerStarted();

    /// The server has been stopped
    void ServerStopped();

private:
    /// Handle a Kristalli protocol message
    void HandleKristalliMessage(kNet::MessageConnection* source, kNet::packet_id_t, kNet::message_id_t id, const char* data, size_t numBytes);

    /// Handle a user disconnecting
    void HandleUserDisconnected(UserConnection* user);

    /// Handle a login message
    void HandleLogin(kNet::MessageConnection* source, const char* data, size_t numBytes);
    /// Finalize the login of a user. Allow security plugins to inspect login credentials. Return true if allowed to log in
    bool FinalizeLogin(UserConnectionPtr user);

    //UserConnectionWeakPtr actionSender;
    TundraLogic* owner_;
    Framework* framework_;
    int current_port_;
    String current_protocol_;


    UserConnectionList userConnectionList_;
};

}

