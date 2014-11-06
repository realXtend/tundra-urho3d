// For conditions of distribution and use, see copyright notice in LICENSE

//#include "StableHeaders.h"
#include "HttpWorkQueue.h"
#include "HttpRequest.h"

#include "Framework.h"
#include "Math/MathFunc.h"

#include <Engine/Core/ProcessUtils.h>
#include <Engine/Core/StringUtils.h>

namespace Tundra
{

// HttpWorkQueue

const float DurationKeepAliveThreads = 10.f;

HttpWorkQueue::HttpWorkQueue() :
    log("HttpWorkQueue"),
    durationNoWork_(0.f)
{
    numMaxThreads_ = Urho3D::GetNumLogicalCPUs();
    if (numMaxThreads_ < 1) numMaxThreads_ = 1;         // Need at least one worker
    else if (numMaxThreads_ > 32) numMaxThreads_ = 32;  // Cap to something sensible
    log.DebugF("Maximum number of threads %d detected from %d physical and %d logical CPUs", numMaxThreads_, Urho3D::GetNumPhysicalCPUs(), Urho3D::GetNumLogicalCPUs());
}

HttpWorkQueue::~HttpWorkQueue()
{
    // Stop all threads first. They hold raw ptrs to our queues.
    StopThreads();
    {
        Urho3D::MutexLock m(mutexRequests_);
        created_.Clear();
        requests_.Clear();
    }
    {
        Urho3D::MutexLock m(mutexCompleted_);
        completed_.Clear();
        executing_.Clear();
    }
}

void HttpWorkQueue::Schedule(const HttpRequestPtr &request)
{
    created_.Push(request);
}

uint HttpWorkQueue::NumPending()
{
    uint num = created_.Size();
    {
        Urho3D::MutexLock m(mutexRequests_);
        num += requests_.Size();
    }
    return num;
}

void HttpWorkQueue::Update(float frametime)
{
    // Signal completion/failure
    uint numExecuting = 0;
    {
        Urho3D::MutexLock m(mutexCompleted_);
        for (auto iter = completed_.Begin(); iter != completed_.End(); ++iter)
            (*iter)->EmitCompletion(*iter);
        completed_.Clear();
        numExecuting = executing_.Size();
    }

    /* Move created requests to the worker thread polled requests queue.
       This is done so that main thread can prepare the created request
       witin the creation frame update without threading conflicts. */
    uint numPending = 0;
    {
        Urho3D::MutexLock m2(mutexRequests_);
        if (created_.Size() > 0)
        {
            requests_.Insert(requests_.End(), created_.Begin(), created_.End());
            created_.Clear();
        }
        numPending = requests_.Size();
    }

    if (numPending + numExecuting == 0)
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
    if (threads_.Size() >= max)
        return;

    while(threads_.Size() < max)
    {
        HttpWorkThread *thread = new HttpWorkThread(this);
        if (thread->Run())
            threads_.Push(thread);
        else
        {
            log.Error("Failed to start worker thread.");
            delete thread;
            break;
        }
    }
    log.DebugF("Thread count now %d", threads_.Size());
}

void HttpWorkQueue::StopThreads()
{
    while(threads_.Size() > 0)
    {
        HttpWorkThread *thread = threads_.Back();
        threads_.Pop();

        log.DebugF("Stopping thread %d", threads_.Size()+1);
        thread->Stop();
        delete thread;
    }
}

HttpRequest* HttpWorkQueue::Next()
{
    mutexRequests_.Acquire();
    if (requests_.Size() > 0)
    {
        // Remove from pending
        HttpRequestPtrList::Iterator iter = requests_.Begin();
        HttpRequestPtr next = *iter;
        requests_.Erase(iter);
        mutexRequests_.Release();

        // Add to executing
        {
            Urho3D::MutexLock m(mutexCompleted_);
            executing_.Push(next);
        }
        return next;
    }
    mutexRequests_.Release();
    return nullptr;
}

HttpRequestPtrList::Iterator HttpWorkQueue::FindExecuting(HttpRequest *request)
{
    // @note You have to ensure mutexCompleted_ is locked prior to calling this function.

    for (auto iter = executing_.Begin(); iter != executing_.End(); ++iter)
        if ((*iter).Get() == request)
            return iter;
    return executing_.End();
}

void HttpWorkQueue::Completed(HttpRequest *request)
{
    Urho3D::MutexLock m(mutexCompleted_);

    HttpRequestPtrList::Iterator done = FindExecuting(request);
    if (done != executing_.End())
    {
        // Push to completed list. Main thread will signal completed/failed.
        HttpRequestPtr doneShared = *done;
        completed_.Push(doneShared);
        executing_.Erase(done);
    }
    else
        log.ErrorF("Failed to remove completed request from executing list: %s", request->Url().CString());
}

// HttpWorkThread

HttpWorkThread::HttpWorkThread(HttpWorkQueue *queue) :
    queue_(queue)
{
}

void HttpWorkThread::ThreadFunction()
{
    LogDebug("[HttpWorkThread] Starting " + String(GetCurrentThreadID()));

    while(shouldRun_)
    {
        HttpRequest  *request = queue_->Next();
        if (request)
        {
            request->Perform();
            queue_->Completed(request);
        }
        else
            Urho3D::Time::Sleep(16);
    }

    LogDebug("[HttpWorkThread] Stopping " + String(GetCurrentThreadID()));
}

}
