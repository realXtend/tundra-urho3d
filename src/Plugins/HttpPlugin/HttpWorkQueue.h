// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "HttpPluginApi.h"
#include "HttpPluginFwd.h"
#include "FrameworkFwd.h"

#include "LoggingFunctions.h"

#include <Urho3D/Container/RefCounted.h>
#include <Urho3D/Core/Thread.h>
#include <Urho3D/Core/Mutex.h>

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
    /// @todo Aborting ongoing/scheduled requests

    uint NumPending();

private:
    /// @note You have to ensure mutexCompleted_ is locked prior to calling this function.
    HttpRequestPtrList::Iterator FindExecuting(HttpRequest *request);

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
    Urho3D::Mutex mutexCompleted_;

    /// Waiting requests.
    /** Accessed from multiple thread,
        protected by mutexRequests_. */
    HttpRequestPtrList requests_;

    /// Completed requests.
    /** Accessed from multiple thread,
        protected by mutexCompleted_. */
    HttpRequestPtrList completed_;

    /// Currently executing requests.
    HttpRequestPtrList executing_;

    /** Newly created requests that will be moved
        to requests_ in the next frame update.
        This protects worker threads from starting
        the request while main thread is still
        setting body/headers etc. */
    HttpRequestPtrList created_;

    /// Stats
    Http::Stats *stats_;

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
};

/// @endcond

}
