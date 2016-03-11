// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "UrhoRendererBindings.h"

namespace JSBindings
{

void Expose_Placeable(duk_context* ctx);

void ExposeUrhoRendererClasses(duk_context* ctx)
{
    Expose_Placeable(ctx);
}

}