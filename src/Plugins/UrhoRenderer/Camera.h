// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IComponent.h"
#include "UrhoModuleApi.h"
#include "UrhoModuleFwd.h"
#include "Math/float3.h"
#include "Math/float4x4.h"
#include "Math/Point.h"
#include "Geometry/Frustum.h"
#include "Geometry/Ray.h"

namespace Tundra
{

typedef WeakPtr<Placeable> PlaceableWeakPtr;

/// Camera component
/** <table class="header">
    <tr>
    <td>
    <h2>Camera</h2>
    Ogre camera entity component
    Needs to be attached to a placeable (aka scene node) to be useful.

    Registered by OgreRenderer::OgreRenderingModule.

    \ingroup OgreRenderingModuleClient

    <b>Attributes</b>:
    <ul>
    <li>float3: upVector
    <div> @copydoc .</div>
    <li>float: nearPlane
    <div> @copydoc </div>
    <li>float: farPlane
    <div> @copydoc </div>
    <li>float3: verticalFov
    <div> @copydoc </div>
    <li>QString: aspectRatio
    <div> @copydoc </div>
    </ul>

    Does not react on any actions.

    Does not emit any actions.

    <b>Depends on the component @ref EC_Placeable "Placeable".</b>
    </table> */
class URHO_MODULE_API Camera : public IComponent
{
    COMPONENT_NAME(Camera, 15)

public:
     /// @cond PRIVATE
    /// Do not directly allocate new components using operator new, but use the factory-based SceneAPI::CreateComponent functions instead.
    explicit Camera(Urho3D::Context* context, Scene* scene);
    /// @endcond
    virtual ~Camera();

    /// Camera up vector. Defines the yaw axis
    Attribute<float3> upVector;

    /// Near clip distance.
    ///\todo Rename to nearClipDistance?
    Attribute<float> nearPlane;

    /// Far clip distance.
    ///\todo Rename to farClipDistance?
    Attribute<float> farPlane;

    /// Vertical field of view as degrees.
    Attribute<float> verticalFov;

    /// Aspect ratio is a string of form "<widthProportion>:<heightProportion>", e.g. "4:3".
    /**  Alternatively float can be used too, e.g. "1.33". If this string is empty, the aspect ratio of the main window viewport is used (default). */
    Attribute<String> aspectRatio;

    /// Returns the actual Urho camera component.
    /** @note Use with caution. */
    Urho3D::Camera* UrhoCamera() const { return camera_; }

    /// Sets this camera as the active main window camera.
    /// Calling this function is equivalent to calling Renderer::SetMainCamera(this).
    void SetActive();

    /// Returns whether camera is active in the viewport
    bool IsActive() const;

    /// Returns the currently used view aspect ratio (width/height).
    float AspectRatio() const;

    /// Sets the currently used view aspect ratio (width/height).
    /** @note Does not change the value of the aspectRatio attribute.
        @note The aspectRatio attribute must be != "" (i.e. we're not using automatic aspect ratio) otherwise this functions has not effect. */
    void SetAspectRatio(float ratio);

    /// Returns whether an entity is visible in the camera's frustum
    bool IsEntityVisible(Entity* entity);

    /// Returns visible entities in the camera's frustum.
    EntityVector VisibleEntities();

    /// Returns a world space ray as cast from the camera through a viewport position.
    /** @param x The x position at which the ray should intersect the viewport, in normalized screen coordinates [0,1].
        @param y The y position at which the ray should intersect the viewport, in normalized screen coordinates [0,1].
        @sa ScreenPointToRay */
    Ray ViewportPointToRay(float x, float y) const;

    /// Returns a world space ray as cast from the camera through a screen (graphics scene) position.
    /** @param x The x screen position.
        @param y The y screen position.
        @sa ViewportPointToRay */
    Ray ScreenPointToRay(uint x, uint y) const;
    Ray ScreenPointToRay(const Point &point) const { return ScreenPointToRay(point.x, point.y); } /**< @overload @param point Screen point. */

    /// Returns the view matrix for this camera, float4x4::nan if not applicable.
    float4x4 ViewMatrix() const;

    /// Returns the projection matrix for this camera, float4x4::nan if not applicable.
    float4x4 ProjectionMatrix() const;

    /// Returns the camera's perspective viewing frustum, Frustum::type == InvalidFrustum if not applicable.
    Frustum ToFrustum() const;

    /// Sets the camera's view from a perspective viewing frustum.
    void SetFromFrustum(const Frustum &f);

private:
    /// Called when the parent entity has been set.
    void UpdateSignals();

    /// Called when component has been added or removed from the parent entity. Checks the existence of the EC_Placeable component, and attaches this camera to it.
    void OnComponentStructureChanged(IComponent*, AttributeChange::Type);

    void AttributesChanged();

    /// Sets placeable component
    /** set a null placeable to detach the camera, otherwise will attach
        @param placeable placeable component */
    void SetPlaceable(const ComponentPtr &placeable);

    /// attaches camera to placeable
    void AttachCamera();

    /// detaches camera from placeable
    void DetachCamera();

    /// Deletes the Urho camera associated with this component from the Urho scene.
    /// After calling this function, the internal camera_ pointer is null.
    void DestroyCamera();

    /// Perform a frustum query for visible entities
    void QueryVisibleEntities();

    void SetNearClipDistance(float distance);
    void SetFarClipDistance(float distance);
    void SetFovY(float fov);

    /// placeable component 
    PlaceableWeakPtr placeable_;

    /// Attached to placeable -flag.
    bool attached_;

    /// World ptr
    GraphicsWorldWeakPtr world_;

    /// Urho camera
    SharedPtr<Urho3D::Camera> camera_;
    /// Internal scene node for the camera. Must exist to allow the Urho camera to be created (even if not effective in the scene) without the Placeable component
    SharedPtr<Urho3D::Node> cameraNode_;
};

COMPONENT_TYPEDEFS(Camera)

}

