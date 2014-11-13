// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"
#include "FrameworkFwd.h"
#include "CoreTypes.h"

#include <SharedPtr.h>
#include <RefCounted.h>
#include <UIElement.h>

namespace Tundra
{

/// Debug hud panel.
class TUNDRACORE_API DebugHudPanel : public RefCounted
{
public:
    DebugHudPanel(Framework *framework);
    virtual ~DebugHudPanel();

    /// Creates panel ui.
    void Create();

    /// Destroy panel ui.
    void Destroy();

    /// Returns widget UI element.
    SharedPtr<Urho3D::UIElement> Widget();

    /// Called when updaters tab @c widget is visible and needs to be updated.
    virtual void UpdatePanel(float frametime, const SharedPtr<Urho3D::UIElement> &widget) = 0;

protected:
    /// Create user interface element and return it.
    virtual SharedPtr<Urho3D::UIElement> CreateImpl() = 0;

    Framework *framework_;

private:
    SharedPtr<Urho3D::UIElement> widget_;
};
typedef WeakPtr<DebugHudPanel> DebugHudPanelWeakPtr;

}
