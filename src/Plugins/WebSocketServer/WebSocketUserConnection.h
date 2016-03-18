// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "WebSocketServerApi.h"
#include "Win.h"

#include "FrameworkFwd.h"
#include "WebSocketFwd.h"
#include "kNetFwd.h"

#include "WebSocketServer.h"
#include "SyncState.h"
#include "UserConnection.h"

namespace WebSocket
{
    class WEBSOCKETSERVER_API UserConnection : public Tundra::UserConnection
    {
    public:
        UserConnection(Urho3D::Context* context, ConnectionPtr connection_);
        ~UserConnection();

        virtual Tundra::String ConnectionType() const { return "websocket"; }

        ConnectionPtr WebSocketConnection() const;

        void Send(const kNet::DataSerializer &data);

        /// Queue a network message to be sent to the client. All implementations may not use the reliable, inOrder, priority and contentID parameters.
        virtual void Send(kNet::message_id_t id, const char* data, size_t numBytes, bool reliable, bool inOrder, unsigned long priority = 100, unsigned long contentID = 0);

        ConnectionWeakPtr webSocketConnection;

    public:
        virtual void Disconnect();
        virtual void Close();
    };
}
