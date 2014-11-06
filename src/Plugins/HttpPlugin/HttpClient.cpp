// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "HttpClient.h"
#include "HttpRequest.h"

#include "Framework.h"
#include "LoggingFunctions.h"

#include <curl/curl.h>

namespace Tundra
{

HttpClient::HttpClient(Framework *framework) :
    framework_(framework)
{
    CURLcode err = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (err == CURLE_OK)
        queue_ = new HttpWorkQueue();
    else
        LogErrorF("[HttpClient] Failed to initialize curl: %s", curl_easy_strerror(err));
}

HttpClient::~HttpClient()
{
    // Stop all threads and cleanup curl requests
    queue_.Reset();

    // Cleanup curl
    curl_global_cleanup();    
}

// Public API

HttpRequestPtr HttpClient::Get(const String &url)
{
    return Schedule(Http::Method::Get, url);
}

HttpRequestPtr HttpClient::Head(const String &url)
{
    return Schedule(Http::Method::Head, url);
}

HttpRequestPtr HttpClient::Options(const String &url)
{
    return Schedule(Http::Method::Options, url);
}

HttpRequestPtr HttpClient::Post(const String &url, const Vector<u8> &body, const String &contentType)
{
    return Schedule(Http::Method::Post, url, body, contentType);
}

HttpRequestPtr HttpClient::Post(const String &url, const String &body, const String &contentType)
{
    Vector<u8> data((const u8*)body.CString(), body.Length());
    return Post(url, data, contentType);
}

HttpRequestPtr HttpClient::Put(const String &url, const Vector<u8> &body, const String &contentType)
{
    return Schedule(Http::Method::Put, url, body, contentType);
}

HttpRequestPtr HttpClient::Put(const String &url, const String &body, const String &contentType)
{
    Vector<u8> data((const u8*)body.CString(), body.Length());
    return Put(url, data, contentType);
}

HttpRequestPtr HttpClient::Patch(const String &url, const Vector<u8> &body, const String &contentType)
{
    return Schedule(Http::Method::Patch, url, body, contentType);
}

HttpRequestPtr HttpClient::Patch(const String &url, const String &body, const String &contentType)
{
    Vector<u8> data((const u8*)body.CString(), body.Length());
    return Patch(url, data, contentType);
}

HttpRequestPtr HttpClient::Delete(const String &url)
{
    return Schedule(Http::Method::Delete, url);
}

// Private API

HttpRequestPtr HttpClient::Schedule(int method, const String &url)
{
    if (!queue_)
        return HttpRequestPtr();

    HttpRequestPtr request(new HttpRequest(framework_, method, url));
    queue_->Schedule(request);
    return request;
}

HttpRequestPtr HttpClient::Schedule(int method, const String &url, const Vector<u8> &body, const String &contentType)
{
    if (!queue_)
        return HttpRequestPtr();

    HttpRequestPtr request(new HttpRequest(framework_, method, url));
    request->SetBody(body, contentType);
    queue_->Schedule(request);
    return request;
}

void HttpClient::Update(float frametime)
{
    if (queue_)
        queue_->Update(frametime);
}

}
