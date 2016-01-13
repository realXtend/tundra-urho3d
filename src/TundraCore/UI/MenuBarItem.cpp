// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

#include "MenuBar.h"
#include "MenuBarItem.h"
#include "Framework.h"
#include "FrameAPI.h"

#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/Menu.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/Window.h>

using namespace Urho3D;

namespace Tundra
{

MenuBarItem::MenuBarItem(const String &title, Framework *framework, MenuBar *menuBar, MenuBarItem *parentItem) :
    Object(framework->GetContext()),
    framework_(framework),
    menuBar_(menuBar),
    parentItem_(parentItem)
{
    Create(title);
}

MenuBarItem::~MenuBarItem()
{
    item_->Remove();
    item_.Reset();

    for (HashMap<String, MenuBarItemPtr>::Iterator iter = subMenus_.Begin(); iter != subMenus_.End(); ++iter)
        iter->second_.Reset();
}

void MenuBarItem::Create(const String &title)
{
    XMLFile *style = context_->GetSubsystem<ResourceCache>()->GetResource<XMLFile>("Data/UI/LoginStyle.xml");

    item_ = new Menu(framework_->GetContext());
    item_->SetStyle("Button", style);
    item_->SetAlignment(HA_LEFT, VA_TOP);
    item_->SetPopupOffset(IntVector2(0, item_->GetHeight()));

    Text *menuText = new Text(framework_->GetContext());
    menuText->SetText(title);
    menuText->SetStyle("Text", style);
    item_->AddChild(menuText);
    SubscribeToEvent(item_.Get(), E_RELEASED, URHO3D_HANDLER(MenuBarItem, OnMenuPressed));

    item_->SetFixedWidth(menuText->GetWidth() + 20);
    if (parentItem_.Null())
    {
        menuBar_->GetRoot()->AddChild(item_);
        menuText->SetAlignment(HA_CENTER, VA_CENTER);
    }
    else
    {
        menuText->SetAlignment(HA_LEFT, VA_CENTER);
        parentItem_->UpdatePopup();
    }

    if (parentItem_.NotNull())
    {
        UIElement *popup = parentItem_->GetMenu()->GetPopup();
        if (popup != NULL)
            popup->AddChild(item_);
        parentItem_->UpdatePopup();
    }
}

void MenuBarItem::CreatePopup()
{
    Window *popup = dynamic_cast<Window*>(item_->GetPopup());
    if (popup == NULL)
    {
        XMLFile *style = context_->GetSubsystem<ResourceCache>()->GetResource<XMLFile>("Data/UI/DefaultStyle.xml");

        popup = new Window(framework_->GetContext());
        popup->SetStyle("MenuBarPopup", style);
        popup->SetLayout(LM_VERTICAL, 1, IntRect(2, 4, 2, 4));
        popup->SetAlignment(HA_LEFT, VA_TOP);
        item_->SetPopup(popup);
    }
}

void MenuBarItem::RemovePopup()
{
    if (item_->GetPopup() != NULL)
        item_->GetPopup()->Remove();
}

void MenuBarItem::UpdatePopup()
{
    UIElement *element = item_->GetPopup();
    if (element == NULL)
        return;

    int width = element->GetWidth() - (element->GetLayoutBorder().left_ + element->GetLayoutBorder().right_);
    for (HashMap<String, MenuBarItemPtr>::Iterator iter = subMenus_.Begin(); iter != subMenus_.End(); ++iter)
        iter->second_->GetMenu()->SetFixedWidth(width);
    
    if (parentItem_.NotNull())
        item_->SetPopupOffset(IntVector2(parentItem_->GetMenu()->GetPopup()->GetWidth() - 2, -4));
}

void MenuBarItem::OnMenuPressed(StringHash /*eventType*/, VariantMap& /*eventData*/)
{
    OnItemPressed.Emit(this);
}

void MenuBarItem::UpdateUI(float /*time*/)
{
    UpdatePopup();
}

Menu* MenuBarItem::GetMenu() const
{
    return item_;
}

MenuBarItem* MenuBarItem::Find(const String &title)
{
    MenuBarItem *item = NULL;
    if (title.Empty())
        return item;

    Vector<String> titles = title.Split('/', true);
    if (titles.Size() > 0)
    {
        item = GetChild(titles[0]);
        if (item != NULL && titles.Size() > 1)
        {
            String subTitle = title.Substring(titles[0].Length() + 1);
            item = item->Find(subTitle);
        }
    }
    return item;
}

MenuBarItem* MenuBarItem::GetChild(const String &title)
{
    // If menu item is already created return it back to user.
    if (subMenus_.Contains(title))
        return subMenus_[title];
    return NULL;
}

void MenuBarItem::Remove()
{
    Text *text = dynamic_cast<Text*>(GetMenu()->GetChild(0));
    if (text != NULL)
    {
        String title = text->GetText();
        if (parentItem_ != NULL)
            parentItem_->RemoveMenuItem(title);
        else
            menuBar_->RemoveMenuItem(title);
    }
}

String MenuBarItem::Title() const
{
    Text *text = dynamic_cast<Text*>(GetMenu()->GetChild(0));
    if (text != NULL)
    {
        return text->GetText();
    }
    return "";
}

MenuBarItem* MenuBarItem::CreateMenuItem(const String &title)
{
    MenuBarItem *item = NULL;
    if (title.Empty())
        return item;

    Vector<String> titles = title.Split('/', true);
    if (titles.Size() > 0)
    {
        item = GetChild(titles[0]);
        if (item != NULL) // MenuItem already exist
        {
            if (titles.Size() == 1)
                return item;

            String subTitle = title.Substring(titles[0].Length() + 1);
            MenuBarItem *childItem = item->CreateMenuItem(subTitle);
            framework_->Frame()->DelayedExecute(0).Connect(this, &MenuBarItem::UpdateUI);
            return childItem;
        }
        else // Menu item missing
        {
            CreatePopup();
            item = new MenuBarItem(titles[0], framework_, menuBar_, this);
            subMenus_[titles[0]] = item;

            if (titles.Size() > 1)
            {
                String subTitle = title.Substring(titles[0].Length() + 1);
                MenuBarItem *childItem = item->CreateMenuItem(subTitle);
                framework_->Frame()->DelayedExecute(0).Connect(this, &MenuBarItem::UpdateUI);
                return childItem;
            }
        }
    }
    return item;
}

void MenuBarItem::RemoveMenuItem(const String &title)
{
    if (subMenus_.Contains(title))
    {
        subMenus_[title].Reset();
        subMenus_.Erase(title);

        // Remove popup ui element if no children exists
        if (subMenus_.Size() == 0)
            RemovePopup();

        framework_->Frame()->DelayedExecute(0).Connect(this, &MenuBarItem::UpdateUI);
    }
}

}