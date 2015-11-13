// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

#include "DebugAPI.h"
#include "DebugHud.h"
#include "Framework.h"
#include "ConsoleAPI.h"
#include "FrameAPI.h"

#include <Urho3D/Input/InputEvents.h>

namespace Tundra
{

DebugAPI::DebugAPI(Framework *framework) :
    Object(framework->GetContext()),
    framework_(framework)
{
    framework_->Console()->RegisterCommand("prof", "Toggle profiling HUD.", this, &DebugAPI::ToggleDebugHudVisibility);

    SubscribeToEvent(Urho3D::E_KEYDOWN, URHO3D_HANDLER(DebugAPI, OnKeyDown));
}

DebugAPI::~DebugAPI()
{
}

void DebugAPI::OnKeyDown(StringHash eventType, VariantMap &eventData)
{
    using namespace Urho3D::KeyDown;
    if (eventType == Urho3D::E_KEYDOWN)
    {
        if (eventData[P_QUALIFIERS].GetInt() == Urho3D::QUAL_SHIFT &&
            eventData[P_KEY].GetInt() == Urho3D::KEY_P && 
            !eventData[P_REPEAT].GetBool())
        {
            ToggleDebugHudVisibility();
        }
    }
}

DebugHud *DebugAPI::Hud()
{
    if (!debugHud_)
        debugHud_ = new DebugHud(framework_);
    return debugHud_;
}

void DebugAPI::ToggleDebugHudVisibility()
{
    Hud()->ToggleVisibility();
}

}
