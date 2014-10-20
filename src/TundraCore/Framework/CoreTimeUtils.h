// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"

namespace Tundra
{

/// Helper utility to limit frame updates to a desired FPS.
struct TUNDRACORE_API FrameLimiter
{
    float step;
    float t;

    /** @param Step in seconds. For example 1.f/30.f
        will target 30 times a second updates. */
    FrameLimiter(float stepInSeconds = 0.f);

    /** Update limiter with @c frametime.
        @param Frametime in seconds since the last update.
        @return True if step time has been reached, false otherwise. */
    bool ShouldUpdate(float frametime);
};

}
