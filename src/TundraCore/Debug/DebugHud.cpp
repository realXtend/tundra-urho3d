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
#include "Framework.h"
#include "FrameAPI.h"
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
    profilerUpdater_ = new ProfilerHud(framework_);

    XMLFile *style = context_->GetSubsystem<ResourceCache>()->GetResource<XMLFile>("UI/DefaultStyle.xml");

    SharedPtr<BorderImage> container = CreateContainer(HA_LEFT, VA_TOP, 100);
    statsText_ = new Text(context_);
    statsText_->SetStyle("TextMonospaceShadowed", style);
    container->AddChild(statsText_);

    container = CreateContainer(HA_LEFT, VA_BOTTOM, 100);
    modeText_ = new Text(context_);
    modeText_->SetStyle("TextMonospaceShadowed", style);
    container->AddChild(modeText_);

    // Profiler tab is implemented here. Normally 3rd party code calls AddTab.
    SharedPtr<UIElement> profilerText(new Text(context_));
    AddTab("Profiler", Urho3D::StaticCast<DebugHudUpdater>(profilerUpdater_), profilerText);

    SetDefaultStyle(context_->GetSubsystem<ResourceCache>()->GetResource<XMLFile>("UI/DefaultStyle.xml"));

    framework_->Frame()->PostFrameUpdate.Connect(this, &DebugHud::OnUpdate);
    SubscribeToEvent(E_SCREENMODE, HANDLER(DebugHud, HandleWindowChange));
}

DebugHud::~DebugHud()
{
    foreach(auto tab, tabs_)
    {
        if (tab->widget)
        {
            UIElement *parent = tab->widget->GetParent();
            if (parent)
                parent->Remove();
            tab->widget->Remove();
        }
        delete tab;
    }
    tabs_.Clear();
    currentTab_ = 0;

    foreach(SharedPtr<BorderImage> c, containers_)
        c->Remove();
    containers_.Clear();
}

void DebugHud::CreateTab(const String &name, DebugHudUpdaterPtr updater, UIElementPtr widget)
{
    XMLFile *style = context_->GetSubsystem<ResourceCache>()->GetResource<XMLFile>("UI/DefaultStyle.xml");

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
    if (!tabLayout_)
    {
        tabLayout_ = new UIElement(context_);
        tabLayout_->SetLayout(LM_HORIZONTAL, 15);
        tabLayout_->SetFixedHeight(35);
        auto border = tabLayout_->GetLayoutBorder();
        border.top_ = 8; border.bottom_ = 8;
        tabLayout_->SetLayoutBorder(border);
        tabContainer_->AddChild(tabLayout_);
    }

    SharedPtr<Button> button(new Button(context_));
    button->SetName(name);
    Text *text = button->CreateChild<Text>("buttonText" + name, 0);
    text->SetInternal(true);
    text->SetText(name);
    buttons_.Push(button);
    tabLayout_->AddChild(button);

    SubscribeToEvent(button, E_RELEASED, HANDLER(DebugHud, HandleTabChange));

    bool isFirstChild = (tabContainer_->GetNumChildren() == 1);
    ScrollView *view = tabContainer_->CreateChild<ScrollView>("scrollView" + name, Urho3D::M_MAX_UNSIGNED);

    /// Auto set style if a text widget to keep things looking consistent.
    Text *textWidget = dynamic_cast<Text*>(widget.Get());
    if (textWidget)
        textWidget->SetStyle("TextMonospaceShadowed", style);
    widget->SetVisible(isFirstChild);

    HudTab *tab = new HudTab();
    tab->name = name;
    tab->updater = updater;
    tab->widget = widget;
    tabs_.Push(tab);

    view->SetContentElement(widget);
    view->SetVisible(isFirstChild);
    view->SetStyle("OverlayScrollView", style);
    tabContainer_->AddChild(view);

    if (isFirstChild && !currentTab_)
        currentTab_ = tab;

}

