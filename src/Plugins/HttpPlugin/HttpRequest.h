// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "HttpPluginApi.h"
#include "HttpDefines.h"

#include "LoggingFunctions.h"

#include <Engine/Core/Mutex.h>
#include <Engine/Container/Str.h>
#include <Engine/Container/HashMap.h>
#include <Engine/Container/RefCounted.h>

#include "HttpCurlInterop.h"

struct http_parser;

namespace Tundra
{

/// HTTP request
class TUNDRA_HTTP_API HttpRequest : public Urho3D::RefCounted
{
    /// @cond PRIVATE
    friend class HttpWorkThread;
    friend size_t CurlReadBody(void *buffer, size_t size, size_t items, void *data);
    friend size_t CurlReadHeaders(char *buffer, size_t size, size_t items, void *data);
    friend int HttpParserStatus(http_parser *p, const char *buf, size_t len);
    friend int HttpParserHeaderField(http_parser *p, const char *buf, size_t len);
    friend int HttpParserHeaderValue(http_parser *p, const char *buf, size_t len);
    /// @endcond

public:
    HttpRequest(Http::Method method, const String &url);
    ~HttpRequest();

    /// Sets verbose stdout logging for this request.
    /** Useful when you want to inspect outgoing and incoming headers and data. */
    void SetVerbose(bool enabled);

    ///////////////////////// REQUEST

    /// Set request body and "Content-Type" header.
    bool SetBody(const Vector<u8> &body, const String &contentType = "application/octet-stream");
    bool SetBody(const String &body, const String &contentType = "text/plain"); ///< @overload

    /// Clears any previously set body and "Content-Type" header.
    bool ClearBody();

    /// Set request header @c name to @c value.
    /** @see AppendHeaders. */
    bool SetHeader(const String &name, const String &value);
    bool SetHeader(const String &name, int value);  ///< @overload
    bool SetHeader(const String &name, uint value); ///< @overload

    /// Append @headers to request.
    /** This function appends and replaces (if a particular header is already set)
        multiple headers. It does not clean existing ones.
        @see SetHeader and ClearHeaders. */
    bool AppendHeaders(const HttpHeaderMap &headers);

    /// Clears any existing request headers.
    bool ClearHeaders();

    /// Removes request header @c name.
    bool RemoveHeader(const String &name);

    ///////////////////////// RESPONSE

    /// Returns if response contained header @c name.
    bool HasResponseHeader(const String &name);

    /// Returns response header @c name as a string.
    String ResponseHeader(const String &name);

    /// Returns response header @c name as a int.
    /** @note If cannot be parsed to int @c defaultValue is returned. */
    int ResponseHeaderInt(const String &name, int defaultValue = 0);

    /// Returns response header @c name as a uint.
    /** @note If cannot be parsed to uint @c defaultValue is returned. */
    uint ResponseHeaderUInt(const String &name, uint defaultValue = 0);

private:
    void Cleanup();

    void DumpResponse(bool headers, bool body);

    /* Internal functions that let threaded code decide about mutex locking.
       Also act as common functions to set/get either response or request headers. */
    bool SetHeaderInternal(const String &name, const String &value, bool respose, bool lock = true);
    bool RemoveHeaderInternal(const String &name, bool respose, bool lock = true);
    bool HasHeaderInternal(const String &name, bool respose, bool lock = true);
    String HeaderInternal(const String &name, bool respose, bool lock = true);
    int HeaderIntInternal(const String &name, int defaultValue, bool respose, bool lock = true);
    uint HeaderUIntInternal(const String &name, uint defaultValue, bool respose, bool lock = true);

    /// Called by HttpWorkThread
    bool Prepare();
    /// Called by HttpWorkThread
    void Perform();

    /// Called by CurlReadBody
    size_t ReadBody(void *buffer, uint size);
    /// Called by CurlReadHeaders
    size_t ReadHeaders(void *buffer, uint size);
    // Called by HttpParserStatus
    int ReadStatus(const char *buffer, uint size);
    // Called by HttpParserHeaderField
    int ReadHeaderField(const char *buffer, uint size);
    // Called by HttpParserHeaderValue
    int ReadHeaderValue(const char *buffer, uint size);

    // Outgoing/incomfing data structures
    Http::RequestData requestData_;
    Http::ResponseData responseData_;

    Urho3D::Mutex mutexPerforming_;
    bool performing_;
    bool verbose_;

    Logger log;
};

}
