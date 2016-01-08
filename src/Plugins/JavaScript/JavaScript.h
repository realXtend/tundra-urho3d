// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IModule.h"
#include "JavaScriptApi.h"
#include "Signals.h"

namespace Tundra
{

class ScriptContext;

/// JavaScript scripting module using the Duktape VM
class JAVASCRIPT_API JavaScript : public IModule
{
    URHO3D_OBJECT(JavaScript, IModule);

public:
    JavaScript(Framework* owner);
    ~JavaScript();

    /// Emitted when a new JavaScript context has been created. Use this to expose more classes to the context.
    Signal1<ScriptContext*> ScriptContextCreated;

private:
    void Load() override;
    void Initialize() override;
    void Uninitialize() override;
};

}
