// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "TundraLogic.h"
#include "KristalliProtocol.h"
#include "SyncManager.h"
#include "Client.h"
#include "Server.h"
#include "Framework.h"
#include "FrameAPI.h"
#include "ConfigAPI.h"
#include "ConsoleAPI.h"
#include "IRenderer.h"
#include "SceneAPI.h"
#include "Scene/Scene.h"
#include "UserConnectedResponseData.h"
#include "LoggingFunctions.h"

#include "AssetAPI.h"
#include "IAsset.h"

#include "LocalAssetProvider.h"
#include "LocalAssetStorage.h"

#include <Timer.h>
#include <StringUtils.h>

namespace Tundra
{

static const unsigned short cDefaultPort = 2345;

TundraLogic::TundraLogic(Framework* owner) :
    IModule("TundraLogic", owner)
{
}

TundraLogic::~TundraLogic()
{
}

void TundraLogic::Load()
{
    kristalliProtocol_ = SharedPtr<Tundra::KristalliProtocol>(new Tundra::KristalliProtocol(this));
    kristalliProtocol_->Load();
}

void TundraLogic::Initialize()
{
    client_ = SharedPtr<Tundra::Client>(new Tundra::Client(this));
    server_ = SharedPtr<Tundra::Server>(new Tundra::Server(this));
    syncManager_ = SharedPtr<Tundra::SyncManager>(new Tundra::SyncManager(this)); // Syncmanager expects client (and server) to exist
    
    framework->Console()->RegisterCommand("connect", "Connects to a server. Usage: connect(address,port,username,password,protocol)")->ExecutedWith.Connect(
        this, &TundraLogic::HandleLogin);

    framework->Console()->RegisterCommand("disconnect", "Disconnects from a server.", client_.Get(), &Client::Logout);

    kristalliProtocol_->Initialize();

    // Load startup parameters once we are running.
    framework->Frame()->DelayedExecute(0.0f).Connect(this, &TundraLogic::ReadStartupParameters);

    client_->Connected.Connect(this, &TundraLogic::ClientConnectedToServer);
    client_->Disconnected.Connect(this, &TundraLogic::ClientDisconnectedFromServer);
}

void TundraLogic::HandleLogin(const StringVector &params) const
{
    String address = "localhost";
    if (params.Size() >= 1)
        address = params[0];
    unsigned short port = 3456;
    if (params.Size() >= 2)
        port = (unsigned short)Urho3D::ToUInt(params[1]);
    String username = "TundraM";
    if (params.Size() >= 3)
        username = params[2];
    String password = "";
    if (params.Size() >= 4)
        password = params[3];
    String protocol = "udp";
    if (params.Size() >= 5)
        protocol = params[4];

    client_->Login(address, port, username, password, protocol);
}

void TundraLogic::Uninitialize()
{
    kristalliProtocol_->Uninitialize();
    kristalliProtocol_.Reset();
    syncManager_.Reset();
    client_.Reset();
    server_.Reset();
}

void TundraLogic::Update(float frametime)
{
    kristalliProtocol_->Update(frametime);
    if (client_)
        client_->Update(frametime);
    if (server_)
        server_->Update(frametime);
    // Run scene sync
    if (syncManager_)
        syncManager_->Update(frametime);
    // Run scene interpolation
    Scene *scene = GetFramework()->Scene()->MainCameraScene();
    if (scene)
        scene->UpdateAttributeInterpolations(frametime);
    
}

void TundraLogic::ReadStartupParameters(float /*time*/)
{
    // Check whether server should be auto started.
    const bool autoStartServer = framework->HasCommandLineParameter("--server");
    const bool hasPortParam = framework->HasCommandLineParameter("--port");
    ushort autoStartServerPort = cDefaultPort;
    if (hasPortParam && !autoStartServer)
        LogWarning("TundraLogicModule::ReadStartupParameters: --port parameter given, but --server parameter is not present. Server will not be started.");

    // Write default values to config if not present.
    ConfigData configData(ConfigAPI::FILE_FRAMEWORK, ConfigAPI::SECTION_SERVER, "port", cDefaultPort, cDefaultPort);
    if (!framework->Config()->HasKey(configData))
        framework->Config()->Write(configData);

    if (autoStartServer)
    {
        // Use parameter port or default to config value
        const StringVector portParam = framework->CommandLineParameters("--port");
        if (hasPortParam && portParam.Empty())
        {
            LogWarning("TundraLogicModule::ReadStartupParameters: --port parameter given without value. Using the default from config.");
            autoStartServerPort = (unsigned short)framework->Config()->Read(configData).GetInt();
        }
        else if (!portParam.Empty())
        {
            autoStartServerPort = (unsigned short)Urho3D::ToInt(portParam.Front());
            if (!autoStartServerPort)
            {
                LogError("TundraLogicModule::ReadStartupParameters: --port parameter is not a valid nonzero value.");
                GetFramework()->Exit();
            }
        }
    }

    /// @todo Move --netRate handling to SyncManager.
    const bool hasNetRate = framework->HasCommandLineParameter("--netrate");
    const StringVector rateParam = framework->CommandLineParameters("--netrate");
    if (hasNetRate && rateParam.Empty())
        LogWarning("TundraLogicModule::ReadStartupParameters: --netrate parameter given without value.");
    if (!rateParam.Empty())
    {
        int rate = Urho3D::ToInt(rateParam.Front());
        if (rate > 0)
            syncManager_->SetUpdatePeriod(1.f / (float)rate);
        else
            LogError("TundraLogicModule::ReadStartupParameters: --netrate parameter is not a valid nonzero value.");
    }

    if (autoStartServer)
        server_->Start(autoStartServerPort); 
    if (framework->HasCommandLineParameter("--file")) // Load startup scene here (if we have one)
        LoadStartupScene();

    StringVector connectArgs = framework->CommandLineParameters("--connect");
    if (connectArgs.Size() > 1) /**< @todo If/when multi-connection support is on place, this should be changed! */
        LogWarning("TundraLogicModule::ReadStartupParameters: multiple --connect parameters given, ignoring all of them!");
    if (connectArgs.Size() == 1)
    {
        StringVector params = connectArgs.Front().Split(';');
        if (params.Size() >= 4)
            client_->Login(/*addr*/params[0], /*port*/(unsigned short)Urho3D::ToInt(params[1]), /*username*/params[3],
            /*optional passwd*/ params.Size() >= 5 ? params[4] : "", /*protocol*/params[2]);
        else
            LogError("TundraLogicModule::ReadStartupParameters: Not enought parameters for --connect. Usage '--connect serverIp;port;protocol;name;password'. Password is optional.");
    }
}

void TundraLogic::LoadStartupScene()
{
    StringVector files = framework->CommandLineParameters("--file");
    if (files.Empty())
        LogError("TundraLogicModule: --file specified without a value.");

    foreach(const String &file, files)
    {
        AssetAPI::AssetRefType refType = AssetAPI::ParseAssetRef(file.Trimmed());
        if (refType != AssetAPI::AssetRefExternalUrl)
            LoadScene(file, false, false);
        else
        {
            AssetTransferPtr transfer = framework->Asset()->RequestAsset(file, "Binary", true);
            if (transfer)
                transfer->Succeeded.Connect(this, &TundraLogic::StartupSceneLoaded);
        }
    }
}

void TundraLogic::StartupSceneLoaded(AssetPtr sceneAsset)
{
    LoadScene(sceneAsset->DiskSource(), false, false);
    sceneAsset->Unload();
}

bool TundraLogic::LoadScene(String filename, bool clearScene, bool useEntityIDsFromFile)
{
    filename = filename.Trimmed();
    if (filename.Empty())
    {
        LogError("TundraLogicModule::LoadScene: Empty filename given!");
        return false;
    }

    filename = framework->LookupRelativePath(filename);

    // If a scene does not exist yet for loading, create it now
    Scene *scene = framework->Scene()->MainCameraScene();
    if (!scene)
        scene = framework->Scene()->CreateScene("TundraServer", true, true).Get();
    
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

SharedPtr<KristalliProtocol> TundraLogic::KristalliProtocol() const
{
    return kristalliProtocol_;
}

SharedPtr<SyncManager> TundraLogic::SyncManager() const
{
    return syncManager_;
}

ClientPtr TundraLogic::Client() const
{
    return client_;
}

ServerPtr TundraLogic::Server() const
{
    return server_;
}

void TundraLogic::ClientConnectedToServer(UserConnectedResponseData *responseData)
{
    if (responseData->responseDataXml)
    {
        Urho3D::XMLElement root = responseData->responseDataXml->GetRoot("asset");
        if (root)
        {
            Urho3D::XMLElement storage = root.GetChild("storage");
            while (storage)
            {
                String storageData = storage.GetAttribute("data");

                bool connectedToRemoteServer = true; // If false, we connected to localhost.
                ///\todo Determine here whether we connected to localhost, and if so, set connectedToRemoteServer = false.
                AssetStoragePtr assetStorage = Fw()->Asset()->DeserializeAssetStorageFromString(storageData, connectedToRemoteServer);

                // Remember that this storage was received from the server, so we can later stop using it when we disconnect (and possibly reconnect to another server).
                if (assetStorage)
                {
                    assetStorage->SetReplicated(true); // We got this from the server.
                    if (connectedToRemoteServer) // If connected to localhost, we always trust the same storages the server is trusting, so don't need to call DetermineStorageTrustStatus.
                        DetermineStorageTrustStatus(assetStorage);
                    storagesReceivedFromServer.Push(AssetStorageWeakPtr(assetStorage));
                }
                
                storage = storage.GetNext("storage");
            }

            Urho3D::XMLElement defaultStorage = root.GetChild("defaultStorage");
            if (defaultStorage)
            {
                String defaultStorageName = defaultStorage.GetAttribute("name");
                AssetStoragePtr defaultStoragePtr = Fw()->Asset()->AssetStorageByName(defaultStorageName);
                if (defaultStoragePtr)
                    framework->Asset()->SetDefaultAssetStorage(defaultStoragePtr);
            }
        }
    }
}

void TundraLogic::ClientDisconnectedFromServer()
{
    for(size_t i = 0; i < storagesReceivedFromServer.Size(); ++i)
    {
        if (storagesReceivedFromServer[i])
            framework->Asset()->RemoveAssetStorage(storagesReceivedFromServer[i]->Name());
    }
    storagesReceivedFromServer.Clear();
}

void TundraLogic::DetermineStorageTrustStatus(AssetStoragePtr storage)
{
    // If the --trustserverstorages command line parameter is set, we trust each storage exactly the way the server does.
    ///\todo Make the a end-user option at runtime/connection time to specify per-server instance whether --trustserverstorages is in effect.
    if (!framework->HasCommandLineParameter("--trustserverstorages"))
    {
        ///\todo Read from ConfigAPI whether to set false/ask/true here.
        ///\todo If the trust state is 'ask', show a *non-modal* notification -> config dialog if the user wants to trust content from this source.
        storage->SetTrustState(IAssetStorage::StorageAskTrust);
    }
}

extern "C"
{

DLLEXPORT void TundraPluginMain(Tundra::Framework *fw)
{
    fw->RegisterModule(new Tundra::TundraLogic(fw));
}

}

}

