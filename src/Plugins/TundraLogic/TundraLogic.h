// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IModule.h"
#include "TundraLogicFwd.h"
#include "TundraLogicApi.h"
#include "Signals.h"
#include "Server.h"
#include "Client.h"

namespace Tundra
{


/// Top-level scene and network protocol handling logic.
class TUNDRALOGIC_API TundraLogic : public IModule
{
    OBJECT(TundraLogic);

public:
    TundraLogic(Framework* owner);
    ~TundraLogic();

    /// \todo hardcoded because no server functionality implemented
    bool IsServer() { return false; }

    /// Returns pointer to KristalliProtocolModule for convenience
    KristalliProtocol *GetKristalliModule() const { return kristalliProtocol_; }

    SyncManager *SyncManager() const { return syncManager_; }

    /// Returns client pointer
    SharedPtr<Client> Client() const { return client_; }
    /// Returns server pointer
    SharedPtr<Server> GetServer() const { return server_; }

private:
    void Load() override;
    void Initialize() override;
    void Uninitialize() override;

    /// Frame update handler
    void OnUpdate(float frametime);

    /// Load startup scene if necessary. Return true on success.
    void LoadStartupScene();

    /// Load a scene file into the active (main camera) scene. Return true on success.
    bool LoadScene(String filename, bool clearScene, bool useEntityIDsFromFile);

    bool startupSceneLoaded;

    /// The sync manager
    SharedPtr<Tundra::SyncManager> syncManager_;
    /// The client
    SharedPtr<Tundra::Client> client_;
    // The server
    SharedPtr<Tundra::Server> server_;
    /// The kristalli protocol
 //   KristalliProtocol *kristalliModule_;
    SharedPtr<Tundra::KristalliProtocol> kristalliProtocol_;
};

}
