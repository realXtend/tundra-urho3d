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

#include "StableHeaders.h"
#include "DebugHud.h"
#include "CoreDebugHuds.h"

#include "Framework.h"
#include "FrameAPI.h"
#include "CoreStringUtils.h"
#include "LoggingFunctions.h"

#include <CoreEvents.h>
#include <ResourceCache.h>
#include <GraphicsEvents.h>
#include <UIEvents.h>

#include <Context.h>
#include <Engine.h>
#include <Font.h>
#include <Graphics.h>
#include <Profiler.h>
#include <Renderer.h>
#include <Text.h>
#include <BorderImage.h>
#include <UI.h>
#include <Button.h>
#include <ScrollView.h>

#include <DebugNew.h>

using namespace Urho3D;

namespace Tundra
{

static const char* qualityTexts[] =
{
    "Low",
    "Med",
    "High"
};

static const char* shadowQualityTexts[] =
{
    "16bit Low",
    "24bit Low",
    "16bit High",
    "24bit High"
};

DebugHud::DebugHud(Framework *framework) :
    Object(framework->GetContext()),
    framework_(framework),
    useRendererStats_(false),
    mode_(DEBUGHUD_SHOW_NONE),
    currentTab_(0)
{
    profilerHudPanel_ = new ProfilerHudPanel(framework_);
    sceneHudPanel_ = new SceneHudPanel(framework_);
    assetHudPanel_ = new AssetHudPanel(framework_);

    XMLFile *style = context_->GetSubsystem<ResourceCache>()->GetResource<XMLFile>("UI/DefaultStyle.xml");

    SharedPtr<BorderImage> container = CreateContainer(HA_LEFT, VA_TOP, 100);
    statsText_ = new Text(context_);
    statsText_->SetStyle("TextMonospaceShadowed", style);
    container->AddChild(statsText_);

    container = CreateContainer(HA_LEFT, VA_BOTTOM, 100);
    modeText_ = new Text(context_);
    modeText_->SetStyle("TextMonospaceShadowed", style);
    container->AddChild(modeText_);

    // Add core debug panels
    AddTab("Profiler", StaticCast<DebugHudPanel>(profilerHudPanel_));
    AddTab("Scene", sceneHudPanel_);
    AddTab("Assets", assetHudPanel_);

    framework_->Frame()->PostFrameUpdate.Connect(this, &DebugHud::OnUpdate);
    SubscribeToEvent(E_SCREENMODE, HANDLER(DebugHud, HandleWindowChange));
}

DebugHud::~DebugHud()
{
    profilerHudPanel_.Reset();

    for(auto iter = tabs_.Begin(); iter != tabs_.End(); ++iter)
        delete (*iter);
    tabs_.Clear();
    currentTab_ = 0;

    foreach(SharedPtr<BorderImage> c, containers_)
        c->Remove();
    containers_.Clear();
}

bool DebugHud::AddTab(const String &name, DebugHudPanelWeakPtr updater)
{
    foreach(auto tab, tabs_)
    {
        if (tab->name.Compare(name, true) == 0)
        {
            LogErrorF("DebugHud::AddTab: Tab '%s' already exists.", name.CString());
            return false;
        }
    }

    // Create widget and tab
    updater->Create();
    UIElementPtr widget = updater->Widget();
    if (!widget)
    {
        LogErrorF("DebugHud::AddTab: Tab '%s' failed to create a widget for embedding.", name.CString());
        return false;
    }
    HudTab *tab = new HudTab();
    tab->name = name;
    tab->updater = updater;
    tabs_.Push(tab);

    // Get default style
    XMLFile *style = context_->GetSubsystem<ResourceCache>()->GetResource<XMLFile>("UI/DefaultStyle.xml");

    // Initialize container
    if (!tabContainer_)
    {
        tabContainer_ = CreateContainer(HA_RIGHT, VA_TOP, 100);
        Graphics *graphics = GetSubsystem<Graphics>();
        if (graphics)
        {
            tabContainer_->SetFixedHeight(graphics->GetHeight());
            tabContainer_->SetFixedWidth(Urho3D::Clamp(static_cast<int>(graphics->GetWidth() * 0.4), 500, 900));
        }
    }
    // Initialize button layout
    if (!tabButtonLayout_)
    {
        tabButtonLayout_ = new UIElement(context_);
        tabButtonLayout_->SetLayout(LM_HORIZONTAL, 15);
        tabButtonLayout_->SetFixedHeight(35);
        auto border = tabButtonLayout_->GetLayoutBorder();
        border.top_ = 8; border.bottom_ = 8;
        tabButtonLayout_->SetLayoutBorder(border);
        tabContainer_->AddChild(tabButtonLayout_);
    }

    // Create button to invoke the panel
    Button *button = tabButtonLayout_->CreateChild<Button>("button" + name, M_MAX_UNSIGNED);
    Text *text = button->CreateChild<Text>("buttonText" + name, 0);
    text->SetInternal(true);
    text->SetText(name);
    button->SetName(name);
    button->SetStyle("Button", style);

    // Sub to click release event
    SubscribeToEvent(button, E_RELEASED, HANDLER(DebugHud, HandleTabChange));

    bool isFirstChild = (tabContainer_->GetNumChildren() == 1);
    ScrollView *panel = tabContainer_->CreateChild<ScrollView>("scrollView" + name, Urho3D::M_MAX_UNSIGNED);

    /// Auto set style if a text widget to keep things looking consistent.
    Text *textWidget = dynamic_cast<Text*>(widget.Get());
    if (textWidget)
        textWidget->SetStyle("TextMonospaceShadowed", style);
    widget->SetVisible(isFirstChild);

    // Embed widget to tab panel
    panel->SetContentElement(widget);
    panel->SetVisible(isFirstChild);
    panel->SetStyle("OverlayScrollView", style);
    tabContainer_->AddChild(panel);

    // If first tab panel set it as current
    if (isFirstChild && !currentTab_)
    {
        button->SetStyle("ButtonSelected");
        currentTab_ = tab;
    }
    return true;
}

BorderImagePtr DebugHud::CreateContainer(int ha, int va, int priority)
{
    XMLFile *style = context_->GetSubsystem<ResourceCache>()->GetResource<XMLFile>("UI/DefaultStyle.xml");

    SharedPtr<BorderImage> container(new BorderImage(context_));
    container->SetAlignment((HorizontalAlignment)ha, (VerticalAlignment)va);
    container->SetPriority(priority);
    container->SetVisible(false);
    container->SetStyle("VOverlayTransparent", style);

    GetSubsystem<UI>()->GetRoot()->AddChild(container);
    
    containers_.Push(container);
    return container;
}

void DebugHud::OnUpdate(float frametime)
{
    if (mode_ == DEBUGHUD_SHOW_NONE)
        return;

    Graphics* graphics = GetSubsystem<Graphics>();
    Renderer* renderer = GetSubsystem<Renderer>();
    if (!renderer || !graphics)
        return;

    if (statsText_->IsVisible())
    {
        unsigned primitives, batches;
        if (!useRendererStats_)
        {
            primitives = graphics->GetNumPrimitives();
            batches = graphics->GetNumBatches();
        }
        else
        {
            primitives = renderer->GetNumPrimitives();
            batches = renderer->GetNumBatches();
        }

        String stats;
        stats.AppendWithFormat("FPS        %d\nTriangles  %s\nBatches    %u\nViews      %u\nLights     %u\nShadowmaps %u\nOccluders  %u",
            static_cast<int>(1.f / GetSubsystem<Time>()->GetTimeStep()),
            FormatDigitGrouping(primitives).CString(),
            batches,
            renderer->GetNumViews(),
            renderer->GetNumLights(true),
            renderer->GetNumShadowMaps(true),
            renderer->GetNumOccluders(true));

        if (!appStats_.Empty())
        {
            stats.Append("\n");
            for (HashMap<String, String>::ConstIterator i = appStats_.Begin(); i != appStats_.End(); ++i)
                stats.AppendWithFormat("\n%s %s", i->first_.CString(), i->second_.CString());
        }

        statsText_->SetText(stats);
    }

    if (modeText_->IsVisible())
    {
        String mode;
        mode.AppendWithFormat("Tex:%s Mat:%s Spec:%s Shadows:%s Size:%i Quality:%s Occlusion:%s Instancing:%s Mode:%s",
            qualityTexts[renderer->GetTextureQuality()],
            qualityTexts[renderer->GetMaterialQuality()],
            renderer->GetSpecularLighting() ? "On" : "Off",
            renderer->GetDrawShadows() ? "On" : "Off",
            renderer->GetShadowMapSize(),
            shadowQualityTexts[renderer->GetShadowQuality()],
            renderer->GetMaxOccluderTriangles() > 0 ? "On" : "Off",
            renderer->GetDynamicInstancing() ? "On" : "Off",
            #ifdef URHO3D_OPENGL
            "OGL");
            #else
            graphics->GetSM3Support() ? "SM3" : "SM2");
            #endif

        modeText_->SetText(mode);
    }

