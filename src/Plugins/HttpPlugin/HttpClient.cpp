// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "HttpClient.h"
#include "HttpRequest.h"
#include "DebugAPI.h"
#include "DebugHud.h"

#include "Framework.h"
#include "ConsoleAPI.h"
#include "LoggingFunctions.h"

#include <UI/Text.h>

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
    httpHudPanel_.Reset();

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

Http::Stats *HttpClient::Stats() const
{
    return (queue_ ? queue_->stats_ : nullptr);
}

// Private API

HttpRequestPtr HttpClient::Create(int method, const String &url)
{
    return HttpRequestPtr(new HttpRequest(framework_, method, url));
}

HttpRequestPtr HttpClient::Create(int method, const String &url, const Vector<u8> &body, const String &contentType)
{
    HttpRequestPtr request(new HttpRequest(framework_, method, url));
    request->SetBody(body, contentType);
    return request;
}

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

bool HttpClient::Schedule(HttpRequestPtr request)
{
    if (!queue_)
        return false;
    queue_->Schedule(request);
    return true;
}

void HttpClient::Initialize()
{
    if (Stats())
    {
        framework_->Console()->RegisterCommand("httpStats", "Dump HTTP statistics to stdout", this, &HttpClient::DumpStats);
        if (!framework_->IsHeadless())
        {
            httpHudPanel_ = new HttpHudPanel(framework_, this, queue_);
            framework_->Debug()->Hud()->AddTab("HTTP", Urho3D::StaticCast<DebugHudPanel>(httpHudPanel_));
        }
    }
}

void HttpClient::Update(float frametime)
{
    if (queue_)
        queue_->Update(frametime);
}

void HttpClient::DumpStats() const
{
    if (Stats())
        Stats()->Dump();
}

/// @cond PRIVATE

HttpHudPanel::HttpHudPanel(Framework *framework, HttpClient *client, HttpWorkQueue *queue) :
    DebugHudPanel(framework),
    client_(client),
    queue_(queue),
    step_(1.f/30.f),
    t_(1.f)
{
}

SharedPtr<Urho3D::UIElement> HttpHudPanel::CreateImpl()
{
    return UIElementPtr(new Urho3D::Text(framework_->GetContext()));
}

void HttpHudPanel::UpdatePanel(float frametime, const SharedPtr<Urho3D::UIElement> &widget)
{
    Urho3D::Text *httpText = dynamic_cast<Urho3D::Text*>(widget.Get());
    if (!httpText || !client_->Stats())
        return;

    t_ += frametime;
    if (t_ >= step_)
    {
        t_ = 0.f;
        httpText->SetText(client_->Stats()->GetData());
    }
}

/// @endcond

}
