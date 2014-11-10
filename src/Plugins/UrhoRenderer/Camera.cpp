// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "Camera.h"
#include "Placeable.h"
#include "GraphicsWorld.h"
#include "UrhoRenderer.h"

#include "Entity.h"
#include "FrameAPI.h"
#include "Scene/Scene.h"
#include "AttributeMetadata.h"
#include "Framework.h"
#include "LoggingFunctions.h"

#include <Profiler.h>
#include <StringUtils.h>
#include <Engine/Scene/Node.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Graphics/Graphics.h>
#include <Engine/Graphics/Camera.h>
#include <Engine/Graphics/GraphicsEvents.h>
#include <Engine/Graphics/Renderer.h>
#include <Engine/Core/Profiler.h>
#include <Geometry/Plane.h>

namespace Tundra
{

Camera::Camera(Urho3D::Context* context, Scene* scene) :
    IComponent(context, scene),
    INIT_ATTRIBUTE_VALUE(upVector, "Up vector", float3::unitY),
    INIT_ATTRIBUTE_VALUE(nearPlane, "Near plane", 0.1f), /**< @todo Appending "distance" to the name would be nice */
    INIT_ATTRIBUTE_VALUE(farPlane, "Far plane", 2000.f), /**< @todo Appending "distance" to the name would be nice */
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
            LogError("Camera: World has expired, skipping uninitialization!");
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
        LogError("Camera::SetActive failed: No Urho camera setup!");
        return;
    }
    if (world_.Expired())
    {
        LogError("Camera::SetActive failed: The camera component is in a scene that has already been destroyed!");
        return;
    }
    if (!ParentEntity())
    {
        LogError("Camera::SetActive failed: The camera component is not attached to an Entity!");
        return;
    }

    world_.Lock()->Renderer()->SetMainCamera(ParentEntity());
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
        LogError("Invalid format for the aspectRatio field: \"" + aspectRatio.Get() + "\"! Should be of form \"float\" or \"float:float\". Leave aspectRatio empty to match the current main viewport aspect ratio.");
    }

    /// \todo Assumes entire physical window is the viewport
    Urho3D::Graphics* gfx = GetSubsystem<Urho3D::Graphics>();
    if (gfx)
        return (float)gfx->GetWidth() / (float)gfx->GetHeight();

    LogWarning("Camera::AspectRatio: No viewport or aspectRatio attribute set! Don't have an aspect ratio for the camera!");
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
        LogError("Can not attach camera: placeable does not have an Urho3D scene node");
        return;
    }
    cameraNode_->SetParent(placeableNode);
}

Ray Camera::ViewportPointToRay(float x, float y) const
{
    // Do a bit of sanity checking that the user didn't go and input absolute window coordinates.
    if (fabs(x) >= 10.f || fabs(y) >= 10.f || !IsFinite(x) || !IsFinite(y))
        LogError("Camera::ViewportPointToRay takes input (x,y) coordinates normalized in the range [0,1]! (You inputted x=" + String(x) + ", y=" + String(y));

    if (camera_)
        return camera_->GetScreenRay(x, y);
    else
        return Ray();
}

Ray Camera::ScreenPointToRay(uint x, uint y) const
{
    Urho3D::Graphics* graphics = GetSubsystem<Urho3D::Graphics>();

    if (graphics && camera_)
        return camera_->GetScreenRay((float)x / (float)graphics->GetWidth(), (float)y / (float)graphics->GetHeight());
    else
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

        Urho3D::Renderer* urhoRenderer = GetSubsystem<Urho3D::Renderer>();
        if (urhoRenderer)
            SubscribeToEvent(urhoRenderer, Urho3D::E_BEGINVIEWUPDATE, HANDLER(Camera, HandleBeginViewUpdate));
    }

    // Make sure we attach to the Placeable if exists.
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
    return (camera_ ? float4x4(camera_->GetView()) : float4x4::nan);
}

float4x4 Camera::ProjectionMatrix() const
{
    return (camera_ ? float4x4(camera_->GetProjection()) : float4x4::nan);
}

void Camera::SetFromFrustum(const Frustum &f)
{
    PlaceablePtr p = placeable_.Lock();
    if (p)
        p->SetWorldTransform(float3x4::LookAt(f.Pos(), f.Pos() + f.Front(), -float3::unitZ, float3::unitY, f.Up()));
    else
        LogWarning("Camera::SetFromFrustum: Camera entity has no Placeable, cannot set world transform.");

    upVector.Set(f.Up(), AttributeChange::Disconnected);
    nearPlane.Set(f.NearPlaneDistance(), AttributeChange::Disconnected);
    farPlane.Set(f.FarPlaneDistance(), AttributeChange::Disconnected);
    // f.horizontalFov is used for calculating the aspect ratio
    verticalFov.Set(RadToDeg(f.VerticalFov()), AttributeChange::Disconnected);
    aspectRatio.Set(String(f.AspectRatio()), AttributeChange::Default);
}

Frustum Camera::ToFrustum() const
{
    Frustum f;

    if (!camera_)
    {
        LogError("Camera::ToFrustum failed: No Urho camera initialized to Camera!");
        return f;
    }
    PlaceablePtr p = placeable_.Lock();
    if (!p)
    {
        LogError("Camera::ToFrustum failed: No Placeable set to the camera entity!");
        return f;
    }

    f.SetPos(p->WorldPosition());
    f.SetFront(p->WorldOrientation() * -float3::unitZ);
    f.SetUp(upVector.Get().Normalized());
    f.SetViewPlaneDistances(nearPlane.Get(), farPlane.Get());
    f.SetVerticalFovAndAspectRatio(DegToRad(verticalFov.Get()), AspectRatio());
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

void Camera::HandleBeginViewUpdate(Urho3D::StringHash /*eventType*/, Urho3D::VariantMap& eventData)
{
    using namespace Urho3D::BeginViewUpdate;
    if (!cameraNode_ || !camera_ || eventData[P_CAMERA].GetPtr() != camera_)
        return;

    // Setup a reflection plane to convert to Tundra's right-handed coordinate system
    Urho3D::Plane zPlane;
    Urho3D::Vector3 cameraPosition = cameraNode_->GetWorldPosition();
    Urho3D::Vector3 cameraDir = cameraNode_->GetWorldDirection();
    zPlane.Define(cameraDir, cameraPosition);
    camera_->SetUseReflection(true);
    camera_->SetReflectionPlane(zPlane);
}

}
