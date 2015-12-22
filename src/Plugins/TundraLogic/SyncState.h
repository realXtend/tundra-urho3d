// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include <kNet/PolledTimer.h>
#include <kNet/Types.h>

#include "TundraLogicApi.h"
#include "TundraLogicFwd.h"
#include "CoreTypes.h"
#include "CoreDefines.h"
#include "SceneFwd.h"

#include "Math/Transform.h"
#include "Math/float3.h"
#include "MsgEntityAction.h"
#include "Signals.h"

#include <Urho3D/Core/Object.h>
#include <Urho3D/Core/Variant.h>
#include <Urho3D/Container/List.h>
#include <Urho3D/Container/HashMap.h>
//#include <list>
#include <map>
#include <set>

namespace Tundra
{

class SceneSyncState;

/// Component's per-user network sync state
struct ComponentSyncState
{
    ComponentSyncState() :
        removed(false),
        isNew(true),
        isInQueue(false),
        id(0)
    {
        for (unsigned i = 0; i < 32; ++i)
            dirtyAttributes[i] = 0;
    }
    
    void MarkAttributeDirty(u8 attrIndex)
    {
        dirtyAttributes[attrIndex >> 3] |= (1 << (attrIndex & 7));
    }
    
    void MarkAttributeCreated(u8 attrIndex)
    {
        newAndRemovedAttributes[attrIndex] = true;
    }
    
    void MarkAttributeRemoved(u8 attrIndex)
    {
        newAndRemovedAttributes[attrIndex] = false;
    }
    
    void DirtyProcessed()
    {
        for (unsigned i = 0; i < 32; ++i)
            dirtyAttributes[i] = 0;
        newAndRemovedAttributes.Clear();
        isNew = false;
    }
    
    u8 dirtyAttributes[32]; ///< Dirty attributes bitfield. A maximum of 256 attributes are supported.
    Urho3D::HashMap<u8, bool> newAndRemovedAttributes; ///< Dynamic attributes by index that have been removed or created since last update. True = create, false = delete
    component_id_t id; ///< Component ID. Duplicated here intentionally to allow recognizing the component without the parent map.
    bool removed; ///< The component has been removed since last update
    bool isNew; ///< The client does not have the component and it must be serialized in full
    bool isInQueue; ///< The component is already in the entity's dirty queue
};

/// Entity's per-user network sync state
struct EntitySyncState
{
    EntitySyncState() :
        removed(false),
        isNew(true),
        isInQueue(false),
        hasPropertyChanges(false),
        hasParentChange(false),
        id(0),
        avgUpdateInterval(0.0f),
        priority(-1.f),
        relevancy(-1.f)
    {
    }
    
    void RemoveFromQueue(component_id_t id)
    {
        auto i = components.find(id);
        if (i != components.end())
        {
            if (i->second.isInQueue)
            {
                for (auto j = dirtyQueue.Begin(); j != dirtyQueue.End(); ++j)
                {
                    if ((*j) == &i->second)
                    {
                        dirtyQueue.Erase(j);
                        break;
                    }
                }
                i->second.isInQueue = false;
            }
        }
    }
    
    void MarkComponentDirty(component_id_t id)
    {
        ComponentSyncState& compState = components[id]; // Creates new if did not exist
        if (!compState.id)
            compState.id = id;
        if (!compState.isInQueue)
        {
            dirtyQueue.Push(&compState);
            compState.isInQueue = true;
        }
    }
    
    void MarkComponentRemoved(component_id_t id)
    {
        // If user did not have the component in the first place, do nothing
        auto i = components.find(id);
        if (i == components.end())
            return;
        // If component is marked new, it was not sent yet and can be simply removed from the sync state
        if (i->second.isNew)
        {
            RemoveFromQueue(id);
            components.erase(id);
            return;
        }
        // Else mark as removed and queue the update
        i->second.removed = true;
        if (!i->second.isInQueue)
        {
            dirtyQueue.Push(&i->second);
            i->second.isInQueue = true;
        }
    }
    
    void DirtyProcessed()
    {
        for (auto i = components.begin(); i != components.end(); ++i)
        {
            i->second.DirtyProcessed();
            i->second.isInQueue = false;
        }
        dirtyQueue.Clear();
        isNew = false;
        hasPropertyChanges = false;
        hasParentChange = false;
    }
    
    void RefreshAvgUpdateInterval()
    {
        float time = updateTimer.MSecsElapsed() * 0.001f;
        updateTimer.Start();
        // If it's the first measurement, set time directly. Else smooth
        if (avgUpdateInterval == 0.0f)
            avgUpdateInterval = time;
        else
            avgUpdateInterval = 0.5f * time + 0.5f * avgUpdateInterval;
    }

    /// Compares EntitySyncStates by FinalPriority().
    /*  @remark Interest management */
    bool operator < (const EntitySyncState &rhs) const
    {
        return FinalPriority() < rhs.FinalPriority();
    }

