// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "CameraApplication.h"
#include "GraphicsWorld.h"
#include "Framework.h"
#include "Placeable.h"
#include "../UrhoRenderer/Camera.h" // Possible ambiguity with Urho3D's Camera component
#include "SceneAPI.h"
#include "Entity.h"
#include "Scene/Scene.h"
#include "LoggingFunctions.h"
#include "Math/float3.h"
#include "Math/MathFunc.h"

#include <InputEvents.h>
#include <ResourceCache.h>
#include <ProcessUtils.h>
#include <XMLFile.h>
#include <UI/UI.h>
#include <UI/UIEvents.h>
#include <UI/UIElement.h>

namespace Tundra
{

CameraApplication::CameraApplication(Framework* owner) :
    IModule("CameraApplication", owner),
    joystickId_(-1)
{
}

CameraApplication::~CameraApplication()
{
}

void CameraApplication::Load()
{
}

void CameraApplication::Initialize()
{
    // Connect to scene change signals.
    framework->Scene()->SceneCreated.Connect(this, &CameraApplication::OnSceneCreated);
    framework->Scene()->SceneAboutToBeRemoved.Connect(this, &CameraApplication::OnSceneAboutToBeRemoved);
}

void CameraApplication::Uninitialize()
{
}

void CameraApplication::Update(float frameTime)
{
    Entity* cameraEntity = framework->Renderer()->MainCamera();
    if (cameraEntity)
        MoveCamera(cameraEntity, frameTime);
    else if (lastScene_)
        CreateCamera();
}

void CameraApplication::OnSceneCreated(Scene *scene, AttributeChange::Type)
{
    if (!scene->ViewEnabled() || framework->IsHeadless())
        return;

    // Remember this scene for camera creation on the next update
    lastScene_ = scene;

    if ((Urho3D::GetPlatform() == "Android" || Urho3D::GetPlatform() == "iOS" || GetSubsystem<Urho3D::Input>()->GetTouchEmulation()) && joystickId_ < 0)
    {
        Urho3D::XMLFile *layout = GetSubsystem<Urho3D::ResourceCache>()->GetResource<Urho3D::XMLFile>("UI/ScreenJoystick.xml");
        Urho3D::XMLFile *style = GetSubsystem<Urho3D::ResourceCache>()->GetResource<Urho3D::XMLFile>("UI/DefaultStyle.xml");
        joystickId_ = GetSubsystem<Urho3D::Input>()->AddScreenJoystick(layout, style);
    }
}

void CameraApplication::OnSceneAboutToBeRemoved(Scene *scene, AttributeChange::Type)
{
    if (joystickId_ >= 0 && lastScene_ && scene == lastScene_.Get())
    {
        GetSubsystem<Urho3D::Input>()->RemoveScreenJoystick(joystickId_);
        joystickId_ = -1;
    }
}

void CameraApplication::CreateCamera()
{
    if (!lastScene_)
        return;

    StringVector components;
    components.Push("Placeable");
    components.Push("Camera");
    Entity* cameraEntity = lastScene_->CreateEntity(0, components, AttributeChange::LocalOnly, false, false, true);
    if (!cameraEntity)
    {
        LogError("CameraApplication::CreateCamera: failed to create camera entity");
        return;
    }
    cameraEntity->SetName("FreeLookCamera");
    IRenderer* renderer = framework->Renderer();
    if (!renderer)
    {
        LogError("CameraApplication::CreateCamera: can not assign camera; no renderer assigned");
        return;
    }
    renderer->SetMainCamera(cameraEntity);

    // If scene has an entity called FreeLookCameraSpawnPos, copy its transform
    Entity* cameraPosEntity = lastScene_->EntityByName("FreeLookCameraSpawnPos");
    if (cameraPosEntity)
    {
        Placeable* cameraPosPlaceable = cameraPosEntity->Component<Placeable>();
        if (cameraPosPlaceable)
            cameraEntity->Component<Placeable>()->transform.Set(cameraPosPlaceable->transform.Get());
    }
}

void CameraApplication::MoveCamera(Entity* cameraEntity, float frameTime)
{
    if (!cameraEntity)
        return;

    Placeable* placeable = cameraEntity->Component<Placeable>();
    if (!placeable)
        return;
    
    /// \todo Use InputAPI once it exists
    Urho3D::Input* input = GetSubsystem<Urho3D::Input>();

    const float cameraMoveSpeed = 10.0f;
    const float cameraRotateSpeed = 0.25f;
    bool changed = false;

    Transform t = placeable->transform.Get();

    if (input->GetMouseButtonDown(Urho3D::MOUSEB_RIGHT))
    {
        t.rot.x -= input->GetMouseMoveY() * cameraRotateSpeed;
        t.rot.y -= input->GetMouseMoveX() * cameraRotateSpeed;
        t.rot.x = Clamp(t.rot.x, -90.0f, 90.0f);
        changed = true;
    }
    else if (input->GetNumTouches() > 0)
    {
        // Find a touch point that is not on top of the movement joystick button.
        for (int ti=0, len=input->GetNumTouches(); ti<len; ++ti)
        {
            Urho3D::TouchState *touch = input->GetTouch(ti);
            if (!touch->touchedElement_.Get())
            {
                t.rot -= (float3(static_cast<float>(touch->delta_.y_), static_cast<float>(touch->delta_.x_), 0.f) * cameraRotateSpeed);;
                changed = true;
                break;
            }
        }
    }

    float3 moveVector = float3::zero;
    // Note right-handed coordinate system
    if (input->GetKeyDown(Urho3D::KEY_W))
        moveVector += float3(0.0f, 0.0f, -1.0f);
    if (input->GetKeyDown(Urho3D::KEY_S))
        moveVector += float3(0.0f, 0.0f, 1.0f);
    if (input->GetKeyDown(Urho3D::KEY_A))
        moveVector += float3(-1.0f, 0.0f, 0.0f);
    if (input->GetKeyDown(Urho3D::KEY_D))
        moveVector += float3(1.0f, 0.0f, 0.0f);
    if (input->GetKeyDown(Urho3D::KEY_SPACE))
        moveVector += float3(0.0f, 1.0f, 0.0f);
    if (input->GetKeyDown(Urho3D::KEY_C))
        moveVector += float3(0.0f, -1.0f, 0.0f);

    if (!moveVector.Equals(float3::zero))
    {
        t.pos += t.Orientation() * (cameraMoveSpeed * frameTime * moveVector);
        changed = true;
    }

    if (changed)
        placeable->transform.Set(t);
}

}

extern "C"
{

DLLEXPORT void TundraPluginMain(Tundra::Framework *fw)
{
    fw->RegisterModule(new Tundra::CameraApplication(fw));
}

}
