// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "Win.h"
#include "AvatarApplication.h"
#include "BinaryAsset.h"
#include "GenericAssetFactory.h"
#include "IComponentFactory.h"
#include "Framework.h"
#include "Placeable.h"
#include "AssetAPI.h"
#include "AvatarDescAsset.h"
#include "CameraApplication.h"
#include "../UrhoRenderer/Camera.h"
#include "../UrhoRenderer/AnimationController.h"
#include "TundraLogic.h"
#include "Client.h"
#include "SyncManager.h"
#include "SceneAPI.h"
#include "Avatar.h"
#include "Entity.h"
#include "Mesh.h"
#include "Scene/Scene.h"
#include "LoggingFunctions.h"
#include "Math/float3.h"
#include "Math/MathFunc.h"
#include "InputAPI.h"
#include "InputContext.h"

#include <Urho3D/Scene/Node.h>

namespace Tundra
{

const float cMaxCameraDistance = 15.0f;

AvatarApplication::AvatarApplication(Framework* owner) :
    IModule("AvatarApplication", owner),
    yaw(0.0f),
    pitch(0.0f),
    cameraDistance(7.0f),
    firstPerson(false),
    lastSentMove(float3::zero)
{
    newCameraDistance = cameraDistance;
}

AvatarApplication::~AvatarApplication()
{
}

void AvatarApplication::Load()
{
    SceneAPI* scene = framework->Scene();
    scene->RegisterComponentFactory(ComponentFactoryPtr(new GenericComponentFactory<Avatar>()));

    framework->Asset()->RegisterAssetTypeFactory(AssetTypeFactoryPtr(new GenericAssetFactory<AvatarDescAsset>("Avatar", ".avatar")));
    framework->Asset()->RegisterAssetTypeFactory(AssetTypeFactoryPtr(new BinaryAssetFactory("AvatarAttachment", ".attachment")));
}

void AvatarApplication::Initialize()
{
    CameraApplication* cameraApp = framework->Module<CameraApplication>();
    if (!cameraApp)
    {
        LogError("CameraApplication not present, can not initialize controls");
        return;
    }
    cameraApp->RotateChanged.Connect(this, &AvatarApplication::OnRotateChanged);
    cameraApp->MoveChanged.Connect(this, &AvatarApplication::OnMoveChanged);
    cameraApp->ZoomChanged.Connect(this, &AvatarApplication::OnZoomChanged);

    // Connect to scene change signals.
    framework->Scene()->SceneCreated.Connect(this, &AvatarApplication::OnSceneCreated);
}

void AvatarApplication::OnSceneCreated(Scene *scene, AttributeChange::Type)
{
    if (!scene->ViewEnabled() || framework->IsHeadless())
        return;

    lastScene_ = scene;
    scene->EntityCreated.Connect(this, &AvatarApplication::OnEntityCreated);
}

void AvatarApplication::OnEntityCreated(Entity* entity, AttributeChange::Type)
{
    // Check if own avatar was created
    TundraLogic* logic = framework->Module<TundraLogic>();
    if (!logic)
        return;
    
    Client* client = logic->Client();
    if (!client->IsConnected())
        return;

    uint connectionID = client->ConnectionId();
    String avatarEntityName = "Avatar" + String(connectionID);
    if (entity->Name() == avatarEntityName)
    {
        LogInfo("AvatarApplication found own avatar " + avatarEntityName);
        ownAvatarEntity_ = entity;

        // Create and activate avatar camera now
        StringVector cameraComponents;
        cameraComponents.Push("Placeable");
        cameraComponents.Push("Camera");
        Entity* avatarCamera = lastScene_->CreateEntity(0, cameraComponents, AttributeChange::LocalOnly, false, false, true);
        avatarCamera->SetName("AvatarCamera");
        avatarCameraEntity_ = avatarCamera;
        avatarCamera->Component<Camera>()->SetActive();
        logic->SyncManager()->SetObserver(EntityPtr(avatarCameraEntity_));
        Placeable* avatarPlaceable = entity->Component<Placeable>();
        // Init yaw/pitch
        if (avatarPlaceable)
        {
            yaw = avatarPlaceable->WorldOrientation().ToEulerZYX().y;
            pitch = 0;
        }
        // Update camera position immediately
        UpdateCameraPosition(0.0f);
    }
}

void AvatarApplication::Uninitialize()
{
}

void AvatarApplication::Update(float frametime)
{
    UpdateAvatarAnimations();
    UpdateCameraPosition(frametime);
}

void AvatarApplication::UpdateAvatarAnimations()
{
    if (!lastScene_)
        return;
    EntityVector entities = lastScene_->EntitiesWithComponent<Avatar>();
    for (uint i = 0; i < entities.Size(); ++i)
    {
        AnimationController* ctrl = entities[i]->Component<AnimationController>();
        if (ctrl)
        {
            // Very simple operation: enable the exclusive animation specified in the "animationState" freedata field
            if (ctrl->animationState.Get().Length())
                ctrl->EnableExclusiveAnimation(ctrl->animationState.Get(), true, 0.1f, 0.1f);
        }
    }
}

void AvatarApplication::UpdateCameraPosition(float frametime)
{
    if (!ownAvatarEntity_ || !avatarCameraEntity_)
        return;
    Placeable* avatarPlaceable = ownAvatarEntity_->Component<Placeable>();
    Placeable* cameraPlaceable = avatarCameraEntity_->Component<Placeable>();
    Mesh* avatarMesh = ownAvatarEntity_->Component<Mesh>();
    Avatar* av = ownAvatarEntity_->Component<Avatar>();
    if (!cameraPlaceable || !avatarPlaceable || !avatarMesh || !av)
        return;

    cameraDistance = Lerp(cameraDistance, newCameraDistance, frametime * 10.0f);
    firstPerson = cameraDistance < 0.2f;
    Urho3D::Node* headBoneNode = nullptr;

    if (firstPerson)
        headBoneNode = avatarMesh->BoneNode(av->AvatarProperty("headbone"));

    Transform t = cameraPlaceable->transform.Get();

    if (!firstPerson || !headBoneNode)
    {
        t.rot = float3(pitch, yaw, 0.0f);
        t.pos = avatarPlaceable->WorldPosition() + t.Orientation() * float3(0.0f, 1.0f, cameraDistance);
    }
    else
    {
        t.rot = float3(0.0f, yaw, 0.0f);
        t.pos = headBoneNode->GetWorldPosition() + t.Orientation() * float3(0.0f, 0.1f, -0.15f); // Position slightly up & forward to avoid clipping into head
        t.rot = float3(pitch, yaw, 0.0f);
    }

    cameraPlaceable->transform.Set(t);
}

void AvatarApplication::OnRotateChanged(float3 rotDelta)
{
    yaw += rotDelta.y;
    pitch += rotDelta.x;
    pitch = Clamp(pitch, -75.0f, 75.0f);

    if (ownAvatarEntity_)
    {
        // Execute rotate action on server to force yaw to client's value
        ownAvatarEntity_->Exec(EntityAction::Server, "SetRotation", String(yaw));
    }
}

void AvatarApplication::OnMoveChanged(float3 moveVector)
{
    if (!ownAvatarEntity_)
        return;

    // The move vector as received from CameraApplication is in node local space. For avatar physics positive z = forward instead
    moveVector.z *= -1.0f;

    if (lastSentMove.x > 0.0f && moveVector.x <= 0.0f)
        ownAvatarEntity_->Exec(EntityAction::Server, "Stop", "right");
    if (lastSentMove.x < 0.0f && moveVector.x >= 0.0f)
        ownAvatarEntity_->Exec(EntityAction::Server, "Stop", "left");
    if (lastSentMove.z > 0.0f && moveVector.z <= 0.0f)
        ownAvatarEntity_->Exec(EntityAction::Server, "Stop", "forward");
    if (lastSentMove.z < 0.0f && moveVector.z >= 0.0f)
        ownAvatarEntity_->Exec(EntityAction::Server, "Stop", "back");
    if (lastSentMove.y > 0.0f && moveVector.y <= 0.0f)
        ownAvatarEntity_->Exec(EntityAction::Server, "Stop", "up");
    if (lastSentMove.y < 0.0f && moveVector.y >= 0.0f)
        ownAvatarEntity_->Exec(EntityAction::Server, "Stop", "down");

    if (moveVector.x > 0.0f)
        ownAvatarEntity_->Exec(EntityAction::Server, "Move", "right");
    if (moveVector.x < 0.0f)
        ownAvatarEntity_->Exec(EntityAction::Server, "Move", "left");
    if (moveVector.z > 0.0f)
        ownAvatarEntity_->Exec(EntityAction::Server, "Move", "forward");
    if (moveVector.z < 0.0f)
        ownAvatarEntity_->Exec(EntityAction::Server, "Move", "back");
    if (moveVector.y > 0.0f)
        ownAvatarEntity_->Exec(EntityAction::Server, "Move", "up");
    if (moveVector.y < 0.0f)
        ownAvatarEntity_->Exec(EntityAction::Server, "Move", "down");

    lastSentMove = moveVector;
}

void AvatarApplication::OnZoomChanged(int delta)
{
    newCameraDistance -= (float)delta;
    newCameraDistance = Clamp(newCameraDistance, 0.0f, cMaxCameraDistance);
}

}

extern "C"
{

DLLEXPORT void TundraPluginMain(Tundra::Framework *fw)
{
    fw->RegisterModule(new Tundra::AvatarApplication(fw));
}

}
