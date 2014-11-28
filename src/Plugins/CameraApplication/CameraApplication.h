// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IModule.h"
#include "SceneFwd.h"
#include "IAttribute.h"
#include "AttributeChangeType.h"
#include "InputFwd.h"
#include "Signals.h"
#include "CameraAPI.h"
#include "Math/float3.h"

#include <Input.h>

namespace Urho3D
{
    class UIElement;
}

namespace Tundra
{

/// Simple freelook camera functionality. Creates a camera to scene and allows it to be controlled with mouse & keys
class CAMERA_API CameraApplication : public IModule
{
    OBJECT(CameraApplication);

public:
    CameraApplication(Framework* owner);
    ~CameraApplication();

    /// Frame-based module update
    void Update(float frametime) override;

    /// Rotate control signal. Transmitted each frame there is mouse/touch rotate
    Signal1<float3> RotateChanged;
    /// Move control signal. Transmitted each frame the move vector changes
    Signal1<float3> MoveChanged;
    /// Zoom control signal. Transmitted each time when the mouse wheel is rotated. \todo Other control methods, such as pinch zoom
    Signal1<int> ZoomChanged;

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
    /// Handle mouse scroll event
    void OnMouseScroll(MouseEvent* e);

    void Load() override;
    void Initialize() override;
    void Uninitialize() override;

    SceneWeakPtr lastScene_;
    EntityWeakPtr lastCamera_;
    SDL_JoystickID joystickId_;

    SharedPtr<InputContext> inputContext_;

    float3 lastMoveVector_;
    float movementHeld_;
};

}