    /// Computes prioritized network update interval in seconds.
    /*  @remark Interest management */
    float ComputePrioritizedUpdateInterval(float maxUpdateRate) const { return Clamp(maxUpdateRate * Log2(100.f / FinalPriority()), maxUpdateRate, MinUpdateRate); }

    /// Returns final/combined priority of this sync state.
    /*  @remark Interest management */
    float FinalPriority() const { return priority * relevancy; }

    static const float MinUpdateRate; ///< 5 (in seconds)
//    static const float MaxUpdateRate; ///< 0.005 (in seconds)

    Urho3D::List<ComponentSyncState*> dirtyQueue; ///< Dirty components
    std::map<component_id_t, ComponentSyncState> components; ///< Component syncstates

    entity_id_t id; ///< Entity ID. Duplicated here intentionally to allow recognizing the entity without the parent map.
    EntityWeakPtr weak; ///< Entity weak ptr.

    bool removed; ///< The entity has been removed since last update
    bool isNew; ///< The client does not have the entity and it must be serialized in full
    bool isInQueue; ///< The entity is already in the scene's dirty queue
    bool hasPropertyChanges; ///< The entity has changes into its other properties, such as temporary flag
    bool hasParentChange; ///> The entity's parent has changed
    
    kNet::PolledTimer updateTimer; ///< Last update received timer, for calculating avgUpdateInterval.
    float avgUpdateInterval; ///< Average network update interval in seconds, used for interpolation.

    // Special cases for rigid body streaming:
    // On the server side, remember the last sent rigid body parameters, so that we can perform effective pruning of redundant data.
    Transform transform;
    float3 linearVelocity;
    float3 angularVelocity;
    kNet::tick_t lastNetworkSendTime; /**< @note Shared usage by rigid body optimization and interest management. */

    /// Priority = size / distance for visible entities, inf for non-visible.
    /** Larger number means larger importancy. If this value has not been yet calculated it's < 0.
        Used to determinate the prioritized update interval of the entity together with relevancy.
        @remark Interest management */
    float priority;

    /// Arbitrary relevancy factor.
    /** Larger number means larger importancy. If this has not been yet calculated it's < 0.
        F.ex. direction or visibility or of the entity can affect this.
        Used to determinate the prioritized update interval of the entity together with priority.
        @remark Interest management */
    float relevancy;
};

struct RigidBodyInterpolationState
{
    // On the client side, remember the state for performing Hermite interpolation (C1, i.e. pos and vel are continuous).
    struct RigidBodyState
    {
        float3 pos;
        float3 vel;
        Quat rot;
        float3 scale;
        float3 angVel; // Angular velocity in Euler ZYX.
    };

    RigidBodyState interpStart;
    RigidBodyState interpEnd;
    float interpTime;

    // If true, we are using linear inter/extrapolation to move the entity.
    // If false, we have handed off this entity for physics to extrapolate.
    bool interpolatorActive;

    /// Remembers the packet id of the most recently received network sync packet. Used to enforce
    /// proper ordering (generate latest-data-guarantee messaging) for the received movement packets.
    kNet::packet_id_t lastReceivedPacketCounter;
};

/// State change request to permit/deny changes.
class TUNDRALOGIC_API StateChangeRequest : public RefCounted
{
public:
    StateChangeRequest(u32 connectionID);

    void Reset(entity_id_t entityId = 0)
    {
        accepted_ = true;
        entityId_ = entityId;
        entity_ = 0;
    }

    /// Set the change request accepted
    void SetAccepted(bool accepted)
    { 
        accepted_ = accepted; 
    }

    /// Accept the whole change request.
    void Accept()
    {
        accepted_ = true;
    }

    /// Reject the whole change request.
    void Reject()
    {
        accepted_ = false;
    }


    bool Accepted()                         { return accepted_; }
    bool Rejected()                         { return !accepted_; }

    u32 ConnectionID()                      { return connectionID_; }

    entity_id_t EntityId()                  { return entityId_; }
    Entity* GetEntity()                     { return entity_; }
    void SetEntity(Entity* entity)          { entity_ = entity; }

private:
    bool accepted_;
    u32 connectionID_;

    entity_id_t entityId_;
    Entity* entity_;
};

typedef Urho3D::List<component_id_t> ComponentIdList;

/// Scene's per-user network sync state
class TUNDRALOGIC_API SceneSyncState : public RefCounted
{
public:
    explicit SceneSyncState(u32 userConnectionID = 0, bool isServer = false);
    virtual ~SceneSyncState();

    /// Entity sync states
    EntitySyncStateMap entities; 

    /// Dirty entity states by ID pending processing
    Urho3D::HashMap<entity_id_t,EntitySyncState*> dirtyEntities;
    /// Dirty entity list pending processing. @remark Interest management
    std::list<EntitySyncState*> dirtyQueue;

    /// Entity interpolations
    std::map<entity_id_t, RigidBodyInterpolationState> entityInterpolations;

    /// Queued EntityAction messages. These will be sent to the user on the next network update tick.
    std::vector<MsgEntityAction> queuedActions;

