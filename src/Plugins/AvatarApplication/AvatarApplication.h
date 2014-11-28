// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IModule.h"
#include "SceneFwd.h"
#include "IAttribute.h"
#include "InputFwd.h"
#include "Math/float3.h"

namespace Tundra
{

/// Avatar component, description asset and hardcoded C++ avatar control functionality.
/** Once proper script support is in place, the application part can be removed. */
class AvatarApplication : public IModule
{
    OBJECT(AvatarApplication);

public:
    AvatarApplication(Framework* owner);
    ~AvatarApplication();

    /// Frame-based module update
    void Update(float frametime) override;

private:
    /// React to scene creation
    void OnSceneCreated(Scene *scene, AttributeChange::Type change);
    /// React to entity creation
    void OnEntityCreated(Entity* entity, AttributeChange::Type change);
    /// Handle rotate delta from CameraApplication
    void OnRotateChanged(float3 rotDelta);
    /// Handle move vector change from CameraApplication
    void OnMoveChanged(float3 moveVector);
    /// Handle zoom change from CameraApplication
    void OnZoomChanged(int delta);
    /// Update avatar camera's position to match own avatar
    void UpdateCameraPosition(float frametime);
    /// Update animations of all avatars in the scene
    void UpdateAvatarAnimations();

    SceneWeakPtr lastScene_;
    EntityWeakPtr ownAvatarEntity_;
    EntityWeakPtr avatarCameraEntity_;

    float yaw, pitch;
    float cameraDistance;
    float newCameraDistance;
    bool firstPerson;
    float3 lastSentMove;

    void Load() override;
    void Initialize() override;
    void Uninitialize() override;
};

}
