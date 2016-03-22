// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

#include "SceneStructureItem.h"
#include "Framework.h"
#include "Entity.h"
#include "IComponent.h"

#include "LoggingFunctions.h"

#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/UIElement.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/ListView.h>

namespace Tundra
{

SceneStructureItem::SceneStructureItem(Context* context, ListView *list, Object *object) :
    Object(context), list_(0),
    style_(0), text_(0),
    toggleButton_(0)
{
    list_ = ListViewWeakPtr(list);
    style_ = context_->GetSubsystem<ResourceCache>()->GetResource<XMLFile>("Data/UI/DefaultStyle.xml");

    text_ = new Text(context_);
    text_->SetStyle("FileSelectorListText", style_);

    toggleButton_ = new Button(context_);
    toggleButton_->SetStyle("HierarchyArrowDown", style_);
    toggleButton_->SetPosition(IntVector2(0, 0));
    toggleButton_->SetVisible(false);
    SubscribeToEvent(toggleButton_, E_RELEASED, URHO3D_HANDLER(SceneStructureItem, OnItemPressed));

    SetData(object);

    text_->AddChild(toggleButton_);
}

SceneStructureItem::~SceneStructureItem()
{
    if (toggleButton_.Get())
    {
        toggleButton_->Remove();
        toggleButton_.Reset();
    }

    if (text_.Get())
    {
        text_->Remove();
        text_.Reset();
    }
}

void SceneStructureItem::SetIndent(int indent, int indentSpacing)
{
    toggleButton_->SetPosition(indent * indentSpacing, 0);
}

void SceneStructureItem::SetData(Object *obj)
{
    data_ = ObjectWeakPtr(obj);

    if (Entity *entity = dynamic_cast<Tundra::Entity*>(obj))
    {
        toggleButton_->SetVisible(true);
        if (entity->IsTemporary())
            SetColor(Color(0.9, 0.3, 0.3));
        else if (entity->IsLocal())
            SetColor(Color(0.3, 0.3, 0.9));
    }
    else
    {
        if (IComponent *comp = dynamic_cast<Tundra::IComponent*>(obj))
        {
            if (comp->ParentEntity()->IsTemporary())
                SetColor(Color(0.9, 0.3, 0.3));
            else if (comp->ParentEntity()->IsLocal())
                SetColor(Color(0.3, 0.3, 0.9));
        }
        toggleButton_->SetVisible(false);
    }
}

Object *SceneStructureItem::Data() const
{
    return data_.Get();
}

void SceneStructureItem::SetColor(Color color)
{
    text_->SetColor(color);
}

void SceneStructureItem::Refresh()
{
    if (list_.Get())
    {
        bool expanded = list_->IsExpanded(list_->FindItem(text_));
        if (expanded)
            toggleButton_->SetStyle("HierarchyArrowDown", style_);
        else
            toggleButton_->SetStyle("HierarchyArrowRight", style_);
    }
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