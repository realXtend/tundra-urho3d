// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IModule.h"
#include "TundraLogicApi.h"
#include "TundraLogicFwd.h"
#include "Signals.h"

namespace Tundra
{

typedef SharedPtr<Tundra::Client> ClientPtr;
typedef SharedPtr<Tundra::Server> ServerPtr;

/// Top-level scene and network protocol handling logic.
class TUNDRALOGIC_API TundraLogic : public IModule
{
    OBJECT(TundraLogic);

public:
    TundraLogic(Framework* owner);
    ~TundraLogic();

    /// \todo hardcoded because no server functionality implemented
    bool IsServer() { return false; }

    /// Returns pointer to KristalliProtocolModule
    SharedPtr<KristalliProtocol> KristalliProtocol() const;

    /// Returns pointer to sync manager
    SharedPtr<SyncManager> SyncManager() const;

    /// Returns client pointer
    ClientPtr Client() const;

    /// Returns server pointer
    ServerPtr Server() const;

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
    ClientPtr client_;
    // The server
    ServerPtr server_;
    /// The kristalli protocol
    SharedPtr<Tundra::KristalliProtocol> kristalliProtocol_;
};

}
