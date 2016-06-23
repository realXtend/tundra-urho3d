// For conditions of distribution and use, see copyright notice in LICENSE

#include "HttpRequest.h"
#include "HttpServer.h"

namespace Tundra
{

HttpRequest::HttpRequest(Urho3D::Context* ctx, void* connection_) :
    Object(ctx),
    connection(connection_)
{
    assert(connection);
}

String HttpRequest::Host() const
{
    HttpServer::Connection* conn = (HttpServer::Connection*)connection;
    return String(conn->get_host().c_str());
}

unsigned short HttpRequest::Port() const
{
    HttpServer::Connection* conn = (HttpServer::Connection*)connection;
    return conn->get_port();
}

String HttpRequest::Method() const
{
    HttpServer::Connection* conn = (HttpServer::Connection*)connection;
    return String(conn->get_request().get_method().c_str());
}

String HttpRequest::Path() const
{
    HttpServer::Connection* conn = (HttpServer::Connection*)connection;
    return String(conn->get_resource().c_str());
}

String HttpRequest::RequestHeader(const String& key) const
{
    HttpServer::Connection* conn = (HttpServer::Connection*)connection;
    return String(conn->get_request_header(key.CString()).c_str());
}

String HttpRequest::ResponseHeader(const String& key) const
{
    HttpServer::Connection* conn = (HttpServer::Connection*)connection;
    return String(conn->get_request_header(key.CString()).c_str());
}

String HttpRequest::Body() const
{
    HttpServer::Connection* conn = (HttpServer::Connection*)connection;
    return String(conn->get_response_body().c_str());
}

void HttpRequest::SetResponseHeader(const String& key, const String& value)
{
    HttpServer::Connection* conn = (HttpServer::Connection*)connection;
    conn->replace_header(key.CString(), value.CString());
}

void HttpRequest::SetBody(const String& body, const String& contentType)
{
    HttpServer::Connection* conn = (HttpServer::Connection*)connection;
    conn->set_body(body.CString());
    if (contentType.Length())
        SetResponseHeader("Content-Type", contentType);
    SetResponseHeader("Content-Length", String(body.Length()));
}

void HttpRequest::SetStatusCode(int code)
{
    HttpServer::Connection* conn = (HttpServer::Connection*)connection;
    conn->set_status((websocketpp::http::status_code::value)code);
}

}