    if (tabContainer_->IsVisible() && currentTab_)
    {
        SharedPtr<DebugHudPanel> panel = currentTab_->updater.Lock();
        UIElementPtr widget = (panel.Get() ? panel->Widget() : UIElementPtr());
        if (panel && widget)
            panel->UpdatePanel(frametime, widget);
    }
}

void DebugHud::SetMode(unsigned mode)
{
    statsText_->GetParent()->SetVisible((mode & DEBUGHUD_SHOW_STATS) != 0);
    modeText_->GetParent()->SetVisible((mode & DEBUGHUD_SHOW_MODE) != 0);
    tabContainer_->SetVisible((mode & DEBUGHUD_SHOW_PANEL) != 0);

    mode_ = mode;
}

void DebugHud::SetProfilerMaxDepth(unsigned depth)
{
    profilerHudPanel_->profilerMaxDepth = depth;
}

void DebugHud::SetProfilerInterval(float interval)
{
    profilerHudPanel_->profilerInterval = Max((int)(interval * 1000.0f), 0);
}

void DebugHud::SetUseRendererStats(bool enable)
{
    useRendererStats_ = enable;
}

void DebugHud::Toggle(unsigned mode)
{
    SetMode(GetMode() ^ mode);
}

void DebugHud::ToggleVisibility()
{
    Toggle(DEBUGHUD_SHOW_ALL);
}

