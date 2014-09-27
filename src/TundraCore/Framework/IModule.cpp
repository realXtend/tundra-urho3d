// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "IModule.h"
#include "Framework.h"

namespace Tundra
{

IModule::IModule(const Urho3D::String &moduleName, Framework* owner) :
    Object(framework->GetContext()),
    name(moduleName),
    framework(owner)
{
}

}