    /// Last sent (client) or received (server) observer position in world coordinates.
    /** If !IsFinite() ObserverPosition message has not been been received from the client. */
    float3 observerPos;
    /// Last sent (client) or received (server) observer orientation in world coordinates, Euler ZYX in degrees.
    /** If !IsFinite() ObserverPosition message has not been been received from the client. */
    float3 observerRot;

    // signals

    /// This signal is emitted when a entity is being added to the client sync state.
    /// All needed data for evaluation logic is in the StateChangeRequest parameter object.
    /// If 'request.Accepted()' is true (default) the entity will be added to the sync state,
    /// otherwise it will be added to the pending entities list. 
    /// @note See also HasPendingEntity and MarkPendingEntityDirty.
    /// @param request StateChangeRequest object.
    Signal1<StateChangeRequest* ARG(request)> AboutToDirtyEntity;

    /// Removes the entity from the sync state and puts it to the pending list.
    /// @remark Enables a 'pending' logic in SyncManager, with which a script can throttle the sending of entities to clients.
    void MarkEntityPending(entity_id_t id);

    /// Adds all currently pending entities to the clients sync state.
    /// @remark Enables a 'pending' logic in SyncManager, with which a script can throttle the sending of entities to clients.
    void MarkPendingEntitiesDirty();

    /// Adds entity with id to the clients sync state.
    /// @remark Enables a 'pending' logic in SyncManager, with which a script can throttle the sending of entities to clients.
    void MarkPendingEntityDirty(entity_id_t id);

    /// Returns all pending entity IDs.
    /// @remark Enables a 'pending' logic in SyncManager, with which a script can throttle the sending of entities to clients.
    VariantList PendingEntityIDs() const;

    /// Returns 0 if no pending entities.
    /// @remark Enables a 'pending' logic in SyncManager, with which a script can throttle the sending of entities to clients.
    entity_id_t NextPendingEntityID() const;

    /// Returns if we have any pending entitys.
    /// @remark Enables a 'pending' logic in SyncManager, with which a script can throttle the sending of entities to clients.
    bool HasPendingEntities() const;

    /// Returns if we have pending entity with id.
    /// @remark Enables a 'pending' logic in SyncManager, with which a script can throttle the sending of entities to clients.
    bool HasPendingEntity(entity_id_t id) const;

public:
    void SetParentScene(SceneWeakPtr scene);
    void Clear();

    /// Gets or creates a new entity sync state.
    /** If a new state is created it will get initialized with id and EntityWeakPtr. */
    EntitySyncState &GetOrCreateEntitySyncState(entity_id_t id);
    
    void RemoveFromQueue(entity_id_t id);

    void MarkEntityProcessed(entity_id_t id);
    void MarkComponentProcessed(entity_id_t id, component_id_t compId);

    /** Mark a entity to be dirty for this client state.
        @return If entity was marked dirty. This depends if the entity has been marked as pending.
        If it is the state will refuse to dirty it, it can only be dirtied by application
        logic by calling MarkPendingEntityDirty. */
    bool MarkEntityDirty(entity_id_t id, bool hasPropertyChanges = false, bool hasParentChange = false);
    void MarkEntityRemoved(entity_id_t id);

    void MarkComponentDirty(entity_id_t id, component_id_t compId);
    void MarkComponentRemoved(entity_id_t id, component_id_t compId);

    void MarkAttributeDirty(entity_id_t id, component_id_t compId, u8 attrIndex);
    void MarkAttributeCreated(entity_id_t id, component_id_t compId, u8 attrIndex);
    void MarkAttributeRemoved(entity_id_t id, component_id_t compId, u8 attrIndex);

    // Silently does the same as MarkEntityDirty without emitting change request signal.
    EntitySyncState& MarkEntityDirtySilent(entity_id_t id);

    // Removes entity from pending lists.
    void RemovePendingEntity(entity_id_t id);

    bool NeedSendPlaceholderComponents() const { return !placeholderComponentsSent_; }
    void MarkPlaceholderComponentsSent() { placeholderComponentsSent_ = true; }

private:
    // Returns if entity with id should be added to the sync state.
    bool ShouldMarkAsDirty(entity_id_t id);

    // Fills changeRequest_ with entity data, returns if request is valid. 
    bool FillRequest(entity_id_t id);
    
    // Adds the entity id to the pending entity list.
    void AddPendingEntity(entity_id_t id);

    /// @remark Enables a 'pending' logic in SyncManager, with which a script can throttle the sending of entities to clients.
    /// @todo This data structure needs to be removed. This is double book-keeping. Instead, track the dirty and pending entities
    ///       with the same dirty bit in EntitySyncState and ComponentSyncState.
    std::vector<entity_id_t> pendingEntities_;

    StateChangeRequest changeRequest_;
    bool isServer_;
    bool placeholderComponentsSent_;
    u32 userConnectionID_;

    SceneWeakPtr scene_;
};

}
