// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "AvatarApplicationBindings.h"

namespace JSBindings
{

void Expose_Avatar(duk_context* ctx);
void Expose_AvatarDescAsset(duk_context* ctx);

void ExposeAvatarApplicationClasses(duk_context* ctx)
{
    Expose_Avatar(ctx);
    Expose_AvatarDescAsset(ctx);
}

}