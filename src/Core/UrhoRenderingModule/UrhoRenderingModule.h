// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IModule.h"
#include "UrhoModuleApi.h"

namespace Tundra
{

/// A renderer module using Urho3D
class URHO_MODULE_API UrhoRenderingModule : public IModule
{
    OBJECT(UrhoRenderingModule);

public:
    UrhoRenderingModule(Framework* owner);
    virtual ~UrhoRenderingModule();

    virtual void Load();
    virtual void Initialize();
    virtual void Uninitialize();
};

}
