// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IModule.h"
#include "HttpPluginFwd.h"
#include "HttpPluginApi.h"

namespace Tundra
{

/// Provides HTTP client and server functionality
class TUNDRA_HTTP_API HttpPlugin : public IModule
{
    URHO3D_OBJECT(HttpPlugin, IModule);

public:
    HttpPlugin(Framework* owner);
    ~HttpPlugin();

private:
    void Load() override;
    void Initialize() override;
    void Uninitialize() override;
    void Update(float frametime) override;

    HttpClientPtr client_;
    HttpAssetProviderPtr provider_;
};

}
