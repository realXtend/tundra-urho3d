// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

#include "UiAPI.h"
#include "MenuBar.h"
#include "MenuBarItem.h"
#include "Framework.h"
#include "FrameAPI.h"

namespace Tundra
{

UiAPI::UiAPI(Framework *framework) :
    Object(framework->GetContext()),
    framework_(framework),
    menuBar_(0)
{
    if (!framework_->HasCommandLineParameter("--nomenubar") && !framework_->HasCommandLineParameter("--nocentralwidget"))
    {
        CreateMenuBar();
        // Wait one frame before the rendering window is initialized
        framework_->Frame()->DelayedExecute(0.0f).Connect(this, &UiAPI::Initialize);
    }
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
    if (!menuBar_.Get())
        return;

    menuBar_->Show();
    if (menuBar_->Children().Size() == 0)
        menuBar_->Hide();
}

}