/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   DebugAPI.h
    @brief  Debug core API. */

#pragma once

#include "TundraCoreApi.h"
#include "CoreTypes.h"
#include "FrameworkFwd.h"

#include <Urho3D/Core/Object.h>

namespace Tundra
{

class DebugHud;

/// Debug core API.
/** Exposes debug and profiling functionality for applications. */
class TUNDRACORE_API DebugAPI : public Object
{
    URHO3D_OBJECT(DebugAPI, Object);

public:
    /// Returns the debug hud.
    DebugHud *Hud();

    /// Toggles debug hud visibility.
    void ToggleDebugHudVisibility();

private:
    friend class Framework;

    /// Constructor. Framework takes ownership of this object.
    /** @param fw Framework */
    explicit DebugAPI(Framework *framework);
    ~DebugAPI();

    void OnKeyDown(StringHash eventType, VariantMap &eventData);

    Framework *framework_;
    SharedPtr<DebugHud> debugHud_;
};

}
