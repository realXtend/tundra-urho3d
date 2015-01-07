/**
 *  For conditions of distribution and use, see copyright notice in LICENSE
 *
 *  @file   FrameAPI.cpp
 *  @brief  Frame core API. Exposes framework's update tick.
 */

#include "StableHeaders.h"
#include "FrameAPI.h"
#include "Framework.h"

#include <Urho3D/Core/Profiler.h>

namespace Tundra
{

FrameAPI::FrameAPI(Framework *fw) :
    Object(fw->GetContext()),
    currentFrameNumber(0)
{
}

FrameAPI::~FrameAPI()
{
}

void FrameAPI::Reset()
{
}

float FrameAPI::WallClockTime() const
{
    return (float)((double)wallClock.GetUSec(false) / 1000000.0);
}

DelayedSignal& FrameAPI::DelayedExecute(float time)
{
    delayedSignals.Push(DelayedSignal());
    DelayedSignal& newSignal = delayedSignals.Back();
    newSignal.startTime = WallClockTime();
    newSignal.triggerTime = newSignal.startTime + time;
    return newSignal;
}

void FrameAPI::Update(float frametime)
{
    PROFILE(FrameAPI_Update);

    Updated.Emit(frametime);
    PostFrameUpdate.Emit(frametime);

    float currentTime = WallClockTime();
    // Trigger and remove expired delayed signals
    for (List<DelayedSignal>::Iterator it = delayedSignals.Begin(); it != delayedSignals.End();)
    {
        if (currentTime >= it->triggerTime)
        {
            it->Emit(currentTime - it->startTime);
            it = delayedSignals.Erase(it);
        }
        else
            ++it;
    }

    ++currentFrameNumber;
    if (currentFrameNumber < 0)
        currentFrameNumber = 0;
}

}
