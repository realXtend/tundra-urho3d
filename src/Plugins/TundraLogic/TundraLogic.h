// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IModule.h"
#include "TundraLogicApi.h"
#include "TundraLogicFwd.h"
#include "AssetFwd.h"
#include "Signals.h"

namespace Tundra
{

typedef SharedPtr<Tundra::Client> ClientPtr;
typedef SharedPtr<Tundra::Server> ServerPtr;

/// Top-level scene and network protocol handling logic.
class TUNDRALOGIC_API TundraLogic : public IModule
{
    URHO3D_OBJECT(TundraLogic, IModule);

public:
    TundraLogic(Framework* owner);
    ~TundraLogic();

    /// Returns whether a server is running
    bool IsServer() const;

    /// Returns pointer to KristalliProtocolModule
    SharedPtr<Tundra::KristalliProtocol> KristalliProtocol() const;

    /// Returns pointer to sync manager
    SharedPtr<Tundra::SyncManager> SyncManager() const;

    /// Returns client pointer
    ClientPtr Client() const;

    /// Returns server pointer
    ServerPtr Server() const;

    /// For console command
    void HandleLogin(const StringVector &params) const;

private:
    void Load() override;
    void Initialize() override;
    void Uninitialize() override;

    /// Frame update handler
    void Update(float frametime) override;

    /// Handle delayed signal to parse command line parameters.
    void ReadStartupParameters(float time);

    /// Load startup scene if necessary.
    void LoadStartupScene();

    /// Load a scene file into the active (main camera) scene. Return true on success.
    bool LoadScene(String filename, bool clearScene, bool useEntityIDsFromFile);

    /// Handle startup scene asset being loaded
    void StartupSceneLoaded(AssetPtr sceneAsset);

    /// Handle client connection to server. Add asset storages advertised by the server.
    void ClientConnectedToServer(UserConnectedResponseData *responseData);

    /// Handle client disconnection from server. Remove asset storages advertised by the server.
    void ClientDisconnectedFromServer();

    /// Whenever we receive a new asset storage from the server, this function is called to determine if the storage is to be trusted.
    void DetermineStorageTrustStatus(AssetStoragePtr storage);

    /// Handle new connection to server. Populate local asset storages if connected from the same machine to the login response data.
    void ServerNewUserConnected(u32 connectionID, UserConnection *connection, UserConnectedResponseData *responseData);
    /// The sync manager
    SharedPtr<Tundra::SyncManager> syncManager_;
    /// The client
    ClientPtr client_;
    // The server
    ServerPtr server_;
    /// The kristalli protocol
    SharedPtr<Tundra::KristalliProtocol> kristalliProtocol_;
    /// Asset storages received from the server upon connecting
    Vector<AssetStorageWeakPtr> storagesReceivedFromServer;
};

}
