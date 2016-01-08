// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "JavaScript.h"
#include "ScriptContext.h"
#include "Framework.h"
#include "LoggingFunctions.h"

namespace Tundra
{

ScriptContext::ScriptContext(Framework* framework) :
    framework_(framework),
    ctx_(0)
{
    ctx_ = duk_create_heap_default();
    framework_->Module<JavaScript>()->ScriptContextCreated.Emit(this);
}

ScriptContext::~ScriptContext()
{
    duk_destroy_heap(ctx_);
    ctx_ = 0;
}

bool ScriptContext::Evaluate(const String& script)
{
    duk_push_string(ctx_, script.CString());
    bool success = duk_peval(ctx_) == 0;
    if (!success)
        LogError("[JavaScript] Evaluate error: " + String(duk_safe_to_string(ctx_, -1)));

    duk_pop(ctx_); // Pop result/error
    return success;
}

bool ScriptContext::Execute(const String& functionName)
{
    duk_push_global_object(ctx_);
    duk_get_prop_string(ctx_, -1, functionName.CString());
    bool success = duk_pcall(ctx_, 0) == 0;
    if (duk_pcall(ctx_, 0) != 0)
        LogError("[JavaScript] Execute error: " + String(duk_safe_to_string(ctx_, -1)));
    
    duk_pop(ctx_); // Pop result/error
    duk_pop(ctx_); // Pop global object
    return success;
}

}