void DebugHud::SetVisible(bool visible)
{
    SetMode(visible ? DEBUGHUD_SHOW_ALL : DEBUGHUD_SHOW_NONE);
}

unsigned DebugHud::GetProfilerMaxDepth() const
{
    return profilerHudPanel_->profilerMaxDepth;
}

float DebugHud::GetProfilerInterval() const
{
    return (float)profilerHudPanel_->profilerInterval / 1000.0f;
}

void DebugHud::SetAppStats(const String& label, const Variant& stats)
{
    SetAppStats(label, stats.ToString());
}

void DebugHud::SetAppStats(const String& label, const String& stats)
{
    bool newLabel = !appStats_.Contains(label);
    appStats_[label] = stats;
    if (newLabel)
        appStats_.Sort();
}

bool DebugHud::ResetAppStats(const String& label)
{
    return appStats_.Erase(label);
}

void DebugHud::ClearAppStats()
{
    appStats_.Clear();
}

void DebugHud::HandleWindowChange(StringHash /*eventType*/, VariantMap& eventData)
{
    using namespace ScreenMode;

    if (tabContainer_)
    {
        tabContainer_->SetFixedHeight(eventData[P_HEIGHT].GetInt());
        tabContainer_->SetFixedWidth(Urho3D::Clamp(static_cast<int>(eventData[P_WIDTH].GetInt() * 0.4), 500, 900));
    }
}

void DebugHud::HandleTabChange(StringHash /*eventType*/, VariantMap& eventData)
{
    using namespace Released;

    Button *button = dynamic_cast<Button*>(eventData[P_ELEMENT].GetPtr());
    if (!button)
        return;

    if (!ShowTab(button->GetName()))
        return;

    Vector<SharedPtr<UIElement> > children = tabButtonLayout_->GetChildren();
    foreach(auto child, children)
    {
        Button *b = dynamic_cast<Button*>(child.Get());
        if (b)
            b->SetStyle(b == button ? "ButtonSelected" : "Button");
    }
}

bool DebugHud::RemoveTab(const String &name)
{
    for (auto iter = tabs_.Begin(); iter != tabs_.End(); ++iter)
    {
        HudTab *tab = (*iter);
        if (tab->name.Compare(name, true))
        {
            tabs_.Erase(iter);
            if (currentTab_ == tab)
            {
                currentTab_ = 0;
                if (tabs_.Begin() != tabs_.End())
                    ShowTab(tabs_.Front()->name);
            }
            if (tab->updater.Get())
                tab->updater->Destroy();
            tab->updater.Reset();
            delete tab;
            return true;
        }
    }
    LogErrorF("DebugHud::ShowTab: Tab '%s' does not exit.", name.CString());
    return false;
}

bool DebugHud::ShowTab(const String &name)
{
    if (name.Empty())
    {
        LogError("DebugHud::ShowTab: Provided tab name is empty");
        return false;
    }

    UIElementPtr widget;
    foreach(auto tab, tabs_)
    {
        if (tab->name.Compare(name, true) == 0 && tab->updater->Widget())
        {
            widget = tab->updater->Widget();
            currentTab_ = tab;
            break;
        }
    }
    if (!widget)
    {
        LogErrorF("DebugHud::ShowTab: Tab '%s' does not exit.", name.CString());
        return false;
    }
    foreach(auto tab, tabs_)
    {
        // Tab widgets are always parented to a ScrollView. Toggle its visibility.
        UIElementPtr currentWidget = tab->updater->Widget();
        bool visible = (currentWidget.Get() == widget.Get());
        UIElement *parent = currentWidget->GetParent();
        if (parent)
        {
            ScrollView *view = dynamic_cast<ScrollView*>(parent->GetParent() ? parent->GetParent() : parent);
            if (view)
                view->SetVisible(visible);
            else
                parent->SetVisible(visible);
        }
        currentWidget->SetVisible(visible);
    }
    return true;
}

StringVector DebugHud::TabNames() const
{
    StringVector names;
    foreach(auto tab, tabs_)
        names.Push(tab->name);
    return names;
}

}
