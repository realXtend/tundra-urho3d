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

#include "TundraCoreApi.h"
#include "CoreTypes.h"

#include <Timer.h>

namespace Urho3D
{
    class Engine;
    class Font;
    class Text;
    class BorderImage;
    class Button;
    class UIElement;
    class XMLFile;
}

namespace Tundra
{

typedef SharedPtr<Urho3D::UIElement> UIElementPtr;
typedef SharedPtr<Urho3D::Text> TextPtr;
typedef SharedPtr<Urho3D::Button> ButtonPtr;
typedef SharedPtr<Urho3D::BorderImage> BorderImagePtr;
typedef Vector<ButtonPtr > ButtonPtrVector;
typedef Vector<BorderImagePtr > BorderImagePtrVector;
typedef Pair<String, UIElementPtr > WidgetPair;
typedef Vector<WidgetPair > WidgetPairVector;

static const unsigned DEBUGHUD_SHOW_NONE = 0x0;
static const unsigned DEBUGHUD_SHOW_STATS = 0x1;
static const unsigned DEBUGHUD_SHOW_MODE = 0x2;
static const unsigned DEBUGHUD_SHOW_PANEL = 0x4;
static const unsigned DEBUGHUD_SHOW_ALL = 0x7;

/// Displays rendering stats and profiling information.
class TUNDRACORE_API DebugHud : public Object
{
    OBJECT(DebugHud);

public:
    /// Construct.
    DebugHud(Urho3D::Context* context);
    /// Destruct.
    ~DebugHud();

    /// Toggle elements.
    void Toggle(unsigned mode);
    /// Toggle visibility.
    void ToggleVisibility();
    /// Sets visivility.
    void SetVisible(bool visible);

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
    unsigned GetProfilerMaxDepth() const { return profilerMaxDepth_; }
    /// Return profiler accumulation interval in seconds
    float GetProfilerInterval() const;

    /// Set UI elements' style from an XML file.
    void SetDefaultStyle(Urho3D::XMLFile* style);

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

    /// Update. Called by HandlePostUpdate().
    void Update();
    /// Return the UI style file.
    Urho3D::XMLFile* GetDefaultStyle() const;
    /// Return rendering stats text.
    Urho3D::Text* GetStatsText() const { return statsText_; }
    /// Return rendering mode text.
    Urho3D::Text* GetModeText() const { return modeText_; }
    /// Return profiler text.
    Urho3D::Text* GetProfilerText() const { return profilerText_; }

    void CreateTab(const String &name, UIElementPtr widget);
    BorderImagePtr CreateContainer(int ha, int va, int priority);

    /// Rendering stats text.
    TextPtr statsText_;
    /// Rendering mode text.
    TextPtr modeText_;
    /// Profiling information text.
    TextPtr profilerText_;
    /// Tab container.
    BorderImagePtr tabContainer_;
    /// Text widget containers.
    BorderImagePtrVector containers_;
    /// Tab buttons.
    ButtonPtrVector buttons_;
    /// Tab layout.
    UIElementPtr tabLayout_;
    /// Tab widgets
    WidgetPairVector widgets_;

    /// Hashmap containing application specific stats.
    HashMap<String, String> appStats_;
    /// Profiler timer.
    Urho3D::Timer profilerTimer_;
    /// Profiler max block depth.
    unsigned profilerMaxDepth_;
    /// Profiler accumulation interval.
    unsigned profilerInterval_;
    /// Show 3D geometry primitive/batch count flag.
    bool useRendererStats_;
    /// Current shown-element mode.
    unsigned mode_;
};

}
