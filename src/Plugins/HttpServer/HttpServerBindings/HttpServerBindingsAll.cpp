// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "HttpServerBindings.h"

namespace JSBindings
{

void Expose_HttpServer(duk_context* ctx);
void Expose_HttpRequest(duk_context* ctx);

void ExposeHttpServerClasses(duk_context* ctx)
{
    Expose_HttpServer(ctx);
    Expose_HttpRequest(ctx);
}

}