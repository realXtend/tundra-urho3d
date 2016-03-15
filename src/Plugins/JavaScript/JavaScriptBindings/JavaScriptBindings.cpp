// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "JavaScriptBindings.h"

namespace JSBindings
{

void Expose_JavaScriptInstance(duk_context* ctx);

void ExposeJavaScriptClasses(duk_context* ctx)
{
    Expose_JavaScriptInstance(ctx);
}

}