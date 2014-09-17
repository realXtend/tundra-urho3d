// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "UrhoRenderingModule.h"
#include "Framework.h"

#include <Log.h>

namespace Tundra
{

UrhoRenderingModule::UrhoRenderingModule(Framework* owner) :
    IModule("UrhoRendering", owner)
{
}

UrhoRenderingModule::~UrhoRenderingModule()
{
}

void UrhoRenderingModule::Load()
{
    /// \todo Register component & asset factories
    LOGINFO("Loaded UrhoRenderingModule");
}

void UrhoRenderingModule::Initialize()
{
}

void UrhoRenderingModule::Uninitialize()
{
}

}

extern "C"
{

DLLEXPORT void TundraPluginMain(Tundra::Framework *fw)
{
    fw->RegisterModule(new Tundra::UrhoRenderingModule(fw));
}

}
