// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "HttpServerApi.h"
#include "CoreTypes.h"
#include "CoreDefines.h"

#include <Urho3D/Container/Str.h>
#include <Urho3D/Core/Object.h>

namespace Tundra
{

class HTTPSERVER_API HttpRequest : public Object
{
    URHO3D_OBJECT(HttpRequest, Object);

public:
    /// Construct with connection pointer. Is not valid to be accessed after the signal (from which this object is received) handling is complete.
    HttpRequest(Urho3D::Context* ctx, void* connection_);

    /// Return request path. [property]
    String Path() const;
    /// Return request host. [property]
    String Host() const;
    /// Return request port. [property]
    unsigned short Port() const;
    /// Return request method (verb.) [property]
    String Method() const;
    /// Return request header, or empty if not defined.
    String RequestHeader(const String& header) const;
    /// Return response header, or empty if not defined.
    String ResponseHeader(const String& header) const;
    /// Return request body. [property]
    String Body() const;

    /// Set response header.
    void SetResponseHeader(const String& header, const String& content);
    /// Set response body and content type. Content-Length header is set appropriately.
    void SetBody(const String& body, const String& contentType = String());
    /// Set response status code.
    void SetStatusCode(int code);

    /// websocketpp connection object. [noscript]
    void* connection;
};

}