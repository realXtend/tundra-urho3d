// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "Camera.h"
#include "Placeable.h"
#include "GraphicsWorld.h"
#include "UrhoRenderer.h"
#include "Entity.h"
#include "FrameAPI.h"
#include "Scene/Scene.h"
#include "Profiler.h"
#include "AttributeMetadata.h"
#include "Framework.h"
#include "Math/MathUtilities.h"

#include <Log.h>
#include <StringUtils.h>
#include <Engine/Scene/Node.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Graphics/Graphics.h>
#include <Engine/Graphics/Camera.h>

namespace Tundra
{

Camera::Camera(Urho3D::Context* context, Scene* scene) :
    IComponent(context, scene),
    INIT_ATTRIBUTE_VALUE(upVector, "Up vector", float3::unitY),
    INIT_ATTRIBUTE_VALUE(nearPlane, "Near plane", 0.1f),
    INIT_ATTRIBUTE_VALUE(farPlane, "Far plane", 2000.f),
    INIT_ATTRIBUTE_VALUE(verticalFov, "Vertical FOV", 45.f),
    INIT_ATTRIBUTE_VALUE(aspectRatio, "Aspect ratio", ""),
    camera_(0),
    cameraNode_(0)
{
    static AttributeMetadata minZero;
    static bool metadataInitialized = false;
    if (!metadataInitialized)
    {
        minZero.minimum = "0.0";
        metadataInitialized = true;
    }
    nearPlane.SetMetadata(&minZero);

    ParentEntitySet.Connect(this, &Camera::UpdateSignals);
}

Camera::~Camera()
{
    if (world_.Expired())
    {
        if (camera_)
            LOGERROR("Camera: World has expired, skipping uninitialization!");
        return;
    }
    
    GraphicsWorldPtr world = world_.Lock();
    DetachCamera();
    
    if (camera_)
    {
        camera_.Reset();
        // The camera component will be destroyed along with the node
        cameraNode_->Remove();
        cameraNode_.Reset();
    }
}

void Camera::SetActive()
{
    if (!ViewEnabled())
        return;

    if (!camera_)
    {
        LOGERROR("Camera::SetActive failed: No Urho camera setup!");
        return;
    }
    if (world_.Expired())
    {
        LOGERROR("Camera::SetActive failed: The camera component is in a scene that has already been destroyed!");
        return;
    }
    if (!ParentEntity())
    {
        LOGERROR("Camera::SetActive failed: The camera component is not attached to an Entity!");
        return;
    }

    world_.Lock()->Renderer()->SetMainCamera(ParentEntity());

    // Forcibly update the aspect ratio for the new camera. Ogre has a bug that activating a new camera will not automatically re-apply the aspect ratio automatically,
    // if its setAutoAspectRatio was set to true. Therefore, re-apply the aspect ratio when activating a new camera to the main viewport.
    SetAspectRatio(AspectRatio());
}

float Camera::AspectRatio() const
{
    if (!aspectRatio.Get().Trimmed().Empty())
    {
        if (!aspectRatio.Get().Contains(":"))
        {
            float ar = Urho3D::ToFloat(aspectRatio.Get());
            if (ar > 0.f)
                return ar;
        }
        else
        {
            StringVector str = aspectRatio.Get().Split(':');
            if (str.Size() == 2)
            {
                float width = Urho3D::ToFloat(str[0]);
                float height = Urho3D::ToFloat(str[1]);

                if (width > 0.f && height > 0.f)
                    return width / height;
            }
        }
        LOGERROR("Invalid format for the aspectRatio field: \"" + aspectRatio.Get() + "\"! Should be of form \"float\" or \"float:float\". Leave aspectRatio empty to match the current main viewport aspect ratio.");
    }

    /// \todo Assumes entire physical window is the viewport
    Urho3D::Graphics* gfx = GetSubsystem<Urho3D::Graphics>();
    if (gfx)
        return (float)gfx->GetWidth() / (float)gfx->GetHeight();

    LOGWARNING("Camera::AspectRatio: No viewport or aspectRatio attribute set! Don't have an aspect ratio for the camera!");
    return 1.f;
}

void Camera::SetAspectRatio(float ratio)
{
    if (camera_)
    {
        camera_->SetAspectRatio(ratio);
        camera_->SetAutoAspectRatio(aspectRatio.Get().Trimmed().Empty()); /**< @note If user inputs garbage into the aspectRatio field, this will incorrectly go true. (but above line prints an error to user, so should be ok). */
    }
}

bool Camera::IsActive() const
{
    if (!camera_ || world_.Expired() || !ParentEntity())
        return false;

    return world_.Lock()->Renderer()->MainCamera() == ParentEntity();
}

void Camera::DetachCamera()
{
    if (!camera_ || world_.Expired())
        return;

    if (placeable_)
    {
        Urho3D::Scene* urhoScene = world_->UrhoScene();
        // When removed from the placeable, attach to scene root to avoid being removed from scene
        cameraNode_->SetParent(urhoScene);
        placeable_.Reset();
    }
}

void Camera::AttachCamera()
{
    if (!camera_ || world_.Expired())
        return;

    // Detach first, in case the original placeable no longer exists
    DetachCamera();

    Entity *entity = ParentEntity();
    if (!entity)
        return;
    placeable_ = entity->Component<Placeable>();
    if (!placeable_)
        return;

    Urho3D::Node* placeableNode = placeable_->UrhoSceneNode();
    if (!placeableNode)
    {
        LOGERROR("Can not attach camera: placeable does not have an Urho3D scene node");
        return;
    }
    cameraNode_->SetParent(placeableNode);
}

Ray Camera::ViewportPointToRay(float x, float y) const
{
    // Do a bit of sanity checking that the user didn't go and input absolute window coordinates.
    if (fabs(x) >= 10.f || fabs(y) >= 10.f || !IsFinite(x) || !IsFinite(y))
        LOGERRORF("Camera::ViewportPointToRay takes input (x,y) coordinates normalized in the range [0,1]! (You inputted x=%f, y=%f", x, y);

    /// \todo Implement
    return Ray();
}

Ray Camera::ScreenPointToRay(uint x, uint y) const
{
    /// \todo Implement
    return Ray();
}

void Camera::UpdateSignals()
{
    Entity* parent = ParentEntity();
    if (!parent)
        return;

    // If scene is not view-enabled, no further action
    if (!ViewEnabled())
        return;

    parent->ComponentAdded.Connect(this, &Camera::OnComponentStructureChanged);
    parent->ComponentRemoved.Connect(this, &Camera::OnComponentStructureChanged);
    
    if (parent->ParentScene())
        world_ = parent->ParentScene()->Subsystem<GraphicsWorld>();

    // Create camera now if not yet created
    if (world_ && !camera_)
    {
        Urho3D::Scene* urhoScene = world_->UrhoScene();
        // Also create a scene node for the camera so that it can exist irrespective of whether there is a Placeable scene node
        // (in Urho3D components can not be created detached from a node)
        cameraNode_ = urhoScene->CreateChild("CameraNode");
        camera_ = cameraNode_->CreateComponent<Urho3D::Camera>();

        // Set initial attribute values
        SetNearClipDistance(nearPlane.Get());
        SetFarClipDistance(farPlane.Get());
        SetFovY(verticalFov.Get());
        SetAspectRatio(AspectRatio());
    }

    // Make sure we attach to the EC_Placeable if exists.
    AttachCamera();
}

void Camera::OnComponentStructureChanged(IComponent*, AttributeChange::Type)
{
    // No-op if attached to the same placeable already
    if (placeable_ == parentEntity->Component<Placeable>())
        return;

    AttachCamera(); // Try to attach if placeable is present, otherwise detach
}

void Camera::AttributesChanged()
{
    if (nearPlane.ValueChanged())
        SetNearClipDistance(nearPlane.Get());
    if (farPlane.ValueChanged())
        SetFarClipDistance(farPlane.Get());
    if (verticalFov.ValueChanged())
        SetFovY(verticalFov.Get());
    else if (aspectRatio.ValueChanged())
        SetAspectRatio(AspectRatio());
}

bool Camera::IsEntityVisible(Entity* entity)
{
    if (!entity || !camera_)
        return false;
    
    /// \todo Implement
    return false;
}

EntityVector Camera::VisibleEntities()
{
    EntityVector ret;
    /// \todo Implement
    return ret;
}

float4x4 Camera::ViewMatrix() const
{
    return (camera_ ? ToFloat4x4(camera_->GetView()) : float4x4::nan);
}

float4x4 Camera::ProjectionMatrix() const
{
    return (camera_ ? ToFloat4x4(camera_->GetProjection()) : float4x4::nan);
}

void Camera::SetFromFrustum(const Frustum &f)
{
    Placeable* p = static_cast<Placeable*>(placeable_.Get());
    if (p)
        p->SetWorldTransform(float3x4::LookAt(f.pos, f.pos + f.front, -float3::unitZ, float3::unitY, f.up));
    else
        LOGWARNING("Camera::SetFromFrustum: Camera entity has no Placeable, cannot set world transform.");

    upVector.Set(f.up, AttributeChange::Disconnected);
    nearPlane.Set(f.nearPlaneDistance, AttributeChange::Disconnected);
    farPlane.Set(f.farPlaneDistance, AttributeChange::Disconnected);
    // f.horizontalFov is used for calculating the aspect ratio
    verticalFov.Set(RadToDeg(f.verticalFov), AttributeChange::Disconnected);
    aspectRatio.Set(String(f.AspectRatio()), AttributeChange::Default);
}

Frustum Camera::ToFrustum() const
{
    Frustum f;
    f.type = InvalidFrustum;
    if (!camera_)
    {
        LOGERROR("Camera::ToFrustum failed: No Ogre camera initialized to Camera!");
        return f;
    }
    Placeable* p = static_cast<Placeable*>(placeable_.Get());
    if (!p)
    {
        LOGERROR("Camera::ToFrustum failed: No Placeable set to the camera entity!");
        return f;
    }

    f.type = PerspectiveFrustum;
    f.pos = p->WorldPosition();
    f.front = p->WorldOrientation() * -float3::unitZ;
    f.up = upVector.Get().Normalized();
    f.nearPlaneDistance = nearPlane.Get();
    f.farPlaneDistance = farPlane.Get();
    f.horizontalFov = DegToRad(verticalFov.Get());
    f.verticalFov = AspectRatio() * f.horizontalFov;
    return f;
}

void Camera::SetNearClipDistance(float distance)
{
    if (camera_ && !world_.Expired())
        camera_->SetNearClip(distance);
}

void Camera::SetFarClipDistance(float distance)
{
    if (camera_ && !world_.Expired())
        camera_->SetFarClip(distance);
}

void Camera::SetFovY(float fov)
{
    if (camera_ && !world_.Expired())
        camera_->SetFov(fov);
}

}
