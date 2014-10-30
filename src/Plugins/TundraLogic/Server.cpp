// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#define _WINSOCKAPI_
#include "Server.h"
#include "TundraLogic.h"
#include "KristalliProtocol.h"

namespace Tundra
{

Server::Server(TundraLogic* owner) :
    Object(owner->GetContext()),
    userConnectionList_()
{
}

UserConnectionList& Server::UserConnections() const
{
    return owner_->GetKristalliModule()->GetUserConnections();
}
}