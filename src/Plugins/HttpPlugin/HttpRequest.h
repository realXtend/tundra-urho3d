// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "HttpPluginApi.h"
#include "HttpDefines.h"

#include "Signals.h"
#include "LoggingFunctions.h"

#include <Urho3D/Core/Mutex.h>
#include <Urho3D/Container/Str.h>
#include <Urho3D/Container/HashMap.h>
#include <Urho3D/Container/RefCounted.h>

#include "HttpCurlInterop.h"

struct http_parser;

namespace Tundra
{

class Framework;
class JSONValue;

/// HTTP request
class TUNDRA_HTTP_API HttpRequest : public Urho3D::RefCounted
{
    /// @cond PRIVATE
    friend class HttpClient;
    friend class HttpWorkThread;
    friend class HttpWorkQueue;
    friend size_t CurlWriteBody(void *buffer, size_t size, size_t items, void *data);
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

    /// Set request body from @c json with Conten-Type header application/json.
    bool SetBodyFromJSON(const JSONValue &json);
    bool SetBodyFromJSON(const String &json); ///< @overload

    /// Set request body from @c filepath with Content-Type header @c contentType.
    bool SetBodyFromFile(const String &filepath, const String &contentType = Http::ContentType::Binary);

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

    /// Sets the source and destination @c filepath for HTTP cache mechanisms.
    /** @param Filepath to used as the response data if '304 Not Modified' without a body is returned by the server.
        Or in the case of '200 OK' with a body response, the data is written to this file prior to completion signals.
        @param If true the reqeusts 'If-Modified-Since' header is set from the files last modification date and time.
        And in the case of writing to the file the responses 'Last-Modified' header is used to set the written files
        last modified timestamp on the operating system level.
        @note This feature simplifies the common HTTP use case of Tundras AssetAPI and the HttpAssetProvider. Additionally
        the disk read/write operations can be executed in the HTTP worker thread without blocking the main thread. */
    bool SetCacheFile(const String &filepath, bool useLastModified);

    /// @overload
    /** @param 'If-Modified-Since' header will be written to the provided @c lastModifiedHttpDate if non empty string. */
    bool SetCacheFile(const String &filepath, const String &lastModifiedHttpDate);

    ///////////////////////// RESPONSE API

    /// Returns status code eg, 200 if request has completed successfully, otherwise -1.
    int StatusCode();

    /// Returns status text eg. "OK" if request has completed, otherwise empty string.
    String Status();

    /// Returns time spent in networking in milliseconds.
    /** @return -1 if request not completed yet or did not perform any networking. */
    int DurationMSec();

    /// Returns average download speed in bytes/second.
    /** @return -1.f if request not completed yet or did not perform any downloading. */
    float AverageDownloadSpeed();

    /// Returns average upload speed in bytes/second.
    /** @return -1.f if request not completed yet or did not perform any uploading. */
    float AverageUploadSpeed();

    /// Returns the response body size in bytes if request has completed.
    /** @return Returns 0 if body is empty or if request has not completed yet.
        Verify completion before calling this function with HasCompleted(). */
    uint ResponseBodySize();

    /// Returns the response body by reference if request has completed.
    /** This function should be preferred to avoid copies of large data chunks.
        @return Body if request has completed, otherwise an empty vector.*/
    const Vector<u8> &ResponseBody();

    /// Copies the response body to @c dest if request has completed.
    /** This function uses memcpy and resizes @c dest. @see ResponseBody.
        @return False if request has not completed or response body is empty. */
    bool CopyResponseBodyTo(Vector<u8> &dest);

    /// Returns a copy/clone of the response body if request has completed.
    /** This function should be avoided for large body sizes. @see ResponseBody(). */
    Vector<u8> CloneResponseBody();

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
    /** @param request Request pointer. You can store this to keep the ptr alive but it is not recommended.
        @param statusCode HTTP response status code. -1 if request failed.
        @param errorMsg Error string. Empty string if request failed. */
    Signal3<HttpRequestPtr & ARG(request), int ARG(statusCode), const String & ARG(errorMsg)> Finished;

private:
    /// Constructed by HttpClient.
    HttpRequest(Framework *framework, int method, const String &url);
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

    /// Parse headers from response raw bytes.
    bool ParseHeaders();
    
    /// Called by HttpWorkQueue in main thread context.
    void EmitCompletion(HttpRequestPtr &self);
    /// Called by HttpWorkQueue in main thread context.
    void WriteStats(Http::Stats *stats);
    /// Invoked in main threa context if verbose is enabled.
    void DumpResponse(bool headers, bool body);

    /// Called by CurlWriteBody
    size_t WriteBody(void *buffer, uint size);
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

    // Framework for API/Urho access.
    Framework *framework_;

    // Outgoing/incomfing data structures
    Http::RequestData requestData_;
    Http::ResponseData responseData_;

    Urho3D::Mutex mutexExecute_;
    bool executing_;
    bool completed_;
    bool verbose_;

    static const Logger log;
};

}
