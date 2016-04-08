// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IModule.h"
#include "SceneFwd.h"
#include "IAttribute.h"
#include "InputFwd.h"
#include "Math/float3.h"

namespace Tundra
{

class JavaScriptInstance;

/// Avatar component and avatar description asset functionality. */
class AvatarApplication : public IModule
{
    URHO3D_OBJECT(AvatarApplication, IModule);

public:
    AvatarApplication(Framework* owner);
    ~AvatarApplication();

private:
    void Load() override;
    void Initialize() override;
    void Uninitialize() override;

    /// Handles script engine creation (register AvatarApplication classes)
    void OnScriptInstanceCreated(JavaScriptInstance* instance);
};

}
