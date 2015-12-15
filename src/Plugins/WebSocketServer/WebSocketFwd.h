// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "CoreTypes.h"

class WebSocketServerModule;

namespace WebSocket
{
    class Server;
    class Handler;
    class UserConnection;
    
    typedef Tundra::SharedPtr<UserConnection> UserConnectionPtr;
    typedef Tundra::WeakPtr<UserConnection> UserConnectionWeakPtr;
    typedef Tundra::Vector<UserConnectionPtr> UserConnectionList;
}

struct libwebsocket_context;

typedef Tundra::SharedPtr<WebSocket::Server> WebSocketServerPtr;
typedef Tundra::SharedPtr<WebSocket::UserConnection> WebSocketUserConnectionPtr;
