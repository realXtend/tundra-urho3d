// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "HttpPluginApi.h"
#include "HttpDefines.h"

#include "Signals.h"
#include "LoggingFunctions.h"

#include <Engine/Core/Mutex.h>
#include <Engine/Container/Str.h>
#include <Engine/Container/HashMap.h>
#include <Engine/Container/RefCounted.h>

#include "HttpCurlInterop.h"

struct http_parser;

namespace Tundra
{

class JSONValue;

/// HTTP request
class TUNDRA_HTTP_API HttpRequest : public Urho3D::RefCounted
{
    /// @cond PRIVATE
    friend class HttpClient;
    friend class HttpWorkThread;
    friend class HttpWorkQueue;
    friend size_t CurlReadBody(void *buffer, size_t size, size_t items, void *data);
    friend size_t CurlReadHeaders(char *buffer, size_t size, size_t items, void *data);
    friend int HttpParserStatus(http_parser *p, const char *buf, size_t len);
    friend int HttpParserHeaderField(http_parser *p, const char *buf, size_t len);
    friend int HttpParserHeaderValue(http_parser *p, const char *buf, size_t len);
    /// @endcond

public:
    /// Returns if the request has started execution.
    /** @see Finished. */
    bool HasStarted();

    /// Returns if the request is currently executing. Meaning started but not yet completed.
    /** @see Finished. */
    bool IsExecuting();

    /// Returns if the request has completed execution.
    /** @see Finished. */
    bool HasCompleted();

    /// Sets verbose stdout logging for this request.
    /** Useful when you want to inspect outgoing and incoming headers and data. */
    void SetVerbose(bool enabled);

    ///////////////////////// REQUEST API

    /// Returns the requests HTTP method as string.
    /** Return string is uppercased, eg. "GET", "POST", "DELETE". */
    String Method() const;

    /// Returns the requests target URL.
    String Url() const;

    /// Return error string.
    /** Empty string if no errors and request completed succesfully
        or if request has not completed yet.
        @see HasCompleted and Finished. */
    String Error();

    /// Set request body and Content-Type header.
    bool SetBody(const Vector<u8> &body, const String &contentType = Http::ContentType::Binary);
    bool SetBody(const String &body, const String &contentType = Http::ContentType::Text); ///< @overload

    /// Set request body to @c json with Conten-Type header application/json.
    bool SetBodyJSON(const JSONValue &json);
    bool SetBodyJSON(const String &json); ///< @overload

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

    ///////////////////////// RESPONSE API

    /// Returns status code eg, 200 if request has completed successfully, otherwise -1.
    int StatusCode();

    /// Returns status text eg. "OK" if request has completed, otherwise empty string.
    String Status();

    /// Returns spent time in networking in milliseconds. 0 if request not completed yet.
    uint DurationMSec();

    /// Returns averate download speed in bytes/second. 0.f if request not completed yet.
    float AvertageDownloadSpeed();

    /// Returns the response body by reference if request has completed.
    /** This function should be preferred to avoid copies of large data chunks.
        @return Body if request has completed, otherwise an empty vector.*/
    const Vector<u8> &ResponseBody();

    /// Copies the response body to @c dest if request has completed.
    /** This function useu memcpy and resizes @c dest. @see ResponseBody.
        @return False if request has not completed or response body is empty. */
    bool ResponseBodyCopyTo(Vector<u8> &dest);

    /// Returns a copy of the response body if request has completed.
    /** This function should be avoided. @see ResponseBody(). */
    Vector<u8> ResponseBodyCopy();

    /// @todo Implement response body to JSONValue and JSON string.
    //JSONValue ResponseJSON();

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

    ///////////////////////// SIGNALS

    /// Emitted on completion.
    /** @param Request pointer. You can store this to keep the ptr alive but it is not recommended.
        @param HTTP response status code. -1 if request failed.
        @param Error string. Empty string if request failed. */
    Signal3<HttpRequestPtr&, int, const String&> Finished;

private:
    /// Constructed by HttpClient.
    HttpRequest(int method, const String &url);
    ~HttpRequest();

    /* Internal functions that let threaded code decide about mutex locking.
       Also act as common functions to set/get either response or request headers. */
    bool SetHeaderInternal(const String &name, const String &value, bool respose, bool lock = true);
    bool RemoveHeaderInternal(const String &name, bool respose, bool lock = true);
    bool HasHeaderInternal(const String &name, bool respose, bool lock = true);
    String HeaderInternal(const String &name, bool respose, bool lock = true);
    int HeaderIntInternal(const String &name, int defaultValue, bool respose, bool lock = true);
    uint HeaderUIntInternal(const String &name, uint defaultValue, bool respose, bool lock = true);

    /// Called by HttpWorkThread in worker thread context.
    void Perform();
    /// Invoked in worker thread context.
    bool Prepare();
    /// Invoked in worker thread context.
    void Cleanup();
    
    /// Called by HttpWorkQueue in main thread context.
    void EmitCompletion(HttpRequestPtr &self);
    /// Invoked in main threa context if verbose is enabled.
    void DumpResponse(bool headers, bool body);

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

    Urho3D::Mutex mutexExecute_;
    bool executing_;
    bool completed_;
    bool verbose_;

    Logger log;
};

}
