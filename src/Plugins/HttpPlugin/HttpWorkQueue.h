// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "HttpPluginApi.h"
#include "HttpPluginFwd.h"
#include "FrameworkFwd.h"

#include "LoggingFunctions.h"

#include <Engine/Container/RefCounted.h>
#include <Engine/Core/Thread.h>
#include <Engine/Core/Mutex.h>

namespace Tundra
{

/// HttpWorkQueue request
class HttpWorkQueue : public Urho3D::RefCounted
{
    /// @cond PRIVATE
    friend class HttpClient;
    friend class HttpWorkThread;
    /// @endcond

public:
    HttpWorkQueue();
    ~HttpWorkQueue();

    void Schedule(const HttpRequestPtr &request);

    uint NumPending();

private:
    void Erase(HttpRequestPtr request);
    void Erase(HttpRequest *request);

    void StartThreads(uint max);
    void StopThreads();

    /// Called by HttpClient
    void Update(float frametime);

    /// Called by HttpWorkThread
    HttpRequest *Next();
    void Completed(HttpRequest *request);

    float durationNoWork_;
    uint numMaxThreads_;
    HttpWorkThreadList threads_;

    Urho3D::Mutex mutexRequests_;
    HttpRequestPtrList requests_;

    Logger log;
};

/// @cond PRIVATE

class HttpWorkThread : public Urho3D::Thread
{
public:
    HttpWorkThread(HttpWorkQueue *queue);

    /// Urho3D::Thread
    void ThreadFunction() override;

private:
    HttpWorkQueue *queue_;

    bool process_;
};

/// @endcond

}
