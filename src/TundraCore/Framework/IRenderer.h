// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"
#include "CoreTypes.h"
#include "Math/float2.h"
#include "Math/float3.h"

#include <Vector.h>

namespace Tundra
{

class Scene;
class Entity;
class IComponent;
class Camera;

struct TUNDRACORE_API RayQueryResult
{
    RayQueryResult()
    :entity(0), component(0), pos(float3::nan), 
    normal(float3::nan), submeshIndex((unsigned)-1), triangleIndex((unsigned)-1), 
    uv(float2::nan), barycentricUV(float2::nan), t(std::numeric_limits<float>::quiet_NaN())
    {
    }

    /// Entity that was hit, null if none
    Entity* entity;
    /// Component which was hit.
    IComponent *component;
    /// World coordinates of hit position
    float3 pos;
    /// World face normal of hit
    float3 normal;
    /// Submesh index in entity, starting from 0
    unsigned submeshIndex;
    /// Triangle index in submesh
    unsigned triangleIndex;
    /// The UV coordinates of the first texture channel in the mesh. (0,0) if no texture mapping.
    float2 uv;
    /// Barycentric coordinates along the triangle.
    float2 barycentricUV;
    // With barycentric coordinates (0,0) corresponds to triangle's v0, (1,0) to v1, and (0,1) to v2. barycentricU + barycentricV <= 1.
    /// Distance along the ray to the point of intersection. If this is FLOAT_INF, then no intersection has occurred.
    float t;
};

typedef Vector<RayQueryResult> RayQueryResultVector;

/// Describes the system renderer.
/** @note This class is not an abstract reimplementable interface, but exists only internally for DLL dependency inversion
        purposes between Framework and UrhoRenderer plugin. This interface is only internal to Framework.
        Do not extend this interface. Avoid using it in client code, and prefer directly getting the UrhoRenderer module. */
class TUNDRACORE_API IRenderer
{
public:
    IRenderer() {}

    virtual ~IRenderer() {}

    /// Returns the Entity which contains the currently active camera that is used to render on the main window.
    /// The returned Entity is guaranteed to have an Camera component, and it is guaranteed to be attached to a scene.
    virtual Entity *MainCamera() = 0;

    /// Returns the Camera of the main camera, or 0 if no main camera is active.
    virtual Camera *MainCameraComponent() = 0;

    /// Returns the Scene the current active main camera is in, or 0 if no main camera is active.
    virtual Scene *MainCameraScene() = 0;

    /// Set an entity (that contains a camera component) as the current active main camera.
    virtual void SetMainCamera(Entity* mainCameraEntity) = 0;
};

}
