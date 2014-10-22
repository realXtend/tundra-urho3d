// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IModule.h"
#include "TundraLogicFwd.h"
#include "TundraLogicApi.h"
#include "Signals.h"

namespace Tundra
{

/// Top-level scene and network protocol handling logic.
class TUNDRALOGIC_API TundraLogic : public IModule
{
    OBJECT(TundraLogic);

public:
    TundraLogic(Framework* owner);
    ~TundraLogic();

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
};

}
