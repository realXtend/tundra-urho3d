// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "CoreBindings.h"

namespace JSBindings
{

void Expose_Entity(duk_context* ctx);
void Expose_IComponent(duk_context* ctx);
void Expose_Scene(duk_context* ctx);

void ExposeCoreClasses(duk_context* ctx)
{
    Expose_Entity(ctx);
    Expose_IComponent(ctx);
    Expose_Scene(ctx);
}

}