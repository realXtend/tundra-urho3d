/**
    For conditions of distribution and use, see copyright notice in LICENSE
 
    @file   EntityAction.h
    @brief  Represents an executable command on an Entity. */

#pragma once

#include "TundraCoreApi.h"
#include "CoreTypes.h"
#include "Signals.h"

#include <RefCounted.h>

namespace Tundra
{

class Entity;

/// Represents an executable command on an Entity.
/** Components (and other instances) can register to these actions by using Entity::ConnectAction().
    Actions allow more complicated in-world logic to be built in slightly more data-driven fashion.
    Actions cannot be created directly, they're created by Entity::Action(). */
class TUNDRACORE_API EntityAction : public Urho3D::RefCounted
{
public:
    ~EntityAction() {}

    /// Returns name of the action.
    const String &Name() const { return name; }

    /// Execution type of the action, i.e. where the actions is executed.
    /** As combinations we get local+server, local+peers(all clients but not server),
        server+peers (everyone but me), local+server+peers (everyone).
        Not all of these sound immediately sensible even, but we know we need to be able to do different things at different times.
        Use the ExecTypeField type to store logical OR combinations of execution types. */
    enum ExecType
    {
        Invalid = 0, ///< Invalid.
        Local = 1, ///< Executed locally.
        Server = 2, ///< Executed on server.
        Peers = 4 ///< Executed on peers.
    };

    /// Used to to store logical OR combinations of execution types.
    typedef unsigned int ExecTypeField;

    /// Emitted when action is triggered.
    /** @param p1 1st parameter for the action, if applicable.
        @param p2 2nd parameter for the action, if applicable.
        @param p3 3rd parameter for the action, if applicable.
        @param rest Rest of the parameters, if applicable. */
    Signal4<String, String, String, StringVector> Triggered;

private:
    friend class Entity;

    /// Constructor.
    /** @param name Name of the action. */
    explicit EntityAction(const String &name);

    /// Triggers this action i.e. emits the Triggered signal.
    /** @param p1 1st parameter for the action, if applicable.
        @param p2 2nd parameter for the action, if applicable.
        @param p3 3rd parameter for the action, if applicable.
        @param rest Rest of the parameters, if applicable. */
    void Trigger(const String &p1 = "", const String &p2 = "", const String &p3 = "", const StringVector &rest = StringVector());

    const String name; ///< Name of the action.
};

}
