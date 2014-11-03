// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include <kNet.h>

#include "TundraLogicApi.h"
#include "TundraLogicFwd.h"
#include "FrameworkFwd.h"
#include "MsgCameraOrientationRequest.h"
#include "Signals.h"
#include <Math/Quat.h>
#include <Object.h>


namespace Tundra
{

/// Top-level scene and network protocol handling logic.
class TUNDRALOGIC_API Client : public Object
{
    OBJECT(Client);

public:
    explicit Client(TundraLogic* owner);
    ~Client();

    /// Perform any per-frame processing
    void Update(float frametime);

    enum ClientLoginState
    {
        NotConnected = 0,
        ConnectionPending,
        ConnectionEstablished,
        LoggedIn
    };

    /// Returns connection/login state
    ClientLoginState LoginState() const { return loginstate_; }

    /// Returns client connection ID (from loginreply message), or zero if not connected.
    u32 ConnectionId() const { return client_id_; }

    /// Returns the underlying kNet MessageConnection object that represents this connection.
    /** This function may return null in the case the connection is not active. */
    kNet::MessageConnection* MessageConnection();

    /// Returns the "virtual" user connection object representing the server. This object will exist always, but its MessageConnection is null when not connected.
    KNetUserConnectionPtr ServerUserConnection() const;

    /// Connect and login. Username and password will be encoded to XML key-value data
    /// @note This function will be deleted in the future.
    void Login(const String& address, unsigned short port, const String& username, const String& password, const String &protocol = String());

    /// Connect and login using the login properties that were previously set with calls to SetLoginProperty.
    void Login(const String& address, unsigned short port, kNet::SocketTransportLayer protocol = kNet::InvalidTransportLayer);

    /// Disconnects the client from the current server, and also deletes all contents from the client scene.
    /** --Delays the logout by one frame, so it is safe to call from scripts.-- */
    void Logout();

    /// Logout immediately and delete the client scene content
    /** @param fail Pass in true if the logout was due to connection/login failure. False, if the connection was aborted deliberately by the client. */
    void DoLogout(bool fail = false);

    /// See if connected & authenticated
    bool IsConnected() const;

     /// Sets the given login property with the given value.
    /** Call this function prior connecting to a scene to specify data that should be carried to the server as initial login data.
        @param key The name of the login property to set. If a previous login property with this name existed, it is overwritten.
        @param value The value to specify for this login property. If "", the login property is deleted and will not be sent. */
    void SetLoginProperty(String key, Urho3D::Variant value);

    /// Returns the login property value of the given name.
    /** @return value of the key, or an empty string if the key was not found. */
    Urho3D::Variant LoginProperty(String key) const;

    /// Returns whether has a login property
    bool HasLoginProperty(String key) const;

    /// Returns login properties as xml string
    String LoginPropertiesAsXml() const;

    /// Returns all the login properties that will be used to login to the server.
    LoginPropertyMap LoginProperties() const { return properties_; }

    /// Deletes all set login properties.
    void ClearLoginProperties() { properties_.Clear(); }

    /// Get the current camera orientation
    /// @todo SyncManager/InterestManager functionality. Move away from here.
    void GetCameraOrientation();

    /// This signal is emitted right before this client is starting to connect to a Tundra server.
    /** Any script or other piece of code can listen to this signal, and as at this point, fill in any internal
        custom data (called "login properties") they need to add to the connection handshake. The server will get 
        all the login properties and a server-side script can do validation and storage of whether the client
        can be authorized to log in or not. */
    Signal0<void> AboutToConnect;

    /// This signal is emitted immediately after this client has successfully connected to a server.
    /// @param responseData This is the data that the server sent back to the client related to the connection.
    Signal1<UserConnectedResponseData* ARG(responseData)> Connected;

    /// Triggered whenever a new message is received from the network.
    Signal4<kNet::packet_id_t, kNet::message_id_t ARG(id), const char* ARG(data), size_t ARG(numBytes)> NetworkMessageReceived;

    /// This signal is emitted when the client has disconnected from the server.
    Signal0<void> Disconnected;

    /// Emitted when a login attempt failed to a server.
    Signal1<const String& ARG(reason)> LoginFailed;

private:
    /// Handles a Kristalli protocol message
    void HandleKristalliMessage(kNet::MessageConnection* source, kNet::packet_id_t, kNet::message_id_t id, const char* data, size_t numBytes);

    void OnConnectionAttemptFailed();

     /// Handles pending login to server
    void CheckLogin();

    /// Actually perform a delayed logout
    void DelayedLogout();

    /// Handles a camera orientation request message
    /// @todo SyncManager/InterestManager functionality. Move away from here.
    void HandleCameraOrientationRequest(kNet::MessageConnection* source, const MsgCameraOrientationRequest& msg);

    /// Handles a loginreply message
    void HandleLoginReply(kNet::MessageConnection* source, const char *data, size_t numBytes);

    /// Handles a client joined message
    void HandleClientJoined(kNet::MessageConnection* source, const MsgClientJoined& msg);

    /// Client: Handles a client left message
    void HandleClientLeft(kNet::MessageConnection* source, const MsgClientLeft& msg);

    ClientLoginState loginstate_;
    LoginPropertyMap properties_;
    bool reconnect_; ///< Whether the connect attempt is a reconnect because of dropped connection
    u32 client_id_; ///< User ID, once known

    TundraLogic* owner_;
    Framework* framework_;

    // Current camera orientation
    /// @todo SyncManager/InterestManager functionality. Move away from here.
    Quat currentcameraorientation_;
    // Current camera location
    /// @todo SyncManager/InterestManager functionality. Move away from here.
    float3 currentcameralocation_;

    bool sendCameraUpdates_;
    bool firstCameraUpdateSent_;

    /// "Virtual" user connection representing the server and its syncstate (client only)
    KNetUserConnectionPtr serverUserConnection_;
};

}
