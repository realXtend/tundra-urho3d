// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "UrhoRenderer.h"
#include "Framework.h"

#include <Log.h>

namespace Tundra
{

UrhoRenderer::UrhoRenderer(Framework* owner) :
    IModule("UrhoRenderer", owner)
{
}

UrhoRenderer::~UrhoRenderer()
{
}

void UrhoRenderer::Load()
{
    /// \todo Register component & asset factories
    LOGINFO("Loaded UrhoRenderer");
}

void UrhoRenderer::Initialize()
{
}

void UrhoRenderer::Uninitialize()
{
}

}

extern "C"
{

DLLEXPORT void TundraPluginMain(Tundra::Framework *fw)
{
    fw->RegisterModule(new Tundra::UrhoRenderer(fw));
}

}
