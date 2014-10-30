// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "HttpWorkQueue.h"
#include "HttpRequest.h"

#include "Framework.h"
#include "LoggingFunctions.h"
#include "Math/MathFunc.h"

#include <Engine/Core/ProcessUtils.h>
#include <Engine/Core/StringUtils.h>

namespace Tundra
{

// HttpWorkQueue

const float DurationKeepAliveThreads = 10.f;

HttpWorkQueue::HttpWorkQueue() :
    durationNoWork_(0.f)
{
    uint numCPUs = Urho3D::GetNumPhysicalCPUs();
    numMaxThreads_ = FloorInt(static_cast<float>(numCPUs) / 2.f);
    if (numMaxThreads_ < 1) numMaxThreads_ = 1;         // Need at least one worker. @todo Main thread only implementation!
    else if (numMaxThreads_ > 8) numMaxThreads_ = 8;    // 8 is plenty
    LogDebug(Urho3D::ToString("HttpWorkQueue: Maximum number of threads %d detected CPUs %d", numMaxThreads_, numCPUs));
}

HttpWorkQueue::~HttpWorkQueue()
{
    StopThreads();

    Urho3D::Mutex mutexRequests_;
    requests_.Clear();
}

void HttpWorkQueue::Schedule(const HttpRequestPtr &request)
{
    Urho3D::MutexLock m(mutexRequests_);
    requests_.Push(request);
}

void HttpWorkQueue::Erase(HttpRequestPtr request)
{
    Erase(request.Get());
}

void HttpWorkQueue::Erase(HttpRequest *request)
{
    Urho3D::MutexLock m(mutexRequests_);
    for (HttpRequestPtrList::Iterator iter = requests_.Begin(); iter != requests_.End(); ++iter)
    {
        if ((*iter).Get() == request)
        {
            requests_.Erase(iter);
            break;
        }
    }
}

uint HttpWorkQueue::NumPending()
{
    Urho3D::MutexLock m(mutexRequests_);
    return requests_.Size();
}

void HttpWorkQueue::Update(float frametime)
{
    uint numPending = NumPending();
    if (numPending == 0)
    {
        /* Don't stop workers immediately. Wait for some time
           if new work will come in. Spinning up threads is not free. */
        durationNoWork_ += frametime;
        if (durationNoWork_ > DurationKeepAliveThreads && threads_.Size() > 0)
            StopThreads();
        return;
    }
    durationNoWork_ = 0.f;

    // Start new threads
    if (threads_.Size() < numPending)
        StartThreads(numPending);
}

void HttpWorkQueue::StartThreads(uint max)
{
    if (max > numMaxThreads_)
        max = numMaxThreads_;
    if (threads_.Size() == max)
        return;

    while(threads_.Size() < max)
    {
        HttpWorkThread *thread = new HttpWorkThread(this);
        if (thread->Run())
            threads_.Push(thread);
        else
        {
            LogError("HttpWorkQueue: Failed to start worker thread.");
            delete thread;
            break;
        }
    }
    LogDebug("HttpWorkQueue:Thread count now " + String(threads_.Size()));
}

void HttpWorkQueue::StopThreads()
{
    while(threads_.Size() > 0)
    {
        HttpWorkThread *thread = threads_.Back();
        threads_.Pop();

        LogDebug("HttpWorkQueue: Stopping thread " + String(threads_.Size() + 1));
        thread->Stop();
        delete thread;
    }
}

HttpRequest *HttpWorkQueue::Next()
{
    Urho3D::MutexLock m(mutexRequests_);
    if (requests_.Size() > 0)
        return requests_.Front();
    return nullptr;
}

void HttpWorkQueue::Completed(HttpRequest *request)
{
    // @todo Push to completed queue for emitting completio on next frame.
    Erase(request);
}

// HttpWorkThread

HttpWorkThread::HttpWorkThread(HttpWorkQueue *queue) :
    queue_(queue)
{
}

void HttpWorkThread::ThreadFunction()
{
    LogDebug("HttpWorkThread: Starting " + String(GetCurrentThreadID()));

    while(shouldRun_)
    {
        HttpRequest *request = queue_->Next();
        if (request)
        {
            request->Perform();
            queue_->Completed(request);
        }
        else
            Urho3D::Time::Sleep(16);
    }

    LogDebug("HttpWorkThread: Stopping" + String(GetCurrentThreadID()));
}

}
