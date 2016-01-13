// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

#include "UiAPI.h"
#include "MenuBar.h"
#include "MenuBarItem.h"
#include "Framework.h"
#include "FrameAPI.h"

#include <Urho3D/Core/Context.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Core/Profiler.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/BorderImage.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/Menu.h>
#include <Urho3D/UI/LineEdit.h>
#include <Urho3D/UI/DropDownList.h>

namespace Tundra
{

UiAPI::UiAPI(Framework *framework) :
    Object(framework->GetContext()),
    framework_(framework),
    menuBar_(0)
{
    // Wait for one frame before intilizing ui elements.
    framework_->Frame()->DelayedExecute(0.0f).Connect(this, &UiAPI::Initialize);
}

UiAPI::~UiAPI()
{
    RelaseMenuBar();
}

MenuBar* UiAPI::GetMenuBar() const
{
    return menuBar_;
}

void UiAPI::CreateMenuBar()
{
    menuBar_ = new MenuBar(framework_);
}

void UiAPI::RelaseMenuBar()
{
    menuBar_.Reset();
}

void UiAPI::Initialize(float /*time*/)
{
    CreateMenuBar();
}

}