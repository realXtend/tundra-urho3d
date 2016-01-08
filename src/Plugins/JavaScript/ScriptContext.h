// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "CoreTypes.h"
#include "CoreDefines.h"
#include "JavaScriptApi.h"

#include "Win.h" // Duktape config will include Windows.h on Windows, include beforehand to avoid problems with ConsoleAPI
#include "duktape.h"

#include <Urho3D/Container/RefCounted.h>

namespace Tundra
{

// JavaScript execution context with its own Duktape heap
class JAVASCRIPT_API ScriptContext : public RefCounted
{
public:
    /// Create the heap & context. Creation will be signaled through the JavaScript module.
    ScriptContext(Framework* framework);
    /// Destroy the heap & context.
    ~ScriptContext();

    /// Evaluate JavaScript in the context.
    bool Evaluate(const String& script);
    /// Call a global function. \todo Parameter passing & return value
    bool Execute(const String& functionName);

    /// Return the Duktape context.
    duk_context* Context() const;

private:
    /// Duktape context.
    duk_context* ctx_;
    /// Framework pointer.
    Framework* framework_;
};

}
