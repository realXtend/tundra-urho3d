// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

#include "Server.h"
#include "TundraLogic.h"
#include "KristalliProtocol.h"

namespace Tundra
{

Server::Server(TundraLogic* owner) :
    Object(owner->GetContext()),
    owner_(owner),
    framework_(owner->Fw()),
    current_port_(-1)
{
}

UserConnectionList& Server::UserConnections() const
{
    return owner_->KristalliProtocol()->UserConnections();
}
}