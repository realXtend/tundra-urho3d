// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#define MATH_BULLET_INTEROP
#include "CollisionShapeUtils.h"
#include "ConvexHull.h"
#include "PhysicsUtils.h"
#include "LoggingFunctions.h"
#include "hull.h"
#include "IMeshAsset.h"

#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/Model.h>

// Disable unreferenced formal parameter coming from Bullet
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4100)
#endif
#include <btBulletDynamicsCommon.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace Tundra
{

void GenerateTriangleMesh(IMeshAsset* mesh, btTriangleMesh* ptr)
{
    PODVector<float3> triangles;
    GetTrianglesFromMesh(mesh, triangles);
    
    for(uint i = 0; i < triangles.Size(); i += 3)
        ptr->addTriangle(triangles[i], triangles[i+1], triangles[i+2]);
}

void GenerateConvexHullSet(IMeshAsset* mesh, ConvexHullSet* ptr)
{
    PODVector<float3> vertices;
    GetTrianglesFromMesh(mesh, vertices);
    if (!vertices.Size())
    {
        LogError("Mesh had no triangles; aborting convex hull generation");
        return;
    }
    
    StanHull::HullDesc desc;
    desc.SetHullFlag(StanHull::QF_TRIANGLES);
    desc.mVcount = (uint)vertices.Size();
    desc.mVertices = &vertices[0].x;
    desc.mVertexStride = sizeof(float3);
    desc.mSkinWidth = 0.01f; // Hardcoded skin width
    
    StanHull::HullLibrary lib;
    StanHull::HullResult result;
    lib.CreateConvexHull(desc, result);

    if (!result.mNumOutputVertices)
    {
        LogError("No vertices were generated; aborting convex hull generation");
        return;
    }
    
    ConvexHull hull;
    hull.position_ = float3(0,0,0);
    /// \todo StanHull always produces only 1 hull. Therefore using a hull set is unnecessary and could be optimized away
    hull.hull_ = shared_ptr<btConvexHullShape>(new btConvexHullShape((const btScalar*)&result.mOutputVertices[0], result.mNumOutputVertices, static_cast<int>(3 * sizeof(float))));
    ptr->hulls_.Push(hull);
    
    lib.ReleaseResult(result);
}

void GetTrianglesFromMesh(IMeshAsset* mesh, PODVector<float3>& dest)
{
    dest.Clear();
    if (!mesh)
    {
        LogError("Null mesh asset specified for GetTrianglesFromMesh");
        return;
    }

    Urho3D::Model* model = mesh->UrhoModel();
    if (!model)
    {
        LogError("Unloaded mesh asset specified for GetTrianglesFromMesh");
        return;
    }

    uint numGeometries = model->GetNumGeometries();

    for (uint i = 0; i < numGeometries; ++i)
    {
        Urho3D::Geometry* geometry = model->GetGeometry(i, 0);
        if (!geometry)
        {
            URHO3D_LOGWARNING("Skipping null geometry in GetTrianglesFromMesh");
            continue;
        }

        const u8* vertexData;
        const u8* indexData;
        uint vertexSize;
        uint indexSize;
        uint elementMask;

        geometry->GetRawData(vertexData, vertexSize, indexData, indexSize, elementMask);
        if (!vertexData || !indexData)
        {
            URHO3D_LOGWARNING("Skipping geometry with no CPU-side geometry data in GetTrianglesFromMesh");
            continue;
        }

        uint indexStart = geometry->GetIndexStart();
        uint indexCount = geometry->GetIndexCount();

        if (indexSize == sizeof(ushort))
        {
            const ushort* indices = ((const ushort*)indexData);
            for (uint j = indexStart; j < indexStart + indexCount; j += 3)
            {
                float3 v0 = *((const Urho3D::Vector3*)(&vertexData[indices[j] * vertexSize]));
                float3 v1 = *((const Urho3D::Vector3*)(&vertexData[indices[j+1] * vertexSize]));
                float3 v2 = *((const Urho3D::Vector3*)(&vertexData[indices[j+2] * vertexSize]));
                dest.Push(v0);
                dest.Push(v1);
                dest.Push(v2);
            }
        }
        else
        {
            const uint* indices = ((const uint*)indexData);
            for (uint j = indexStart; j < indexStart + indexCount; j += 3)
            {
                float3 v0 = *((const Urho3D::Vector3*)(&vertexData[indices[j] * vertexSize]));
                float3 v1 = *((const Urho3D::Vector3*)(&vertexData[indices[j + 1] * vertexSize]));
                float3 v2 = *((const Urho3D::Vector3*)(&vertexData[indices[j + 2] * vertexSize]));
                dest.Push(v0);
                dest.Push(v1);
                dest.Push(v2);
            }
        }
    }
}

}
