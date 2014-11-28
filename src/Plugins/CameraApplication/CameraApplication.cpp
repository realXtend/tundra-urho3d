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
#include "InputAPI.h"
#include "InputContext.h"

#include <InputEvents.h>
#include <ResourceCache.h>
#include <ProcessUtils.h>
#include <XMLFile.h>
#include <UI/UI.h>
#include <UI/UIEvents.h>
#include <UI/UIElement.h>

namespace Tundra
{

const float cMoveSpeed = 15.0f;
const float cRotateSpeed = 0.20f;

CameraApplication::CameraApplication(Framework* owner) :
    IModule("CameraApplication", owner),
    joystickId_(-1),
    movementHeld_(0.f),
    lastMoveVector_(float3::zero)
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

    inputContext_ = framework->Input()->RegisterInputContext("CameraApplication", 101);
    inputContext_->MouseScroll.Connect(this, &CameraApplication::OnMouseScroll);
}

void CameraApplication::Uninitialize()
{
    inputContext_.Reset();
}

void CameraApplication::Update(float frameTime)
{
    Entity* cameraEntity = framework->Renderer()->MainCamera();
    if (cameraEntity)
        MoveCamera(cameraEntity, frameTime);
    else if (!cameraEntity && lastScene_)
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
    lastCamera_ = cameraEntity;

    lastScene_->EntityCreated.Connect(this, &CameraApplication::CheckCameraSpawnPos);

    CheckCameraSpawnPos(lastScene_->EntityByName("FreeLookCameraSpawnPos"), AttributeChange::Default);
}

void CameraApplication::CheckCameraSpawnPos(Entity* entity, AttributeChange::Type /*change*/)
{
    if (!entity)
        return;

    Entity* cameraEntity = framework->Renderer()->MainCamera();
    if (!cameraEntity || !cameraEntity->Component<Placeable>())
        return;
    
    Placeable* cameraPosPlaceable = entity->Component<Placeable>();
    if (cameraPosPlaceable)
        cameraEntity->Component<Placeable>()->transform.Set(cameraPosPlaceable->transform.Get());
}

void CameraApplication::MoveCamera(Entity* cameraEntity, float frameTime)
{
    if (!cameraEntity)
        return;

    Placeable* placeable = cameraEntity->Component<Placeable>();
    if (!placeable)
        return;
    
    InputAPI* input = framework->Input();

    bool changed = false;

    Transform t = placeable->transform.Get();

    float3 rotDelta = float3::zero;

    if (inputContext_->IsMouseButtonDown(Urho3D::MOUSEB_RIGHT))
    {
        rotDelta.x -= input->GetMouseMoveY() * cRotateSpeed;
        rotDelta.y -= input->GetMouseMoveX() * cRotateSpeed;
    }
    else if (inputContext_->GetNumTouches() > 0)
    {
        // Find a touch point that is not on top of the movement joystick button.
        for (int ti=0, len=input->GetNumTouches(); ti<len; ++ti)
        {
            Urho3D::TouchState *touch = input->GetTouch(ti);
            if (!touch->touchedElement_.Get())
            {
                rotDelta -= (float3(static_cast<float>(touch->delta_.y_), static_cast<float>(touch->delta_.x_), 0.f) * cRotateSpeed);
                break;
            }
        }
    }

    if (!rotDelta.Equals(float3::zero))
    {
        RotateChanged.Emit(rotDelta);

        t.rot.x += rotDelta.x;
        t.rot.y += rotDelta.y;
        t.rot.x = Clamp(t.rot.x, -90.0f, 90.0f);
        changed = true;
    }

    float3 moveVector = float3::zero;
    // Note right-handed coordinate system
    if (inputContext_->IsKeyDown(Urho3D::KEY_W))
        moveVector += float3(0.0f, 0.0f, -1.0f);
    if (inputContext_->IsKeyDown(Urho3D::KEY_S))
        moveVector += float3(0.0f, 0.0f, 1.0f);
    if (inputContext_->IsKeyDown(Urho3D::KEY_A))
        moveVector += float3(-1.0f, 0.0f, 0.0f);
    if (inputContext_->IsKeyDown(Urho3D::KEY_D))
        moveVector += float3(1.0f, 0.0f, 0.0f);
    if (inputContext_->IsKeyDown(Urho3D::KEY_SPACE))
        moveVector += float3(0.0f, 1.0f, 0.0f);
    if (inputContext_->IsKeyDown(Urho3D::KEY_C))
        moveVector += float3(0.0f, -1.0f, 0.0f);

    if (!moveVector.Equals(lastMoveVector_))
    {
        lastMoveVector_ = moveVector;
        MoveChanged.Emit(moveVector);
    }

    if (inputContext_->IsKeyPressed(Urho3D::KEY_SHIFT))
        moveVector *= 2;

    if (!moveVector.Equals(float3::zero))
    {
        movementHeld_ = Clamp(movementHeld_ + (frameTime * 4.f), 0.f, 1.0f);
        t.pos += t.Orientation() * (cMoveSpeed * frameTime * moveVector * movementHeld_);
        changed = true;
    }
    else
        movementHeld_ = 0.f;

    // If some other camera (like avatar) is active, do not actually move, only transmit the move signals
    if (changed && cameraEntity == lastCamera_)
        placeable->transform.Set(t);
}

void CameraApplication::OnMouseScroll(MouseEvent* e)
{
    ZoomChanged.Emit(e->relativeZ);
}

}

extern "C"
{

DLLEXPORT void TundraPluginMain(Tundra::Framework *fw)
{
    fw->RegisterModule(new Tundra::CameraApplication(fw));
}

}
