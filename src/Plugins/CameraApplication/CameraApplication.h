// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IModule.h"
#include "SceneFwd.h"
#include "IAttribute.h"
#include "AttributeChangeType.h"
#include "InputFwd.h"

#include <Input.h>

namespace Urho3D
{
    class UIElement;
}

namespace Tundra
{

/// Simple freelook camera functionality. Creates a camera to scene and allows it to be controlled with mouse & keys
class CameraApplication : public IModule
{
    OBJECT(CameraApplication);

public:
    CameraApplication(Framework* owner);
    ~CameraApplication();

    /// Frame-based module update
    void Update(float frametime) override;

private:
    /// React to scene creation and removal.
    void OnSceneCreated(Scene *scene, AttributeChange::Type change);
    void OnSceneAboutToBeRemoved(Scene *scene, AttributeChange::Type change);

    /// Create camera to the last created scene
    void CreateCamera();
    /// Move camera by mouse & keys
    void MoveCamera(Entity* cameraEntity, float frametime);
    /// Check for creation of an entity; whether it is the camera spawnpos
    void CheckCameraSpawnPos(Entity* entity, AttributeChange::Type change);

    void Load() override;
    void Initialize() override;
    void Uninitialize() override;

    SceneWeakPtr lastScene_;
    SDL_JoystickID joystickId_;

    SharedPtr<InputContext> inputContext_;

    float movementHeld_;
};

}
