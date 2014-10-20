// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "CoreTimeUtils.h"

#include "Math/MathFunc.h"

namespace Tundra
{

FrameLimiter::FrameLimiter(float step_) :
    step(step_),
    t(0.f)
{
}

bool FrameLimiter::ShouldUpdate(float frametime)
{
    t += frametime;
    if (t >= step)
    {
        t = Mod(t, step);
        return true;
    }
    return false;
}

}
