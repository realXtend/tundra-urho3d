// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "CoreTypes.h"
#include <memory>

namespace Tundra
{
    class TundraLogic;
    class KristalliProtocol;
    class SyncManager;
    class InterestManager;
    class Client;
    class Server;
    class UserConnection;
    class KNetUserConnection;
    class SceneSyncState;
    struct EntitySyncState;
    

    struct MsgLoginReply;
    struct MsgClientJoined;
    struct MsgClientLeft;
    struct MsgEntityAction;
    struct UserConnectedResponseData;

    typedef Urho3D::SharedPtr<UserConnection> UserConnectionPtr;
    typedef Urho3D::Vector<UserConnectionPtr> UserConnectionList;
    typedef Urho3D::SharedPtr<KNetUserConnection> KNetUserConnectionPtr;

    typedef Urho3D::HashMap<String, Variant> LoginPropertyMap;
}

