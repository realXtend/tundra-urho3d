// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "CoreDefines.h"
#include "BulletPhysicsApi.h"
#include "BulletPhysicsFwd.h"
#include "Math/float3.h"

namespace Tundra
{

class IMeshAsset;

void BULLETPHYSICS_API GenerateTriangleMesh(IMeshAsset* mesh, btTriangleMesh* ptr);
void BULLETPHYSICS_API GetTrianglesFromMesh(IMeshAsset*, PODVector<float3>& dest);
void BULLETPHYSICS_API GenerateConvexHullSet(IMeshAsset* mesh, ConvexHullSet* ptr);

}
