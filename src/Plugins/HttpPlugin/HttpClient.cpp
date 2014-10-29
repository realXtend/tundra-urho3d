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
    {
        queue_ = new HttpWorkQueue();

        // Debug
        //Get("http://meshmoon.data.s3.amazonaws.com/avatars/kate/kate.png"); //->SetVerbose(true);
    }
    else
        LogError(String("HttpClient: Failed to initialize curl: ") + curl_easy_strerror(err));
}

HttpClient::~HttpClient()
{
    // Stop all threads and cleanup curl requests
    queue_.Reset();

    // Cleanup curl
    curl_global_cleanup();    
}

void HttpClient::Update(float frametime)
{
    if (queue_)
        queue_->Update(frametime);
}

HttpRequestPtr HttpClient::Get(const String &url)
{
    if (!queue_)
        return HttpRequestPtr();

    HttpRequestPtr request(new HttpRequest(url));
    queue_->Schedule(request);
    return request;
}

}
