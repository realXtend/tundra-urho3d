/// Class copied from the Urho3D project. Modified for Tundra use.

//
// Copyright (c) 2008-2014 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include "FrameworkFwd.h"
#include "TundraCoreApi.h"
#include "CoreTypes.h"

#include "DebugHudPanel.h"

#include <Timer.h>

namespace Urho3D
{
    class Text;
    class BorderImage;
    class Button;
    class UIElement;
}

namespace Tundra
{

typedef SharedPtr<Urho3D::UIElement> UIElementPtr;
typedef WeakPtr<Urho3D::UIElement> UIElementWeakPtr;
typedef SharedPtr<Urho3D::Text> TextPtr;
typedef SharedPtr<Urho3D::Button> ButtonPtr;
typedef SharedPtr<Urho3D::BorderImage> BorderImagePtr;
typedef Vector<ButtonPtr > ButtonPtrVector;
typedef Vector<BorderImagePtr > BorderImagePtrVector;

static const unsigned DEBUGHUD_SHOW_NONE  = 0x0;
static const unsigned DEBUGHUD_SHOW_STATS = 0x1;
static const unsigned DEBUGHUD_SHOW_MODE  = 0x2;
static const unsigned DEBUGHUD_SHOW_PANEL = 0x4;
static const unsigned DEBUGHUD_SHOW_ALL   = 0x7;

class ProfilerHudPanel;

/// Displays rendering stats and profiling information.
class TUNDRACORE_API DebugHud : public Object
{
    OBJECT(DebugHud);

public:
    /// Construct.
    DebugHud(Framework *framework);
    /// Destruct.
    ~DebugHud();

    /// Toggle elements.
    void Toggle(unsigned mode);
    /// Toggle visibility.
    void ToggleVisibility();
    /// Sets visivility.
    void SetVisible(bool visible);

    /// Add a new tab to profiler.
    bool AddTab(const String &name, DebugHudPanelWeakPtr updater);
    /// Removes tab @c name.
    void RemoveTab(const String &name);
    /// Show tab @c name.
    void ShowTab(const String &name);
    /// Returns available tab names.
    StringVector TabNames() const;

    /// Set elements to show.
    void SetMode(unsigned mode);
    /// Set maximum profiler block depth, default unlimited.
    void SetProfilerMaxDepth(unsigned depth);
    /// Set profiler accumulation interval in seconds.
    void SetProfilerInterval(float interval);
    /// Set whether to show 3D geometry primitive/batch count only. Default false.
    void SetUseRendererStats(bool enable);

    /// Return currently shown elements.
    unsigned GetMode() const { return mode_; }
    /// Return maximum profiler block depth.
    unsigned GetProfilerMaxDepth() const;
    /// Return profiler accumulation interval in seconds
    float GetProfilerInterval() const;

    /// Return whether showing 3D geometry primitive/batch count only.
    bool GetUseRendererStats() const { return useRendererStats_; }
    /// Set application-specific stats.
    void SetAppStats(const String& label, const Variant& stats);
    /// Set application-specific stats.
    void SetAppStats(const String& label, const String& stats);
    /// Reset application-specific stats. Return true if it was erased successfully.
    bool ResetAppStats(const String& label);
    /// Clear all application-specific stats.
    void ClearAppStats();

private:
    /// Handle logic post-update event. The HUD texts are updated here.
    void HandlePostUpdate(StringHash eventType, VariantMap& eventData);
    /// Handle window size change.
    void HandleWindowChange(StringHash eventType, VariantMap& eventData);
    /// Handle tab change from button click.
    void HandleTabChange(StringHash eventType, VariantMap& eventData);
    /// Update.
    void OnUpdate(float frametime);

    struct HudTab
    {
        String name;
        DebugHudPanelWeakPtr updater;
    };

    BorderImagePtr CreateContainer(int ha, int va, int priority);

    /// Rendering stats text.
    TextPtr statsText_;
    /// Rendering mode text.
    TextPtr modeText_;

    /// Tab container.
    BorderImagePtr tabContainer_;
    /// Text widget containers.
    BorderImagePtrVector containers_;
    /// Tab buttonlayout.
    UIElementPtr tabButtonLayout_;
    /// Tab widget data.
    Vector<HudTab*> tabs_;
    /// Current tab
    HudTab *currentTab_;

    /// Profiler panel implementation
    SharedPtr<ProfilerHudPanel> profilerHudPanel_;

    /// Hashmap containing application specific stats.
    HashMap<String, String> appStats_;

    /// Show 3D geometry primitive/batch count flag.
    bool useRendererStats_;
    /// Current shown-element mode.
    unsigned mode_;

    Framework *framework_;
};

/// @cond PRIVATE
class ProfilerHudPanel : public DebugHudPanel
{
public:
    ProfilerHudPanel(Framework *framework);

    /// DebugHudPanel override.
    void UpdatePanel(float frametime, const UIElementPtr &widget) override;

    /// Profiler max block depth.
    unsigned profilerMaxDepth;
    /// Profiler accumulation interval.
    unsigned profilerInterval;

protected:
    /// DebugHudPanel override.
    UIElementPtr CreateImpl() override;
private:
    /// Profiler timer.
    Urho3D::Timer profilerTimer_;
};
/// @endcond

}
