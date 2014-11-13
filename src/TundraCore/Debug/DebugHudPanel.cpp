// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "DebugHudPanel.h"

#include "UI/ScrollView.h"

namespace Tundra
{

DebugHudPanel::DebugHudPanel(Framework *framework) :
    framework_(framework)
{
}

DebugHudPanel::~DebugHudPanel()
{
    Destroy();
}

void DebugHudPanel::Create()
{
    widget_ = CreateImpl();
}

void DebugHudPanel::Destroy()
{
    if (widget_)
    {
        Urho3D::UIElement *parent = widget_->GetParent();
        if (parent)
        {
            /** Null out the shared ptr from parent scroll view so it can't retain
                the ptr that might have been created in another dynamic library.
                This is important step and reverses the DebugHud::AddTab parenting. */
            Urho3D::ScrollView *panel = dynamic_cast<Urho3D::ScrollView*>(parent->GetParent() ? parent->GetParent() : parent);
            if (panel)
                panel->SetContentElement(nullptr);
            parent->Remove();
        }
        widget_->Remove();
    }
    widget_.Reset();
}

SharedPtr<Urho3D::UIElement> DebugHudPanel::Widget()
{
    return widget_;
}

}