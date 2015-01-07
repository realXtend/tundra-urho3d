// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"
#include "CoreTypes.h"

#include <Urho3D/Container/RefCounted.h>

namespace Tundra
{

/// Interface for different script instances
class TUNDRACORE_API IScriptInstance : public RefCounted
{
public:
    /// Default constuctor.
    IScriptInstance() : trusted_(false) {}

    /// Destructor.
    virtual ~IScriptInstance() {}

    /// Loads this script instance.
    virtual void Load() = 0;

    /// Unloads this script instance.
    virtual void Unload() = 0;

    /// Starts this script instance.
    virtual void Run() = 0;

    /// Return whether the script has been run.
    virtual bool IsEvaluated() const = 0;

    /// Dumps engine information into a string. Used for debugging/profiling.
    virtual HashMap<String, uint> DumpEngineInformation() = 0;

protected:
    /// Whether this instance executed trusted code or not. 
    /** By default everything loaded remotely (with e.g. http) is untrusted,
        and not exposed anything with system access. */
    bool trusted_;
};

};
