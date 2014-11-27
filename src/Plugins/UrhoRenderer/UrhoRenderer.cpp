// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "UrhoRenderer.h"
#include "GraphicsWorld.h"
#include "Framework.h"
#include "Placeable.h"
#include "Mesh.h"
#include "AnimationController.h"
#include "Camera.h"
#include "Light.h"
#include "EnvironmentLight.h"
#include "Terrain.h"
#include "Sky.h"
#include "ParticleSystem.h"
#include "IComponentFactory.h"
#include "ConfigAPI.h"
#include "SceneAPI.h"
#include "AssetAPI.h"
#include "Entity.h"
#include "Scene/Scene.h"
#include "LoggingFunctions.h"

#include "TextureAsset.h"
#include "UrhoMeshAsset.h"
#include "Ogre/OgreMeshAsset.h"
#include "Ogre/OgreMaterialAsset.h"
#include "Ogre/OgreSkeletonAsset.h"
#include "Ogre/DefaultOgreMaterialProcessor.h"
#include "Ogre/OgreParticleAsset.h"
#include "GenericAssetFactory.h"

#include <Engine/Core/CoreEvents.h>
#include <Engine/Core/ProcessUtils.h>
#include <Engine/Graphics/Camera.h>
#include <Engine/Graphics/Graphics.h>
#include <Engine/Graphics/GraphicsEvents.h>
#include <Engine/Graphics/Renderer.h>
#include <Engine/Graphics/Viewport.h>

namespace Tundra
{

UrhoRenderer::UrhoRenderer(Framework* owner) :
    IModule("UrhoRenderer", owner)
{
    // Register default material convertor
    RegisterOgreMaterialProcessor(new DefaultOgreMaterialProcessor(GetContext()));
}

UrhoRenderer::~UrhoRenderer()
{
}

void UrhoRenderer::Load()
{
    SceneAPI* scene = framework->Scene();
    scene->RegisterComponentFactory(ComponentFactoryPtr(new GenericComponentFactory<Placeable>()));
    scene->RegisterComponentFactory(ComponentFactoryPtr(new GenericComponentFactory<Mesh>()));
    scene->RegisterComponentFactory(ComponentFactoryPtr(new GenericComponentFactory<Camera>()));
    scene->RegisterComponentFactory(ComponentFactoryPtr(new GenericComponentFactory<Light>()));
    scene->RegisterComponentFactory(ComponentFactoryPtr(new GenericComponentFactory<AnimationController>()));
    scene->RegisterComponentFactory(ComponentFactoryPtr(new GenericComponentFactory<EnvironmentLight>()));
    scene->RegisterComponentFactory(ComponentFactoryPtr(new GenericComponentFactory<Terrain>()));
    scene->RegisterComponentFactory(ComponentFactoryPtr(new GenericComponentFactory<Sky>()));
    scene->RegisterComponentFactory(ComponentFactoryPtr(new GenericComponentFactory<ParticleSystem>));

    /// \todo Check and add new supported texture extensions
    StringList textureExtensions;
    textureExtensions.Push(".crn"); // CRN to DDS decompression implemented in TextureAsset
    textureExtensions.Push(".dds");
    textureExtensions.Push(".png");
    textureExtensions.Push(".jpeg");
    textureExtensions.Push(".jpg");
    textureExtensions.Push(".gif");
    textureExtensions.Push(".bmp");
    textureExtensions.Push(".tga");
    textureExtensions.Push(".psd");

    framework->Asset()->RegisterAssetTypeFactory(AssetTypeFactoryPtr(new GenericAssetFactory<UrhoMeshAsset>("UrhoMesh", ".mdl")));
    framework->Asset()->RegisterAssetTypeFactory(AssetTypeFactoryPtr(new GenericAssetFactory<OgreMeshAsset>("OgreMesh", ".mesh")));
    framework->Asset()->RegisterAssetTypeFactory(AssetTypeFactoryPtr(new GenericAssetFactory<OgreMaterialAsset>("OgreMaterial", ".material")));
    framework->Asset()->RegisterAssetTypeFactory(AssetTypeFactoryPtr(new GenericAssetFactory<OgreSkeletonAsset>("OgreSkeleton", ".skeleton")));
    framework->Asset()->RegisterAssetTypeFactory(AssetTypeFactoryPtr(new GenericAssetFactory<OgreParticleAsset>("OgreParticle", ".particle")));
    framework->Asset()->RegisterAssetTypeFactory(AssetTypeFactoryPtr(new GenericAssetFactory<TextureAsset>("Texture", textureExtensions)));
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

        // Track window position and screen mode changes to keep config up-to-date
        SubscribeToEvent(Urho3D::E_WINDOWPOS, HANDLER(UrhoRenderer, HandleScreenModeChange));
        SubscribeToEvent(Urho3D::E_SCREENMODE, HANDLER(UrhoRenderer, HandleScreenModeChange));
    
        // Disable shadows completely for now on mobile devices, as the shadow bias is problematic, and it consumes GPU performance
        // Also disable specular highlights for per-pixel lighting
        if (Urho3D::GetPlatform() == "Android" || Urho3D::GetPlatform() == "iOS")
        {
            rend->SetDrawShadows(false);
            rend->SetSpecularLighting(false);
        }
    }
}

