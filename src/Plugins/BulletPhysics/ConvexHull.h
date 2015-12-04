// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "CoreDefines.h"
#include "CoreTypes.h"
#include "BulletPhysicsApi.h"
#include "BulletPhysicsFwd.h"
#include "Math/float3.h"

#include "StdPtr.h"

namespace Tundra
{

/** @cond PRIVATE */
struct ConvexHull
{
    float3 position_;
    shared_ptr<btConvexHullShape> hull_;
};

struct ConvexHullSet
{
    Vector<ConvexHull> hulls_;
};
/** @endcond */

}
