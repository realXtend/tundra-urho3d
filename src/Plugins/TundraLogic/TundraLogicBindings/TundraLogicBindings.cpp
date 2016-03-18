// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "TundraLogicBindings.h"

namespace JSBindings
{

void Expose_Client(duk_context* ctx);
void Expose_Server(duk_context* ctx);
void Expose_SyncManager(duk_context* ctx);
void Expose_UserConnection(duk_context* ctx);

void ExposeTundraLogicClasses(duk_context* ctx)
{
    Expose_Client(ctx);
    Expose_Server(ctx);
    Expose_SyncManager(ctx);
    Expose_UserConnection(ctx);
}

}