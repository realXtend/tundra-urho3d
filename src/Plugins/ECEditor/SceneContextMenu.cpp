// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

#include "SceneContextMenu.h"
#include "Framework.h"

#include "LoggingFunctions.h"

#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/UIElement.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/Menu.h>

namespace Tundra
{

SceneContextMenu::SceneContextMenu(Context* context) :
    Object(context)
{
    XMLFile *style = context_->GetSubsystem<ResourceCache>()->GetResource<XMLFile>("Data/UI/DefaultStyle.xml");

    window_ = new Window(context);
    window_->SetStyle("Window", style);
    window_->SetName("SceneContextMenu");
    window_->SetMinWidth(150);
    window_->SetMinHeight(67);
    window_->SetMaxWidth(150);
    window_->SetLayoutMode(LM_VERTICAL);
    window_->SetLayout(LM_VERTICAL, 4, IntRect(6, 6, 6, 6));
    window_->SetMovable(false);
    window_->SetEnabled(true);
}

SceneContextMenu::~SceneContextMenu()
{
    if (window_.NotNull())
        window_->Remove();
    window_.Reset();
}

Text *SceneContextMenu::GetItem(const String &id)
{
    if (contextItemMap_.Contains(id))
        return contextItemMap_[id];
    return NULL;
}

Text *SceneContextMenu::CreateItem(const String &id, const String &text)
{
    Text *t = GetItem(id);
    if (t != NULL)
    {
        t->SetText(text);
    }
    else
    {
        XMLFile *style = context_->GetSubsystem<ResourceCache>()->GetResource<XMLFile>("Data/UI/DefaultStyle.xml");

        t = new Text(context_);
        t->SetText(text);
        t->SetStyle("FileSelectorListText", style);
        t->SetEnabled(true);
        t->SetFocusMode(FocusMode::FM_FOCUSABLE_DEFOCUSABLE);
        contextItemMap_[id] = TextWeakPtr(t);
        SubscribeToEvent(t, E_FOCUSED, URHO3D_HANDLER(SceneContextMenu, OnItemPressed));
        window_->AddChild(t);
    }
    return t;
}

void SceneContextMenu::Clear()
{
    
}

UIElement *SceneContextMenu::Widget()
{
    return window_;
}

void SceneContextMenu::OnItemPressed(StringHash /*eventType*/, VariantMap& /*eventData*/)
{
    LogWarning("Clicked!");
}

}