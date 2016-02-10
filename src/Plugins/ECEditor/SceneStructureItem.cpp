// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

#include "SceneStructureItem.h"
#include "Framework.h"

#include "LoggingFunctions.h"

#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/UIElement.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/Button.h>

namespace Tundra
{

SceneStructureItem::SceneStructureItem(Context* context) :
    Object(context)
{
    XMLFile *style = context_->GetSubsystem<ResourceCache>()->GetResource<XMLFile>("Data/UI/DefaultStyle.xml");

    text_ = new Text(context_);
    text_->SetStyle("FileSelectorListText", style);

    toggleButton_ = new Button(context_);
    toggleButton_->SetStyle("ArrowDown", style);
    toggleButton_->SetPosition(IntVector2(0, 0));
    SubscribeToEvent(toggleButton_, E_RELEASED, URHO3D_HANDLER(SceneStructureItem, OnItemPressed));
    text_->AddChild(toggleButton_);
}

SceneStructureItem::~SceneStructureItem()
{
    if (text_.NotNull())
    {
        text_->Remove();
        text_.Reset();
    }

    if (toggleButton_.NotNull())
    {
        toggleButton_->Remove();
        toggleButton_.Reset();
    }
}

void SceneStructureItem::SetIndent(int indent, int indentSpacing)
{
    toggleButton_->SetPosition(indent * indentSpacing, 0);
}

void SceneStructureItem::SetType(ItemType type)
{
    if (type == type_)
        return;

    type_ = type;
    switch (type_)
    {
        case ItemType::Entity:
            toggleButton_->SetVisible(true);
        break;
        case ItemType::Component:
        case ItemType::Attribute:
            toggleButton_->SetVisible(false);
        break;
    }
}

SceneStructureItem::ItemType SceneStructureItem::Type() const
{
    return type_;
}

void SceneStructureItem::SetText(const String &text)
{
    text_->SetText(text);
}

UIElement *SceneStructureItem::Widget() const
{
    return text_;
}

void SceneStructureItem::OnItemPressed(StringHash /*eventType*/, VariantMap& /*eventData*/)
{
    OnTogglePressed.Emit(this);
}

}