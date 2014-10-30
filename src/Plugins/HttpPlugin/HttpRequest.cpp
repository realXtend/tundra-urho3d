// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "HttpRequest.h"

#include "LoggingFunctions.h"

#include <Engine/Core/WorkQueue.h>
#include <Engine/Core/StringUtils.h>

#include <curl/curl.h>
#include "http-parser/http_parser.h"

namespace Tundra
{

#define HTTP_INITIAL_BODY_SIZE (256*1024*1024)

// HttpRequest

HttpRequest::HttpRequest(Http::Method method, const String &url) :
    performing_(false),
    verbose_(false)
{
    requestData_.options[Options::Url] = Curl::Option(CURLOPT_URL, Variant(url));
    requestData_.options[Options::Url] = Curl::Option(static_cast<CURLoption>(Http::MethodCurlOption(method)), Http::MethodCurlOptionValue(method));
}

HttpRequest::~HttpRequest()
{
    Cleanup();
}

// Public API

void HttpRequest::SetVerbose(bool enabled)
{
    Urho3D::MutexLock m(mutexPerforming_);
    if (performing_)
    {
        LogError("HttpRequest::SetVerbose: Cannot set verbose to a running request.");
        return;
    }
    verbose_ = enabled;
}

// Request API

bool HttpRequest::SetBody(const Vector<u8> &body, const String &contentType)
{
    {
        Urho3D::MutexLock m(mutexPerforming_);
        if (performing_)
        {
            LogError("HttpRequest::SetBody: Cannot set body to a running request.");
            return false;
        }
        requestData_.bodyBytes = body;
    }
    return SetHeader("Content-Type", contentType);
}

bool HttpRequest::ClearBody()
{
    {
        Urho3D::MutexLock m(mutexPerforming_);
        if (performing_)
        {
            LogError("HttpRequest::ClearBody: Cannot set body to a running request.");
            return false;
        }
        requestData_.bodyBytes.Clear();
    }
    RemoveHeader("Content-Type");
    return true;
}

bool HttpRequest::SetBody(const String &body, const String &contentType)
{
    Vector<u8> data((const u8*)body.CString(), body.Length());
    return SetBody(data, contentType);
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
    Urho3D::MutexLock m(mutexPerforming_);
    if (performing_)
    {
        LogError("HttpRequest::AppendHeaders: Cannot append headers to a running request.");
        return false;
    }
    for(HttpHeaderMap::const_iterator iter = headers.begin(); iter != headers.end(); ++iter)
        SetHeaderInternal(iter->first, iter->second, false, false); // Do not lock inside SetHeaderInternal, already aquired above.
    return true;
}

bool HttpRequest::ClearHeaders()
{
    Urho3D::MutexLock m(mutexPerforming_);
    if (performing_)
    {
        LogError("HttpRequest::ClearHeaders: Cannot clear headers in a running request.");
        return false;
    }
    requestData_.headers.clear();
    return true;
}

bool HttpRequest::RemoveHeader(const String &name)
{
    return RemoveHeaderInternal(name, false);
}

// Response API

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
        Urho3D::MutexLock m(mutexPerforming_);
        if (performing_)
        {
            // @note Atm this function cant be invoked for a response
            LogError(String("HttpRequest::") + (respose ? "SetResponseHeader: Request is not completed yet" : "SetHeader: Cannot set a running requests headers"));
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
        Urho3D::MutexLock m(mutexPerforming_);
        if (performing_)
        {
            // @note Atm this function cant be invoked for a response
            LogError(String("HttpRequest::") + (respose ? "RemoveResponseHeader: Request is not completed yet" : "RemoveHeader: Cannot remove a running requests headers"));
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
        Urho3D::MutexLock m(mutexPerforming_);
        if (performing_)
        {
            LogError(String("HttpRequest::") + (respose ? "HasResponseHeader: Request is not completed yet" : "HasHeader: Cannot access a running requests headers"));
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
        Urho3D::MutexLock m(mutexPerforming_);
        if (performing_)
        {
            LogError(String("HttpRequest::") + (respose ? "ResponseHeader: Request is not completed yet" : "Header: Cannot access a running requests headers"));
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
        Urho3D::MutexLock m(mutexPerforming_);
        if (performing_)
        {
            LogError(String("HttpRequest::") + (respose ? "ResponseHeaderInt: Request is not completed yet" : "HeaderInt: Cannot access a running requests headers"));
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
        Urho3D::MutexLock m(mutexPerforming_);
        if (performing_)
        {
            LogError(String("HttpRequest::") + (respose ? "ResponseHeaderUInt: Request is not completed yet" : "HeaderUInt: Cannot access a running requests headers"));
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

bool HttpRequest::Prepare()
{
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

    // Standard options
    if (verbose_)
        curl_easy_setopt(requestData_.curlHandle, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(requestData_.curlHandle, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(requestData_.curlHandle, CURLOPT_FOLLOWLOCATION, 1L);

    // Write custom headers
    curl_slist *headers = requestData_.CreateCurlHeaders(verbose_);
    if (headers)
    {
        CURLcode res = curl_easy_setopt(requestData_.curlHandle, CURLOPT_HTTPHEADER, headers);
        if (res != CURLE_OK)
        {
            LogError(Urho3D::ToString("HttpRequest: Failed to initialze custom request headers: %s", curl_easy_strerror(res)));
            return false;
        }
    }
    
    // Write custom options
    for(Curl::OptionMap::ConstIterator iter = requestData_.options.Begin(); iter != requestData_.options.End(); ++iter)
    {
        if (iter->second_.value.GetType() == Urho3D::VAR_STRING)
        {
            CURLcode res = curl_easy_setopt(requestData_.curlHandle, iter->second_.option, iter->second_.value.GetString().CString());
            if (res != CURLE_OK)
            {
                LogError(Urho3D::ToString("HttpRequest: Failed to initialze request option '%s'='%s': %s", iter->first_.CString(), iter->second_.value.GetString().CString(), curl_easy_strerror(res)));
                return false;
            }
            else if (verbose_)
                PrintRaw(Urho3D::ToString("  %s: %s", iter->first_.CString(), iter->second_.value.GetString().CString()));
        }
        else
            LogError("Unkown variant for curl_easy_setopt");
    }
    return true;
}

void HttpRequest::Perform()
{
    mutexPerforming_.Acquire();
    // @todo mark failed on false
    performing_ = Prepare();
    mutexPerforming_.Release();
    if (!performing_)
        return;

    Urho3D::Timer timer;
    CURLcode res = curl_easy_perform(requestData_.curlHandle);
    if (res != CURLE_OK)
        LogError(Urho3D::ToString("HttpRequest: Failed to initialze request %s", curl_easy_strerror(res)));
    requestData_.msecSpent = timer.GetMSec(false);

    // Compact unused bytes from input buffers
    responseData_.headersBytes.Compact();
    responseData_.bodyBytes.Compact();

    if (verbose_)
        DumpResponse(true, true);

    Cleanup();

    mutexPerforming_.Acquire();
    performing_ = false;
    mutexPerforming_.Release();
}

void HttpRequest::Cleanup()
{
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

void HttpRequest::DumpResponse(bool headers, bool body)
{
    if (headers || body)
    {
        String str;
        str.AppendWithFormat("\nHttpRequest:\n  Url: %s\n  Completed in: %d msec\n\n", requestData_.OptionValueString(Options::Url).CString(), requestData_.msecSpent);
        PrintRaw(str);
    }

    if (headers)
    {
        String str;
        str.AppendWithFormat("HEADERS %d bytes", responseData_.headersBytes.Size());
        str.Append("\n-------------------------------------------------------------------------------\n");
        //str.Append((const char*)&responseData_.HeadersBytes[0], responseData_.HeadersBytes.Size());
        //str.Append("\nPARSED HEADERS\n");
        for (HttpHeaderMap::const_iterator iter = responseData_.headers.begin(); iter != responseData_.headers.end(); ++iter)
            str.AppendWithFormat("'%s' '%s'\n", iter->first.CString(), iter->second.CString());
        str.Append("\n");
        PrintRaw(str);
    }
    if (body)
    {
        String str;
        str.AppendWithFormat("BODY %d bytes", responseData_.bodyBytes.Size());
        str.Append("\n-------------------------------------------------------------------------------\n");
        str.Append((const char*)&responseData_.bodyBytes[0], responseData_.bodyBytes.Size());
        str.Append("\n\n");
        PrintRaw(str);
    }
}

size_t HttpRequest::ReadBody(void *buffer, uint size)
{
    /* First body bytes are being received. Parse headers to determine exact size of
       incoming data. This way we don't have to resize the body buffer mid flight. */
    if (responseData_.bodyBytes.Empty() && !responseData_.headersBytes.Empty())
    {
        http_parser_settings settings;
        InitHttpParserSettings(&settings);

        http_parser parser;
        parser.data = this;

        http_parser_init(&parser, HTTP_RESPONSE);
        http_parser_execute(&parser, &settings, (const char*)&responseData_.headersBytes[0], (size_t)responseData_.headersBytes.Size());

        http_errno err = HTTP_PARSER_ERRNO(&parser);
        if (err != HPE_OK)
        {
            LogError(Urho3D::ToString("HttpRequest: Error while parsing headers: %s %s", http_errno_name(err), http_errno_description(err)));
            return 0; // Propagates a CURLE_WRITE_ERROR and aborts transfer
        }
        // Headers have been parsed. Reserve bodyBytes_ to "Content-Length" size.
        responseData_.bodyBytes.Reserve(HeaderUIntInternal("Content-Length", HTTP_INITIAL_BODY_SIZE, false));
    }

    Vector<u8> data(static_cast<u8*>(buffer), size);
    responseData_.bodyBytes.Push(data);
    return size;
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
        LogError("HttpRequest: Header parsing error. Value streamed prior to field.");
        return 1;
    }
    responseData_.headers[responseData_.lastHeaderField] = String(buffer, size);
    responseData_.lastHeaderField = "";
    return 0; // http parser 0 = no error, continue parsing
}

}
