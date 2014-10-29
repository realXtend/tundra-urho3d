// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "HttpPluginApi.h"
#include "HttpDefines.h"

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
    HttpRequest(const String &url);
    ~HttpRequest();

    bool HasHeader(const String &name);
    String Header(const String &name);
    int HeaderInt(const String &name, int defaultValue = 0);
    uint HeaderUInt(const String &name, uint defaultValue = 0);

    /// Sets verbose stdout logging for this request.
    /** Useful when you want to inspect outgoing and incoming headers and data. */
    void SetVerbose(bool enabled);

private:
    void Cleanup();

    void DumpResponse(bool headers, bool body);

    // Internal functions that let threaded code decide about mutex locking
    bool HasHeaderInternal(const String &name, bool lock = true);
    String HeaderInternal(const String &name, bool lock = true);
    int HeaderIntInternal(const String &name, int defaultValue = 0, bool lock = true);
    uint HeaderUIntInternal(const String &name, uint defaultValue = 0, bool lock = true);

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
    HttpRequestData requestData_;
    HttpResponseData responseData_;

    Urho3D::Mutex mutexPerforming_;
    bool performing_;
    bool verbose_;
};

}
