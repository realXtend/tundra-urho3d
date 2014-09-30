/**
 *  For conditions of distribution and use, see copyright notice in LICENSE
 *
 *  @file   FrameAPI.cpp
 *  @brief  Frame core API. Exposes framework's update tick.
 */

#include "StableHeaders.h"
#include "FrameAPI.h"
#include "Framework.h"

#include <Profiler.h>

namespace Tundra
{

FrameAPI::FrameAPI(Framework *fw) :
    Urho3D::Object(fw->GetContext()),
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

void FrameAPI::Update(float frametime)
{
    PROFILE(FrameAPI_Update);

    Updated.Emit(frametime);
    PostFrameUpdate.Emit(frametime);

    ++currentFrameNumber;
    if (currentFrameNumber < 0)
        currentFrameNumber = 0;
}

}
