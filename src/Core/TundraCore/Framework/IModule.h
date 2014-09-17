// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"
#include "CoreTypes.h"
#include "CoreDefines.h"
#include "FrameworkFwd.h"

#include <Object.h>

namespace Tundra
{

/// Interface for modules. When creating new modules, inherit from this class.
class TUNDRACORE_API IModule : public Urho3D::Object
{
    OBJECT(IModule);

public:
    /// Constructor.
    /** @param moduleName Module name. */
    explicit IModule(const Urho3D::String &moduleName, Framework* framework);
    virtual ~IModule();

    /// Called when module is loaded into memory.
    /** Override in your own module. Do not call.
        Components and asset types exposed by the module should be registered here using SceneAPI and AssetAPI respectively. */
    virtual void Load() {}

    /// Called when module is taken in use.
    /** Override in your own module. Do not call. */
    virtual void Initialize() {}

    /// Called when module is removed from use.
    /** Override in your own module. Do not call. */
    virtual void Uninitialize() {}

    /// Called when module is unloaded from memory.
    /** Override in your own module. Do not call. */
    virtual void Unload() {}

    /// Synchronized update for the module
    /** Override in your own module if you want to perform synchronized update. Do not call.
        @param frametime elapsed time in seconds since last frame */
    virtual void Update(f32 frametime);

    /// Returns the name of the module.
    const Urho3D::String &Name() const { return name; }

    /// Returns parent framework.
    Framework *GetFramework() const { return framework_; }

protected:
    Framework *framework_; ///< Parent framework

private:
    const Urho3D::String name; ///< Name of the module
};

}

