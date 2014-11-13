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

DebugHud::DebugHud(Context* context) :
    Object(context),
    profilerMaxDepth_(M_MAX_UNSIGNED),
    profilerInterval_(1000),
    useRendererStats_(false),
    mode_(DEBUGHUD_SHOW_NONE)
{
    XMLFile *style = context_->GetSubsystem<ResourceCache>()->GetResource<XMLFile>("UI/DefaultStyle.xml");

    SharedPtr<BorderImage> container = CreateContainer(HA_LEFT, VA_TOP, 100);
    statsText_ = new Text(context_);
    statsText_->SetStyle("TextMonospaceShadowed", style);
    container->AddChild(statsText_);

    container = CreateContainer(HA_LEFT, VA_BOTTOM, 100);
    modeText_ = new Text(context_);
    modeText_->SetStyle("TextMonospaceShadowed", style);
    container->AddChild(modeText_);

    profilerText_ = new Text(context_);
    profilerText_->SetStyle("TextMonospaceShadowed", style);
    CreateTab("Profiler", Urho3D::StaticCast<UIElement>(profilerText_));

    SetDefaultStyle(context_->GetSubsystem<ResourceCache>()->GetResource<XMLFile>("UI/DefaultStyle.xml"));

    SubscribeToEvent(E_POSTUPDATE, HANDLER(DebugHud, HandlePostUpdate));
    SubscribeToEvent(E_SCREENMODE, HANDLER(DebugHud, HandleWindowChange));
}

DebugHud::~DebugHud()
{
    foreach(SharedPtr<BorderImage> c, containers_)
        c->Remove();

    widgets_.Clear();
    containers_.Clear();
}

void DebugHud::CreateTab(const String &name, UIElementPtr widget)
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

    widget->SetVisible(true);
    widgets_.Push(WidgetPair(name, widget));

    view->SetContentElement(widget);
    view->SetVisible(isFirstChild);
    view->SetStyle("OverlayScrollView", style);
    tabContainer_->AddChild(view);
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

    foreach(auto widget, widgets_)
    {
        Text *label = dynamic_cast<Text*>(widget.second_.Get());
        if (label)
            label->SetStyle("TextMonospaceShadowed", style);
    }
    foreach(auto button, buttons_)
        button->SetStyle("Button", style);
    foreach(SharedPtr<BorderImage> c, containers_)
        c->SetStyle("VOverlayTransparent", style);
}

void DebugHud::Update()
{
    if (mode_ == DEBUGHUD_SHOW_NONE)
        return;

    Graphics* graphics = GetSubsystem<Graphics>();
    Renderer* renderer = GetSubsystem<Renderer>();
    if (!renderer || !graphics)
        return;

    // Ensure UI-elements are not detached
    if (!statsText_->GetParent())
    {
        UI* ui = GetSubsystem<UI>();
        UIElement* uiRoot = ui->GetRoot();
        uiRoot->AddChild(statsText_);
        uiRoot->AddChild(modeText_);
        uiRoot->AddChild(profilerText_);
    }

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

    Profiler* profiler = GetSubsystem<Profiler>();
    if (profiler)
    {
        if (profilerTimer_.GetMSec(false) >= profilerInterval_)
        {
            profilerTimer_.Reset();

            if (profilerText_->IsVisible())
            {
                String profilerOutput = profiler->GetData(false, false, profilerMaxDepth_);
                profilerText_->SetText(profilerOutput);
            }

            profiler->BeginInterval();
        }
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
    profilerMaxDepth_ = depth;
}

void DebugHud::SetProfilerInterval(float interval)
{
    profilerInterval_ = Max((int)(interval * 1000.0f), 0);
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

XMLFile* DebugHud::GetDefaultStyle() const
{
    return statsText_->GetDefaultStyle(false);
}

float DebugHud::GetProfilerInterval() const
{
    return (float)profilerInterval_ / 1000.0f;
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

void DebugHud::HandlePostUpdate(StringHash /*eventType*/, VariantMap& /*eventData*/)
{
    using namespace PostUpdate;

    Update();
}

void DebugHud::HandleTabChange(StringHash /*eventType*/, VariantMap& eventData)
{
    using namespace Released;

    Button *button = dynamic_cast<Button*>(eventData[P_ELEMENT].GetPtr());
    if (button)
        ShowTab(button->GetName());
}

void DebugHud::ShowTab(const String &name)
{
    if (name.Empty())
    {
        LogError("DebugHud::ShowTab: Provided tab name is empty");
        return;
    }

    UIElementPtr tab;
    for (auto iter = widgets_.Begin(); iter != widgets_.End(); ++iter)
    {
        if (iter->first_.Compare(name, true) == 0)
        {
            tab = iter->second_;
            break;
        }
    }
    if (!tab)
    {
        LogErrorF("DebugHud::ShowTab: Tab '%s' does not exit.", name.CString());
        return;
    }
    for (auto iter = widgets_.Begin(); iter != widgets_.End(); ++iter)
    {
        // Tab widgets are always parented to a ScrollView. Toggle its visibility.
        bool visible = iter->second_.Get() == tab.Get();
        UIElement *parent = iter->second_->GetParent();
        if (parent)
            parent->SetVisible(visible);
        iter->second_->SetVisible(visible);
    }
}

StringVector DebugHud::TabNames() const
{
    StringVector names;
    for (auto iter = widgets_.Begin(); iter != widgets_.End(); ++iter)
        names.Push(iter->first_);
    return names;
}

}
