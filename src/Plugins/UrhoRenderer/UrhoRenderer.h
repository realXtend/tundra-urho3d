// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IModule.h"
#include "UrhoModuleApi.h"

namespace Tundra
{

/// A renderer module using Urho3D
class URHO_MODULE_API UrhoRenderer : public IModule
{
    OBJECT(UrhoRenderer);

public:
    UrhoRenderer(Framework* owner);
    ~UrhoRenderer();

private:
    void Load() override;
    void Initialize() override;
    void Uninitialize() override;
};

}
