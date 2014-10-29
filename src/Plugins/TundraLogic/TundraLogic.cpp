// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "TundraLogic.h"
#include "Framework.h"
#include "FrameAPI.h"
#include "IRenderer.h"
#include "SceneAPI.h"
#include "Scene/Scene.h"
#include "LoggingFunctions.h"
#include "AssetAPI.h"
#include "LocalAssetProvider.h"
#include "LocalAssetStorage.h"

#include <Timer.h>

namespace Tundra
{

TundraLogic::TundraLogic(Framework* owner) :
    IModule("TundraLogic", owner),
    startupSceneLoaded(false)
{
}

TundraLogic::~TundraLogic()
{
}

void TundraLogic::Load()
{
}

void TundraLogic::Initialize()
{
    framework->Frame()->Updated.Connect(this, &TundraLogic::OnUpdate);

    // Add the System asset storage
    String systemAssetDir = framework->InstallationDirectory() + "Data/Assets";
    IAssetStorage* storage = framework->Asset()->AssetProvider<LocalAssetProvider>()->AddStorageDirectory(systemAssetDir, "System", true, false);
    storage->SetReplicated(false); // If we are a server, don't pass this storage to the client.

}

void TundraLogic::Uninitialize()
{
}

void TundraLogic::OnUpdate(float /*frametime*/)
{
    // Load startup scene on first round of the main loop, as all modules are initialized
    if (!startupSceneLoaded)
    {
        LoadStartupScene();
        startupSceneLoaded = true;
    }

}

void TundraLogic::LoadStartupScene()
{
    Scene *scene = framework->Scene()->MainCameraScene();
    if (!scene)
    {
        scene = framework->Scene()->CreateScene("TundraServer", true, true).Get();
    }

    bool hasFile = framework->HasCommandLineParameter("--file");
    StringVector files = framework->CommandLineParameters("--file");
    if (hasFile && files.Empty())
        LogError("TundraLogicModule: --file specified without a value.");

    bool loaded = false;

    foreach(String file, files)
    {
        /// \todo File asset handling. Now just loaded from the filesystem.
        loaded |= LoadScene(file, false, false);
    }

    if (loaded)
    {
        // Create a camera at 0,0,0 to show something
        /// \todo Do not do here, rather should be done by CameraApplication
        Entity* entity = scene->CreateEntity();
        entity->CreateComponent(20); // Placeable
        entity->CreateComponent(15); // Camera
        IRenderer* renderer = framework->Renderer();
        if (renderer)
            renderer->SetMainCamera(entity);
    }
}

bool TundraLogic::LoadScene(String filename, bool clearScene, bool useEntityIDsFromFile)
{
    Scene *scene = framework->Scene()->MainCameraScene();
    if (!scene)
    {
        LogError("TundraLogicModule::LoadScene: No active scene found!");
        return false;
    }
    filename = filename.Trimmed();
    if (filename.Empty())
    {
        LogError("TundraLogicModule::LoadScene: Empty filename given!");
        return false;
    }

    LogInfo("Loading startup scene from " + filename + " ...");
    Urho3D::HiresTimer timer;

    bool useBinary = filename.Find(".tbin", 0, false) != String::NPOS;
    Vector<Entity *> entities;
    if (useBinary)
        entities = scene->LoadSceneBinary(filename, clearScene, useEntityIDsFromFile, AttributeChange::Default);
    else
        entities = scene->LoadSceneXML(filename, clearScene, useEntityIDsFromFile, AttributeChange::Default);
    LogInfo("Loading of startup scene finished. " + String(entities.Size()) + " entities created in " + String((int)(timer.GetUSec(true) / 1000)) + " msecs.");
    return entities.Size() > 0;
}

extern "C"
{

DLLEXPORT void TundraPluginMain(Tundra::Framework *fw)
{
    fw->RegisterModule(new Tundra::TundraLogic(fw));
}

}

}

