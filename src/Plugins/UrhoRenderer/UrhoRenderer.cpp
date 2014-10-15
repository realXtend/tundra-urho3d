// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "UrhoRenderer.h"
#include "GraphicsWorld.h"
#include "Framework.h"
#include "Placeable.h"
#include "Mesh.h"
#include "Camera.h"
#include "IComponentFactory.h"
#include "SceneAPI.h"
#include "Entity.h"
#include "Scene/Scene.h"

#include <Log.h>
#include <Engine/Graphics/Camera.h>
#include <Engine/Graphics/Graphics.h>
#include <Engine/Graphics/Renderer.h>
#include <Engine/Graphics/Viewport.h>

namespace Tundra
{

UrhoRenderer::UrhoRenderer(Framework* owner) :
    IModule("UrhoRenderer", owner)
{
}

UrhoRenderer::~UrhoRenderer()
{
}

void UrhoRenderer::Load()
{
    /// \todo Register component & asset factories
    SceneAPI* scene = framework->Scene();
    scene->RegisterComponentFactory(ComponentFactoryPtr(new GenericComponentFactory<Placeable>()));
    scene->RegisterComponentFactory(ComponentFactoryPtr(new GenericComponentFactory<Mesh>()));
    scene->RegisterComponentFactory(ComponentFactoryPtr(new GenericComponentFactory<Camera>()));
}

void UrhoRenderer::Initialize()
{
    framework->RegisterRenderer(this);

    // Connect to scene change signals.
    framework->Scene()->SceneCreated.Connect(this, &UrhoRenderer::CreateGraphicsWorld);
    framework->Scene()->SceneAboutToBeRemoved.Connect(this, &UrhoRenderer::RemoveGraphicsWorld);

    // Enable the main (full-screen) viewport
    Urho3D::Renderer* rend = GetSubsystem<Urho3D::Renderer>();
    if (rend)
    {
        rend->SetNumViewports(1);
        rend->SetViewport(0, new Urho3D::Viewport(context_));
    }

    /// \todo Remove
    // Test code to get something on the screen
    ScenePtr scene = framework->Scene()->CreateScene("Test", true, true);
    {
        EntityPtr entity = scene->CreateEntity();
        entity->SetName("TestBox");
        Placeable* pl = entity->CreateComponent<Placeable>();
        entity->CreateComponent<Mesh>();
        Transform newTransform;
        newTransform.pos = float3(0.0f, 0.0f, 10.0f);
        pl->transform.Set(newTransform);
    }
    {
        EntityPtr entity = scene->CreateEntity();
        entity->SetName("Camera");
        entity->CreateComponent<Placeable>();
        Camera* cam = entity->CreateComponent<Camera>();
        cam->SetActive();
    }

    scene->SaveSceneXML("Test.txml", true, true);
}

void UrhoRenderer::Uninitialize()
{
    framework->RegisterRenderer(0);
}

Entity *UrhoRenderer::MainCamera()
{
    Entity *mainCameraEntity = activeMainCamera.Lock().Get();
    if (!mainCameraEntity)
        return 0;

    if (!mainCameraEntity->ParentScene() || !mainCameraEntity->Component<Camera>())
    {
        SetMainCamera(0);
        return 0;
    }
    return mainCameraEntity;
}

Camera *UrhoRenderer::MainCameraComponent()
{
    Entity *mainCamera = MainCamera();
    if (!mainCamera)
        return 0;
    return mainCamera->Component<Camera>().Get();
}

Scene *UrhoRenderer::MainCameraScene()
{
    Entity *mainCamera = MainCamera();
    Scene *scene = mainCamera ? mainCamera->ParentScene() : 0;
    if (scene)
        return scene;

    // If there is no active camera, return the first scene on the list.
    const SceneMap &scenes = framework->Scene()->Scenes();
    if (scenes.Size() > 0)
        return scenes.Begin()->second_.Get();

    return 0;
}

void UrhoRenderer::SetMainCamera(Entity *mainCameraEntity)
{
    activeMainCamera = mainCameraEntity;

    Urho3D::Camera *newActiveCamera = 0;
    Camera *cameraComponent = mainCameraEntity ? mainCameraEntity->Component<Camera>().Get() : 0;
    if (cameraComponent)
        newActiveCamera = cameraComponent->UrhoCamera();
    else
    {
        activeMainCamera.Reset();
        if (mainCameraEntity)
            LOGWARNING("Cannot activate camera '" + mainCameraEntity->Name() + "': It does not have a Camera component!");
    }
    if (mainCameraEntity && !mainCameraEntity->ParentScene()) // If the new to-be camera is not in a scene, don't add it as active.
    {
        LOGWARNING("Cannot activate camera \"" + mainCameraEntity->Name() + "\": It is not attached to a scene!");
        activeMainCamera.Reset();
        newActiveCamera = 0;
    }

    if (!activeMainCamera.Lock() || !newActiveCamera)
        LOGWARNING("Setting main window camera to null!");

    Urho3D::Renderer* rend = GetSubsystem<Urho3D::Renderer>();
    if (!rend)
        return; // In headless mode the renderer doesn't exist

    Urho3D::Viewport* vp = rend->GetViewport(0);
    if (vp)
    {
        vp->SetCamera(newActiveCamera);
        vp->SetScene(mainCameraEntity->ParentScene()->Subsystem<GraphicsWorld>()->UrhoScene());
    }
    else
        LOGWARNING("Could not set active camera, no viewport defined");

    MainCameraChanged.Emit(mainCameraEntity);
}

int UrhoRenderer::WindowWidth() const
{
    Urho3D::Graphics* gfx = GetSubsystem<Urho3D::Graphics>();
    return gfx ? gfx->GetWidth() : 0;
}

int UrhoRenderer::WindowHeight() const
{
    Urho3D::Graphics* gfx = GetSubsystem<Urho3D::Graphics>();
    return gfx ? gfx->GetHeight() : 0;
}

void UrhoRenderer::CreateGraphicsWorld(Scene *scene, AttributeChange::Type)
{
    // Add an OgreWorld to the scene
    if (scene->ViewEnabled())
        scene->AddSubsystem(new GraphicsWorld(this, scene));
}

void UrhoRenderer::RemoveGraphicsWorld(Scene *scene, AttributeChange::Type)
{
    scene->RemoveSubsystem(scene->Subsystem<GraphicsWorld>());
}

}

extern "C"
{

DLLEXPORT void TundraPluginMain(Tundra::Framework *fw)
{
    fw->RegisterModule(new Tundra::UrhoRenderer(fw));
}

}