void UrhoRenderer::Uninitialize()
{
    framework->RegisterRenderer(0);
    Urho3D::Renderer* rend = GetSubsystem<Urho3D::Renderer>();
    // Let go of the viewport that we created. If done later at Urho Context destruction time, may cause a crash
    if (rend)
        rend->SetViewport(0, 0);
}

void UrhoRenderer::HandleScreenModeChange(StringHash /*eventType*/, VariantMap& /*eventData*/)
{
    ConfigAPI *config = framework->Config();
    if (!config)
        return;

    HashMap<String, Variant> data;

    Urho3D::Graphics* graphics = GetSubsystem<Urho3D::Graphics>();
    if (graphics)
    {
        data["window position"] = graphics->GetWindowPosition();
        data["window size"] = Urho3D::IntVector2(graphics->GetWidth(), graphics->GetHeight());
        data["window fullscreen"] = graphics->GetFullscreen();
    
        /* Store potentially frequent runtime changes in memory only.
        The changes will be written to disk latest at a clean Framework exit. */
        ConfigFile &f = config->GetFile(ConfigAPI::FILE_FRAMEWORK);
        f.Set(ConfigAPI::SECTION_GRAPHICS, data);
    }
}

Entity *UrhoRenderer::MainCamera()
{
    Entity *mainCameraEntity = activeMainCamera.Get();
    if (!mainCameraEntity)
        return nullptr;

    if (!mainCameraEntity->ParentScene() || !mainCameraEntity->Component<Camera>())
    {
        SetMainCamera(0);
        return nullptr;
    }
    return mainCameraEntity;
}

Camera *UrhoRenderer::MainCameraComponent()
{
    Entity *mainCamera = MainCamera();
    if (!mainCamera)
        return nullptr;
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

    return nullptr;
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
            LogWarning("Cannot activate camera '" + mainCameraEntity->Name() + "': It does not have a Camera component!");
    }
    if (mainCameraEntity && !mainCameraEntity->ParentScene()) // If the new to-be camera is not in a scene, don't add it as active.
    {
        LogWarning("Cannot activate camera \"" + mainCameraEntity->Name() + "\": It is not attached to a scene!");
        activeMainCamera.Reset();
        newActiveCamera = 0;
    }

    if (!activeMainCamera.Lock() || !newActiveCamera)
        LogWarning("Setting main window camera to null!");

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
        LogWarning("Could not set active camera, no viewport defined");

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

void UrhoRenderer::RegisterOgreMaterialProcessor(IOgreMaterialProcessor* processor, bool addFirst)
{
    if (addFirst)
        materialProcessors.Insert(0, SharedPtr<IOgreMaterialProcessor>(processor));
    else
        materialProcessors.Push(SharedPtr<IOgreMaterialProcessor>(processor));
}

void UrhoRenderer::UnregisterOgreMaterialProcessor(IOgreMaterialProcessor* processor)
{
    for (Vector<SharedPtr<IOgreMaterialProcessor> >::Iterator i = materialProcessors.Begin(); i != materialProcessors.End(); ++i)
    {
        if (i->Get() == processor)
        {
            materialProcessors.Erase(i);
            return;
        }
    }

    LogWarning("Could not find Ogre material processor to remove");
}

IOgreMaterialProcessor* UrhoRenderer::FindOgreMaterialProcessor(const Ogre::MaterialParser& material) const
{
    for (Vector<SharedPtr<IOgreMaterialProcessor> >::ConstIterator i = materialProcessors.Begin(); i != materialProcessors.End(); ++i)
    {
        if ((*i)->CanConvert(material))
            return *i;
    }

    return nullptr;
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
