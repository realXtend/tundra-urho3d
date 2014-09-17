// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "IModule.h"
#include "Framework.h"

namespace Tundra
{

IModule::IModule(const Urho3D::String &moduleName, Framework* framework) :
    Object(framework->GetContext()),
    name(moduleName),
    framework_(framework)
{
}

IModule::~IModule()
{
}

void IModule::Update(f32 UNUSED_PARAM(frameTime))
{
}

}
