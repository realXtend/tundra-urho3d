// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IModule.h"
#include "SceneFwd.h"
#include "IAttribute.h"
#include "AttributeChangeType.h"

namespace Tundra
{

/// Simply freelook camera functionality. Creates a camera to scene and allows it to be controlled with mouse & keys
class CameraApplication : public IModule
{
    OBJECT(CameraApplication);

public:
    CameraApplication(Framework* owner);
    ~CameraApplication();

    /// Frame-based module update
    void Update(float frametime) override;

private:
    /// React to scene creation.
    void OnSceneCreated(Scene *scene, AttributeChange::Type change);

    /// Create camera to the last created scene
    void CreateCamera();
    /// Move camera by mouse & keys
    void MoveCamera(Entity* cameraEntity, float frametime);

    SceneWeakPtr lastScene;

    void Load() override;
    void Initialize() override;
    void Uninitialize() override;
};

}
