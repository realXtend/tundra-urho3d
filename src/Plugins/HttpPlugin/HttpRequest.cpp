// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "HttpRequest.h"

#include "Framework.h"
#include "JSON.h"

#include <Engine/Core/WorkQueue.h>
#include <Engine/Core/StringUtils.h>
#include <Engine/IO/FileSystem.h>
#include <Engine/IO/File.h>

#include <curl/curl.h>
#include "HttpParser/http_parser.h"

namespace Tundra
{

#define HTTP_INITIAL_BODY_SIZE (256*1024)

// HttpRequest

HttpRequest::HttpRequest(Framework* framework, int method, const String &url) :
    framework_(framework),
    log("HttpRequest"),
    executing_(false),
    verbose_(false),
    completed_(false)
{
    requestData_.options[Options::Url] = Curl::Option(CURLOPT_URL, Variant(url));
    requestData_.options[Options::Method] = Curl::Option(static_cast<CURLoption>(Http::Method::CurlOption(method)), Http::Method::CurlOptionValue(method));
}

HttpRequest::~HttpRequest()
{
    Cleanup();
}

// Public API

bool HttpRequest::HasStarted()
{
    Urho3D::MutexLock m(mutexExecute_);
    return (executing_ || completed_);
}

bool HttpRequest::IsExecuting()
{
    Urho3D::MutexLock m(mutexExecute_);
    return executing_;
}

bool HttpRequest::HasCompleted()
{
    Urho3D::MutexLock m(mutexExecute_);
    return completed_;
}

void HttpRequest::SetVerbose(bool enabled)
{
    Urho3D::MutexLock m(mutexExecute_);
    if (executing_)
    {
        log.Error("SetVerbose: Cannot set verbose to a running request.");
        return;
    }
    verbose_ = enabled;
}

// Request API

String HttpRequest::Method() const
{
    // Options are read but not written to in the work thread. Don't lock.
    Curl::OptionMap::ConstIterator option = requestData_.options.Find(Options::Method);
    if (option != requestData_.options.End() && option->second_.value.GetType() == Urho3D::VAR_INT)
        return Http::Method::ToString(option->second_.value.GetInt());
    return "";
}

String HttpRequest::Url() const
{
    // Options are read but not written to in the work thread. Don't lock.
    Curl::OptionMap::ConstIterator option = requestData_.options.Find(Options::Url);
    if (option != requestData_.options.End() && option->second_.value.GetType() == Urho3D::VAR_STRING)
        return option->second_.value.GetString();
    return "";
}

String HttpRequest::Error()
{
    if (!HasCompleted())
        return "";
    return requestData_.error;
}

bool HttpRequest::SetBody(const Vector<u8> &body, const String &contentType)
{
    {
        Urho3D::MutexLock m(mutexExecute_);
        if (executing_)
        {
            log.Error("SetBody: Cannot set body to a running request.");
            return false;
        }
        /* @note We allow setting empty body with a Content-Type
           as it might be perfectly valid situation for POS/PUT etc.
           for certain servers/applications. */
        requestData_.bodyBytes = body;
    }
    return SetHeader(Http::Header::ContentType, contentType);
}

bool HttpRequest::SetBody(const String &body, const String &contentType)
{
    Vector<u8> data((const u8*)body.CString(), body.Length());
    return SetBody(data, contentType);
}

bool HttpRequest::SetBodyJSON(const JSONValue &json)
{
    String data = json.ToString(0);
    return SetBodyJSON(data);
}

bool HttpRequest::SetBodyJSON(const String &json)
{
    return SetBody(json, Http::ContentType::JSON);
}

bool HttpRequest::ClearBody()
{
    {
        Urho3D::MutexLock m(mutexExecute_);
        if (executing_)
        {
            log.Error("ClearBody: Cannot set body to a running request.");
            return false;
        }
        requestData_.bodyBytes.Clear();
    }
    RemoveHeader(Http::Header::ContentType);
    return true;
}

bool HttpRequest::SetHeader(const String &name, const String &value)
{
    return SetHeaderInternal(name, value, false);
}

bool HttpRequest::SetHeader(const String &name, int value)
{
    return SetHeader(name, Urho3D::ToString("%d", value));
}

bool HttpRequest::SetHeader(const String &name, uint value)
{
    return SetHeader(name, Urho3D::ToString("%d", value));
}

bool HttpRequest::AppendHeaders(const HttpHeaderMap &headers)
{
    Urho3D::MutexLock m(mutexExecute_);
    if (executing_)
    {
        log.Error("AppendHeaders: Cannot append headers to a running request.");
        return false;
    }
    for(HttpHeaderMap::const_iterator iter = headers.begin(); iter != headers.end(); ++iter)
        SetHeaderInternal(iter->first, iter->second, false, false); // Do not lock inside SetHeaderInternal, already aquired above.
    return true;
}

bool HttpRequest::ClearHeaders()
{
    Urho3D::MutexLock m(mutexExecute_);
    if (executing_)
    {
        log.Error("ClearHeaders: Cannot clear headers from a running request.");
        return false;
    }
    requestData_.headers.clear();
    return true;
}

bool HttpRequest::RemoveHeader(const String &name)
{
    return RemoveHeaderInternal(name, false);
}

bool HttpRequest::SetCacheFile(const String &filepath, bool useLastModified)
{
    Urho3D::MutexLock m(mutexExecute_);
    if (executing_)
    {
        log.Error("SetCacheFile: Cannot set cache file to a running request.");
        return false;
    }
    requestData_.cacheFile = Urho3D::GetInternalPath(filepath);

    // Read 'If-Modified-Since' from cache file
    if (useLastModified && framework_->GetSubsystem<Urho3D::FileSystem>()->FileExists(requestData_.cacheFile))
    {
        uint epoch = framework_->GetSubsystem<Urho3D::FileSystem>()->GetLastModifiedTime(requestData_.cacheFile);
        String lastModifiedHttpDate = Http::LocalEpochToHttpDate(static_cast<time_t>(epoch)); // GetLastModifiedTime returns local timezoned epoch
        if (!lastModifiedHttpDate.Empty())
            SetHeaderInternal(Http::Header::IfModifiedSince, lastModifiedHttpDate, false, false); // Do not lock inside SetHeaderInternal, already aquired above.
    }
    return true;
}

bool HttpRequest::SetCacheFile(const String &filepath, const String &lastModifiedHttpDate)
{
    Urho3D::MutexLock m(mutexExecute_);
    if (executing_)
    {
        log.Error("SetCacheFile: Cannot set cache file to a running request.");
        return false;
    }
    requestData_.cacheFile = Urho3D::GetInternalPath(filepath);

    if (!lastModifiedHttpDate.Empty())
        SetHeaderInternal(Http::Header::IfModifiedSince, lastModifiedHttpDate, false, false); // Do not lock inside SetHeaderInternal, already aquired above.
    return true;
}

// Response API

int HttpRequest::StatusCode()
{
    if (!HasCompleted())
        return -1;
    return responseData_.status;
}

String HttpRequest::Status()
{
    if (!HasCompleted())
        return "";
    return responseData_.statusText;
}

uint HttpRequest::DurationMSec()
{
    if (!HasCompleted())
        return 0;
    return requestData_.msecSpent;
}

float HttpRequest::AvertageDownloadSpeed()
{
    if (!HasCompleted())
        return 0.f;
    return static_cast<float>(responseData_.bytesPerSec);
}

const Vector<u8> bodyNotReady;

const Vector<u8> &HttpRequest::ResponseBody()
{
    if (!HasCompleted())
        return bodyNotReady;
    return responseData_.bodyBytes;
}

bool HttpRequest::ResponseBodyCopyTo(Vector<u8> &dest)
{
    if (!HasCompleted() || responseData_.bodyBytes.Empty())
        return false;
    dest.Resize(responseData_.bodyBytes.Size());
    memcpy(&dest[0], &responseData_.bodyBytes[0], responseData_.bodyBytes.Size());
    // @todo Below option is probably slower than memcpy?
    //dest.Insert(dest.Begin(), responseData_.bodyBytes.Begin(), responseData_.bodyBytes.End());
    return true; //(dest.Size() == responseData_.bodyBytes.Size());
}

Vector<u8> HttpRequest::ResponseBodyCopy()
{
    if (!HasCompleted())
        return Vector<u8>();
    return responseData_.bodyBytes;
}

bool HttpRequest::HasResponseHeader(const String &name)
{
    return HasHeaderInternal(name, true);
}

String HttpRequest::ResponseHeader(const String &name)
{
    return HeaderInternal(name, true);
}

int HttpRequest::ResponseHeaderInt(const String &name, int defaultValue)
{
    return HeaderIntInternal(name, defaultValue, true);
}

uint HttpRequest::ResponseHeaderUInt(const String &name, uint defaultValue)
{
    return HeaderUIntInternal(name, defaultValue, true);
}

// Curl callbacks

size_t CurlReadBody(void *buffer, size_t size, size_t items, void *data)
{
    HttpRequest* transfer = reinterpret_cast<HttpRequest*>(data);
    return transfer->ReadBody(buffer, static_cast<uint>(size * items));
}

size_t CurlReadHeaders(char *buffer, size_t size, size_t items, void *data)
{
    HttpRequest* transfer = reinterpret_cast<HttpRequest*>(data);
    return transfer->ReadHeaders(buffer, static_cast<uint>(size * items));
}

// HTTP parser callbacks

int HttpParserStatus(http_parser *p, const char *buf, size_t len)
{
    HttpRequest* transfer = reinterpret_cast<HttpRequest*>(p->data);
    return transfer->ReadStatus(buf, static_cast<uint>(len));
}

int HttpParserHeaderField(http_parser *p, const char *buf, size_t len)
{
    HttpRequest* transfer = reinterpret_cast<HttpRequest*>(p->data);
    return transfer->ReadHeaderField(buf, static_cast<uint>(len));
}

int HttpParserHeaderValue(http_parser *p, const char *buf, size_t len)
{
    HttpRequest* transfer = reinterpret_cast<HttpRequest*>(p->data);
    return transfer->ReadHeaderValue(buf, static_cast<uint>(len));
}

int HttpParserHeadersComplete(http_parser* /*p*/)
{
    /* This will tell parser to stop parsing after headers.
       Body will be consumed in CurlReadBody after headers
       parsing is completed. */
    return 1;
}

void InitHttpParserSettings(http_parser_settings *settings)
{
    settings->on_message_begin      = 0;
    settings->on_url                = 0;
    settings->on_body               = 0;
    settings->on_message_complete   = 0;
    settings->on_status             = HttpParserStatus;
    settings->on_header_field       = HttpParserHeaderField;
    settings->on_header_value       = HttpParserHeaderValue;
    settings->on_headers_complete   = HttpParserHeadersComplete;
}

// Private API

bool HttpRequest::SetHeaderInternal(const String &name, const String &value, bool respose, bool lock)
{
    if (lock)
    {
        Urho3D::MutexLock m(mutexExecute_);
        if (executing_)
        {
            // @note Atm this function cant be invoked for a response
            log.Error(String(respose ? "SetResponseHeader: Request is not completed yet" : "SetHeader: Cannot set a running requests headers"));
            return false;
        }
    }
    HttpHeaderMap &headers = (respose ? responseData_.headers : requestData_.headers);
    headers[name] = value;
    return true;
}

bool HttpRequest::RemoveHeaderInternal(const String &name, bool respose, bool lock)
{
    if (lock)
    {
        Urho3D::MutexLock m(mutexExecute_);
        if (executing_)
        {
            // @note Atm this function cant be invoked for a response
            log.Error(String(respose ? "RemoveResponseHeader: Request is not completed yet" : "RemoveHeader: Cannot remove a running requests headers"));
            return false;
        }
    }
    HttpHeaderMap &headers = (respose ? responseData_.headers : requestData_.headers);
    HttpHeaderMap::iterator found = headers.find(name);
    while (found != headers.end())
    {
        headers.erase(found);
        return true;
    }
    return false;
}

bool HttpRequest::HasHeaderInternal(const String &name, bool respose, bool lock)
{
    if (lock)
    {
        Urho3D::MutexLock m(mutexExecute_);
        if (executing_)
        {
            log.Error(String(respose ? "HasResponseHeader: Request is not completed yet" : "HasHeader: Cannot access a running requests headers"));
            return false;
        }
    }
    const HttpHeaderMap &headers = (respose ? responseData_.headers : requestData_.headers);
    return (headers.find(name.CString()) != headers.end());
}

String HttpRequest::HeaderInternal(const String &name, bool respose, bool lock)
{
    if (lock)
    {
        Urho3D::MutexLock m(mutexExecute_);
        if (executing_)
        {
            log.Error(String(respose ? "ResponseHeader: Request is not completed yet" : "Header: Cannot access a running requests headers"));
            return "";
        }
    }
    const HttpHeaderMap &headers = (respose ? responseData_.headers : requestData_.headers);
    HttpHeaderMap::const_iterator header = headers.find(name.CString());
    if (header != headers.end())
        return header->second;
    return "";
}

int HttpRequest::HeaderIntInternal(const String &name, int defaultValue, bool respose, bool lock)
{
    if (lock)
    {
        Urho3D::MutexLock m(mutexExecute_);
        if (executing_)
        {
            log.Error(String(respose ? "ResponseHeaderInt: Request is not completed yet" : "HeaderInt: Cannot access a running requests headers"));
            return 0;
        }
    }
    const HttpHeaderMap &headers = (respose ? responseData_.headers : requestData_.headers);

    String valueStr;
    HttpHeaderMap::const_iterator header = headers.find(name.CString());
    if (header != headers.end())
        valueStr = header->second;

    if (valueStr.Empty())
        return defaultValue;
    int value = Urho3D::ToInt(valueStr);
    // 0 is can also mean error in parsing a int
    if (value == 0 && valueStr[0] != '0')
        return defaultValue;
    return value;
}

uint HttpRequest::HeaderUIntInternal(const String &name, uint defaultValue, bool respose, bool lock)
{
    if (lock)
    {
        Urho3D::MutexLock m(mutexExecute_);
        if (executing_)
        {
            log.Error(String((respose ? "ResponseHeaderUInt: Request is not completed yet" : "HeaderUInt: Cannot access a running requests headers")));
            return 0;
        }
    }
    const HttpHeaderMap &headers = (respose ? responseData_.headers : requestData_.headers);

    String valueStr;
    HttpHeaderMap::const_iterator header = headers.find(name.CString());
    if (header != headers.end())
        valueStr = header->second;

    if (valueStr.Empty())
        return defaultValue;
    uint value = Urho3D::ToUInt(valueStr);
    // 0 is can also mean error in parsing a uint
    if (value == 0 && valueStr[0] != '0')
        return defaultValue;
    return value;
}

void HttpRequest::Perform()
{
    // @note Invoked in worker thread context
    {
        Urho3D::MutexLock m(mutexExecute_);
        executing_ = Prepare();
        completed_ = !executing_;
    }
    if (!executing_)
        return;

    Urho3D::Timer timer;
    CURLcode res = curl_easy_perform(requestData_.curlHandle);
    if (res != CURLE_OK)
    {
        requestData_.error = curl_easy_strerror(res);
        log.ErrorF("Failed to initialze request: %s", requestData_.error.CString());
    }
    requestData_.msecSpent = timer.GetMSec(false);

    /* Compact unused bytes from input buffers. bodyBytes should not have any free
       capacity if Content-Lenght header was provided by the server and correct. */
    responseData_.headersBytes.Compact();
    responseData_.bodyBytes.Compact();

    /// @todo Don't run if request was aborted. Does this error check suffice?
    if (res == CURLE_OK)
    {
        // Read response information
        if (curl_easy_getinfo(requestData_.curlHandle, CURLINFO_RESPONSE_CODE, &responseData_.status) != CURLE_OK)
            log.ErrorF("Failed to read response status code");
        if (curl_easy_getinfo(requestData_.curlHandle, CURLINFO_SPEED_DOWNLOAD, &responseData_.bytesPerSec) != CURLE_OK)
            log.ErrorF("Failed to read response download speed");

        // Only perform this sanity check for 200 OK. As eg. Not Modified might not return the true bytes in header.
        if (responseData_.status == 200)
        {
            uint contentLenght = HeaderUIntInternal(Http::Header::ContentLength, 0, true, false);
            if (contentLenght > 0 && responseData_.bodyBytes.Size() != contentLenght)
                log.WarningF("Content-Lenght %d header does not match size of %d read bytes for %s. Data might be incomplete.", contentLenght, responseData_.bodyBytes.Size(), requestData_.options[Options::Url].value.GetString().CString());

            // Write cache file if designated. File will be written regardless if server sent a 'Last-Modified' header.
            if (!requestData_.cacheFile.Empty())
            {
                String lastModified = HeaderInternal(Http::Header::LastModified, true, false);

                /** @todo Check if this is safe. We are in a secondary thread here. But it *should* be guaranteed by AssetAPI and the HttpClient that
                    one request is ongoing at a time to a unique URL. The URL designates the filepath where we are writing. Framework and Urho3D
                    Engine and its subsystem are guaranteed to be up while any worker thread is running (exit blocks waiting for workers to finish).
                    Still this is dicy, it would be nice to execute the disk write in thread but if not safe it can be moved to main thread. */
                Urho3D::File file(framework_->GetContext(), requestData_.cacheFile, Urho3D::FILE_WRITE);
                if (file.IsOpen())
                {
                    uint bodySize = responseData_.bodyBytes.Size();
                    if (file.Write(&responseData_.bodyBytes[0], bodySize) == bodySize)
                    {
                        file.Close(); // File shared ptr will close when gets out of scope. Lets just do it here before modifying below last modified on the file.
                        time_t epoch = Http::HttpDateToUtcEpoch(lastModified);
                        if (epoch > 0) // SetLastModifiedTime converts utc epoch correctly to local
                            framework_->GetSubsystem<Urho3D::FileSystem>()->SetLastModifiedTime(requestData_.cacheFile, static_cast<uint>(epoch));
                    }
                }
            }
        }
        else if (responseData_.status == 304)
        {
            ParseHeaders();

            /// See above 200 OK file access comment
            Urho3D::File file(framework_->GetContext(), requestData_.cacheFile, Urho3D::FILE_READ);
            if (file.IsOpen())
            {
                uint fileSize = file.GetSize();
                responseData_.bodyBytes.Resize(fileSize);
                if (file.Read(&responseData_.bodyBytes[0], fileSize) != fileSize)
                {
                    log.Error("Failed to read cached disk source for '304 not Modified' response.");
                    responseData_.bodyBytes.Clear();
                }
            }
        }
    }

    Cleanup();

    {
        Urho3D::MutexLock m(mutexExecute_);
        executing_ = false;
        completed_ = true;
    }
}

bool HttpRequest::Prepare()
{
    // @note Invoked in worker thread context

    /* @todo If we want to reuse the curl handle, its possible,
       but not within the same HttpRequest. They would need to be pooled somewhere.
       Research if this would make a perf difference. */
    if (!requestData_.curlHandle)
        requestData_.curlHandle = curl_easy_init();
    if (!requestData_.curlHandle)
        return false;

    if (verbose_)
        PrintRaw("HttpRequest::Prepare\n");

    // Reading response stream
    curl_easy_setopt(requestData_.curlHandle, CURLOPT_WRITEFUNCTION, CurlReadBody);
    curl_easy_setopt(requestData_.curlHandle, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(requestData_.curlHandle, CURLOPT_HEADERFUNCTION, CurlReadHeaders);
    curl_easy_setopt(requestData_.curlHandle, CURLOPT_HEADERDATA, this);

    // Logging to stdout. Only do this when loglevel is debug, it gets pretty messy.
    if (verbose_ && IsLogLevelEnabled(LogLevelDebug))
        curl_easy_setopt(requestData_.curlHandle, CURLOPT_VERBOSE, 1L);

    // Standard options
    curl_easy_setopt(requestData_.curlHandle, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(requestData_.curlHandle, CURLOPT_FOLLOWLOCATION, 1L);

    // Write custom headers
    curl_slist *headers = requestData_.CreateCurlHeaders(verbose_);
    if (headers)
    {
        CURLcode res = curl_easy_setopt(requestData_.curlHandle, CURLOPT_HTTPHEADER, headers);
        if (res != CURLE_OK)
        {
            requestData_.error = curl_easy_strerror(res);
            log.ErrorF("Failed to initialze custom request headers: %s", requestData_.error.CString());
            return false;
        }
    }
    
    // Write custom options
    for(Curl::OptionMap::ConstIterator iter = requestData_.options.Begin(); iter != requestData_.options.End(); ++iter)
    {
        const Curl::Option &option = iter->second_;
        const Variant &value = option.value;
        if (value.GetType() == Urho3D::VAR_STRING)
        {
            CURLcode res = curl_easy_setopt(requestData_.curlHandle, option.option, value.GetString().CString());
            if (res != CURLE_OK)
            {
                requestData_.error = curl_easy_strerror(res);
                log.ErrorF("Failed to initialze request option '%s'='%s': %s", iter->first_.CString(), value.GetString().CString(), requestData_.error.CString());
                return false;
            }
        }
        else if (value.GetType() == Urho3D::VAR_INT)
        {
            CURLcode res = curl_easy_setopt(requestData_.curlHandle, option.option, value.GetInt());
            if (res != CURLE_OK)
            {
                requestData_.error = curl_easy_strerror(res);
                log.ErrorF("Failed to initialze request option '%s'=%d: %s", iter->first_.CString(), value.GetInt(), requestData_.error.CString());
                return false;
            }
        }
        else
            log.ErrorF("Unkown variant type %d for curl_easy_setopt", iter->second_.value.GetType());

        if (verbose_)
            PrintRaw(Urho3D::ToString("  %s: %s\n", iter->first_.CString(), value.ToString().CString()));
    }
    return true;
}

void HttpRequest::Cleanup()
{
    // @note Invoked in worker thread context

    if (requestData_.curlHandle)
    {
        curl_easy_cleanup(requestData_.curlHandle);
        requestData_.curlHandle = 0;
    }
    if (requestData_.curlHeaders)
    {
        curl_slist_free_all(requestData_.curlHeaders);
        requestData_.curlHeaders = 0;
    }
}

void HttpRequest::EmitCompletion(HttpRequestPtr &self)
{
    // @note Invoked in main thread context

    if (verbose_)
        DumpResponse(true, true);

    Finished.Emit(self, responseData_.status, requestData_.error);
}

bool ShouldPrintBody(const String &contentType)
{
    return (!contentType.Empty() && (contentType.Contains("text", false) || contentType.Contains("xml") || contentType.Contains("json")));
}

void HttpRequest::DumpResponse(bool headers, bool body)
{
    if (headers || body)
    {
        String str;
        str.Append("\nHttpRequest\n");
        str.Append("-------------------------------------------------------------------------------\n");
        str.AppendWithFormat("  Url     : %s\n", requestData_.OptionValueString(Options::Url).CString());
        str.AppendWithFormat("  Status  : %d %s\n", responseData_.status, responseData_.statusText.CString());
        str.AppendWithFormat("  Headers : %d bytes\n", responseData_.headersBytes.Size());
        str.AppendWithFormat("  Body    : %d bytes\n", responseData_.bodyBytes.Size());
        str.AppendWithFormat("  Spent   : %d msec\n", requestData_.msecSpent);
        if (responseData_.bytesPerSec > 1.0)
        str.AppendWithFormat("  Speed   : %f kb/sec\n", responseData_.bytesPerSec / 1024);
        if (!requestData_.error.Empty())
            str.AppendWithFormat("  Error   : %s\n", requestData_.error.CString());
        str.Append("\n");
        PrintRaw(str);
    }

    if (headers)
    {
        String str;
        for (HttpHeaderMap::const_iterator iter = responseData_.headers.begin(); iter != responseData_.headers.end(); ++iter)
            str.AppendWithFormat("'%s' '%s'\n", iter->first.CString(), iter->second.CString());
        str.Append("\n");
        PrintRaw(str);
    }
    if (body && responseData_.status != 304)
    {
        String str;
        String contentType = HeaderInternal(Http::Header::ContentType, true, false);
        if (ShouldPrintBody(contentType))
        {
            str.Append((const char*)&responseData_.bodyBytes[0], responseData_.bodyBytes.Size());
            str.Append("\n");
        }
        else
            str.AppendWithFormat("Not printing Content-Type '%s' body to stdout\n", contentType.CString());
        str.Append("\n");
        PrintRaw(str);
    }
}

size_t HttpRequest::ReadBody(void *buffer, uint size)
{
    /* First body bytes are being received. Parse headers to determine exact size of
       incoming data. This way we don't have to resize the body buffer mid flight. */
    if (responseData_.bodyBytes.Empty() && !responseData_.headersBytes.Empty())
    {
        if (!ParseHeaders())
            return 0; // Propagates a CURLE_WRITE_ERROR and aborts transfer

        // Headers have been parsed. Reserve bodyBytes_ to "Content-Length" size.
        responseData_.bodyBytes.Reserve(HeaderUIntInternal(Http::Header::ContentLength, HTTP_INITIAL_BODY_SIZE, true, false));
    }

    Vector<u8> data(static_cast<u8*>(buffer), size);
    responseData_.bodyBytes.Push(data);
    return size;
}

bool HttpRequest::ParseHeaders()
{
    if (responseData_.headersBytes.Empty())
        return false;

    http_parser_settings settings;
    InitHttpParserSettings(&settings);

    http_parser parser;
    parser.data = this;

    http_parser_init(&parser, HTTP_RESPONSE);
    http_parser_execute(&parser, &settings, (const char*)&responseData_.headersBytes[0], (size_t)responseData_.headersBytes.Size());

    http_errno err = HTTP_PARSER_ERRNO(&parser);
    if (err != HPE_OK)
    {
        requestData_.error = Urho3D::ToString("%s %s", http_errno_name(err), http_errno_description(err));
        log.Error("Error while parsing headers: " + requestData_.error);
        return false; // Propagates a CURLE_WRITE_ERROR and aborts transfer
    }
    return true;
}

size_t HttpRequest::ReadHeaders(void *buffer, uint size)
{
    if (responseData_.headersBytes.Empty())
        responseData_.headersBytes.Reserve(HTTP_MAX_HEADER_SIZE);

    Vector<u8> data(static_cast<u8*>(buffer), size);
    responseData_.headersBytes.Push(data);
    return size;
}

int HttpRequest::ReadStatus(const char *buffer, uint size)
{
    responseData_.statusText = String(buffer, size);
    return 0; // http parser 0 = no error, continue parsing
}

int HttpRequest::ReadHeaderField(const char *buffer, uint size)
{
    responseData_.lastHeaderField = String(buffer, size);
    return 0; // http parser 0 = no error, continue parsing
}

int HttpRequest::ReadHeaderValue(const char *buffer, uint size)
{
    if (responseData_.lastHeaderField.Empty())
    {
        log.Error("Header parsing error. Value streamed prior to field.");
        return 1;
    }
    responseData_.headers[responseData_.lastHeaderField] = String(buffer, size);
    responseData_.lastHeaderField = "";
    return 0; // http parser 0 = no error, continue parsing
}

}
