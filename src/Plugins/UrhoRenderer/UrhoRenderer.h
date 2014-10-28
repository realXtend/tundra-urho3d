// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IModule.h"
#include "IRenderer.h"
#include "SceneFwd.h"
#include "AttributeChangeType.h"
#include "UrhoModuleFwd.h"
#include "UrhoModuleApi.h"
#include "Signals.h"

namespace Tundra
{

/// A renderer module using Urho3D
class URHO_MODULE_API UrhoRenderer : public IModule, public IRenderer
{
    OBJECT(UrhoRenderer);

public:
    UrhoRenderer(Framework* owner);
    ~UrhoRenderer();

    /// Returns the Entity which contains the currently active camera that is used to render on the main window.
    /// The returned Entity is guaranteed to have an Camera component, and it is guaranteed to be attached to a scene.
    Entity *MainCamera();

    /// Returns the Camera of the main camera, or 0 if no main camera is active.
    Camera *MainCameraComponent();

    /// Returns the Scene the current active main camera is in, or 0 if no main camera is active.
    Scene *MainCameraScene();

    /// Sets the given Entity as the main camera for the main window.
    /** This function fails if the given Entity does not have an Camera component, or if the given Entity is not attached to a scene.
        Whenever the main camera is changed, the signal MainCameraChanged is triggered. */
    void SetMainCamera(Entity *mainCameraEntity) override;

    /// Emitted every time the main window active camera changes.
    /** The pointer specified in this signal may be null, if the main camera was set to null.
        If the specified entity is non-zero, it is guaranteed to have an Camera component, and it is attached to some scene. */
    Signal1<Entity*> MainCameraChanged;

    /// Returns window width, or 0 if no render window
    int WindowWidth() const;

    /// Returns window height, or 0 if no render window
    int WindowHeight() const;

private:
    void Load() override;
    void Initialize() override;
    void Uninitialize() override;

    // Handles Urho3D::Graphics E_SCREENMODE & E_WINDOWPOS events.
    void HandleScreenModeChange(StringHash eventType, VariantMap &eventData);

    /// Creates GraphicsWorld for a Scene.
    void CreateGraphicsWorld(Scene *scene, AttributeChange::Type);
    /// Removes GraphicsWorld from a Scene.
    void RemoveGraphicsWorld(Scene *scene, AttributeChange::Type);

    /// Stores the camera that is active in the main window.
    EntityWeakPtr activeMainCamera;
};

}
