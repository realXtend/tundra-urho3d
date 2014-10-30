// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

#include "UserConnection.h"
#include "Entity.h"
#include "LoggingFunctions.h"
#include "Client.h"

namespace Tundra
{

UserConnection::UserConnection(Object* owner) : 
    Object(owner->GetContext()),
    userID(0),
    protocolVersion(ProtocolOriginal)
{}

void UserConnection::Send(kNet::message_id_t id, bool reliable, bool inOrder, kNet::DataSerializer& ds, unsigned long priority, unsigned long contentID)
{
    Send(id, ds.GetData(), ds.BytesFilled(), reliable, inOrder, priority, contentID);
}

void UserConnection::EmitNetworkMessageReceived(kNet::packet_id_t packetId, kNet::message_id_t messageId, const char* data, size_t numBytes)
{
    NetworkMessageReceived.Emit(this, packetId, messageId, data, numBytes);
}

void UserConnection::Exec(Entity *entity, const String &action, const StringList &params)
{
    if (entity)
        ActionTriggered.Emit(this, entity, action, params);
        //emit ActionTriggered(this, entity, action, params);
    else
        LogWarning("UserConnection::Exec: null entity passed!");
}

void UserConnection::Exec(Entity *entity, const String &action, const String &p1, const String &p2, const String &p3)
{
    StringList params(3);
    params[0] = p1;
    params[1] = p2;
    params[2] = p3;
    Exec(entity, action, params);
}

void UserConnection::SetProperty(const String& key, const String& value)
{
    properties[key] = value;
}

Variant UserConnection::Property(const String& key) const
{
    static String empty;
    
    auto i = properties.Find(key);
    if (i != properties.End())
        return i->second_;
    else
        return empty;
}

bool UserConnection::HasProperty(const String& key) const
{
    auto i = properties.Find(key);
    if (i == properties.End())
        return false;
    else
        return !i->second_.IsEmpty();
}

void UserConnection::DenyConnection(const String &reason)
{
    properties["authenticated"] = false;
    properties["reason"] = reason;
}

KNetUserConnection::KNetUserConnection(Object* owner) : 
    UserConnection(owner)
{
}

void KNetUserConnection::Send(kNet::message_id_t id, const char* data, size_t numBytes, bool reliable, bool inOrder, unsigned long priority, unsigned long contentID)
{
    if (!data && numBytes)
    {
        LogError("KNetUserConnection::Send: can not queue message, null data pointer with nonzero data size specified");
        return;
    }

    if (!connection)
    {
        LogError("KNetUserConnection::Send: can not queue message as MessageConnection is null");
        return;
    }

    kNet::NetworkMessage* msg = connection->StartNewMessage(id, numBytes);
    if (numBytes)
        memcpy(msg->data, data, numBytes); /// \todo Copy should be optimized out
    msg->reliable = reliable;
    msg->inOrder = inOrder;
    msg->priority = priority;
    msg->contentID = contentID;
    connection->EndAndQueueMessage(msg);
}

void KNetUserConnection::Disconnect()
{
    if (connection)
        connection->Disconnect(0);
}


void KNetUserConnection::Close()
{
    if (connection)
        connection->Close(0);
}

}