BorderImagePtr DebugHud::CreateContainer(int ha, int va, int priority)
{
    SharedPtr<BorderImage> container(new BorderImage(context_));
    container->SetAlignment((HorizontalAlignment)ha, (VerticalAlignment)va);
    container->SetPriority(priority);
    container->SetVisible(false);

    GetSubsystem<UI>()->GetRoot()->AddChild(container);
    
    containers_.Push(container);
    return container;
}

void DebugHud::SetDefaultStyle(XMLFile* style)
{
    if (!style)
        return;

    foreach(auto button, buttons_)
        button->SetStyle("Button", style);
    foreach(SharedPtr<BorderImage> c, containers_)
        c->SetStyle("VOverlayTransparent", style);
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
        stats.AppendWithFormat("Triangles %u\nBatches %u\nViews %u\nLights %u\nShadowmaps %u\nOccluders %u",
            primitives,
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

    if (currentTab_ && currentTab_->updater.Get())
        currentTab_->updater->UpdateDebugHud(frametime, currentTab_->widget);
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
    profilerUpdater_->profilerMaxDepth = depth;
}

void DebugHud::SetProfilerInterval(float interval)
{
    profilerUpdater_->profilerInterval = Max((int)(interval * 1000.0f), 0);
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
    return profilerUpdater_->profilerMaxDepth;
}

float DebugHud::GetProfilerInterval() const
{
    return (float)profilerUpdater_->profilerInterval / 1000.0f;
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
    if (button)
        ShowTab(button->GetName());
}

bool DebugHud::AddTab(const String &name, DebugHudUpdaterPtr updater, UIElementPtr widget)
{
    foreach(auto tab, tabs_)
    {
        if (tab->name.Compare(name, true) == 0)
        {
            LogErrorF("DebugHud::AddTab: Tab '%s' already exists.", name.CString());
            return false;
        }
    }
    CreateTab(name, updater, widget);
    return true;
}

void DebugHud::RemoveTab(const String &name)
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
            if (tab->widget)
            {
                UIElement *parent = tab->widget->GetParent();
                if (parent)
                    parent->Remove();
                tab->widget->Remove();
            }
            delete tab;
            return;
        }
    }
    LogErrorF("DebugHud::ShowTab: Tab '%s' does not exit.", name.CString());
}

void DebugHud::ShowTab(const String &name)
{
    if (name.Empty())
    {
        LogError("DebugHud::ShowTab: Provided tab name is empty");
        return;
    }

    UIElementPtr widget;
    foreach(auto tab, tabs_)
    {
        if (tab->name.Compare(name, true) == 0 && tab->widget)
        {
            widget = tab->widget;
            currentTab_ = tab;
            break;
        }
    }
    if (!widget)
    {
        LogErrorF("DebugHud::ShowTab: Tab '%s' does not exit.", name.CString());
        return;
    }
    foreach(auto tab, tabs_)
    {
        // Tab widgets are always parented to a ScrollView. Toggle its visibility.
        bool visible = (tab->widget.Get() == widget.Get());
        UIElement *parent = tab->widget->GetParent();
        if (parent)
            parent->SetVisible(visible);
        tab->widget->SetVisible(visible);
    }
    
}

StringVector DebugHud::TabNames() const
{
    StringVector names;
    foreach(auto tab, tabs_)
        names.Push(tab->name);
    return names;
}

// ProfilerHud

ProfilerHud::ProfilerHud(Framework *framework) :
    DebugHudUpdater(framework),
    profilerMaxDepth(M_MAX_UNSIGNED),
    profilerInterval(1000)
{
}

void ProfilerHud::UpdateDebugHud(float /*frametime*/, const UIElementPtr &widget)
{
    Text *profilerText = dynamic_cast<Text*>(widget.Get());
    if (!profilerText)
        return;
    Profiler* profiler = framework_->GetSubsystem<Profiler>();
    if (!profiler)
        return;

    if (profilerTimer_.GetMSec(false) >= profilerInterval)
    {
        profilerTimer_.Reset();

        String profilerOutput = profiler->GetData(false, false, profilerMaxDepth);
        profilerText->SetText(profilerOutput);

        profiler->BeginInterval();
    }
}

}
