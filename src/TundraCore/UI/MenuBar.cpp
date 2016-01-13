// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

#include "MenuBar.h"
#include "MenuBarItem.h"
#include "Framework.h"

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

MenuBar::MenuBar(Framework *framework) :
    Object(framework->GetContext()),
    framework_(framework)
{
    Create();
}

MenuBar::~MenuBar()
{
    bar_->Remove();
    bar_.Reset();
    for (HashMap<String, MenuBarItemPtr>::Iterator iter = rootItems_.Begin(); iter != rootItems_.End(); ++iter)
    {
        iter->second_.Reset();
    }
}

void MenuBar::Create()
{
    XMLFile *style = context_->GetSubsystem<ResourceCache>()->GetResource<XMLFile>("Data/UI/LoginStyle.xml");

    bar_ = new Menu(framework_->GetContext());
    GetSubsystem<UI>()->GetRoot()->AddChild(bar_);
    bar_->SetStyle("MenuBar", style);
    bar_->SetHeight(24);
    bar_->SetMinHeight(24);
    bar_->SetPosition(IntVector2(0, 0));
    bar_->SetAlignment(HA_LEFT, VA_TOP);
    bar_->SetFixedWidth(GetSubsystem<UI>()->GetRoot()->GetWidth());
    bar_->SetLayout(LM_HORIZONTAL, 4, IntRect(4, 0, 4, 0));

    Hide();
}

void MenuBar::Show()
{
    if (bar_.NotNull())
        bar_->SetVisible(true);
}

void MenuBar::Hide()
{
    if (bar_.NotNull())
        bar_->SetVisible(false);
}

MenuBarItem* MenuBar::Find(const String &title)
{
    MenuBarItem *item = NULL;
    if (title.Empty())
        return item;

    Vector<String> titles = title.Split('/', true);
    if (titles.Size() > 0)
    {
        item = GetMenuItem(titles[0]);
        if (item != NULL && titles.Size() > 1)
        {
            String subTitle = title.Substring(titles[0].Length() + 1);
            item = item->Find(subTitle);
        }
    }
    return item;
}

MenuBarItem* MenuBar::GetMenuItem(const String &title)
{
    // If menu item is already created return it back to user.
    if (rootItems_.Contains(title))
        return rootItems_[title];
    return NULL;
}

MenuBarItem* MenuBar::CreateMenuItem(const String &title)
{
    MenuBarItem *item = NULL;
    if (title.Empty())
        return item;

    Vector<String> titles = title.Split('/', true);
    if (titles.Size() > 0)
    {
        Show();
        item = GetMenuItem(titles[0]);
        if (item != NULL) // MenuItem already exist
        {
            if (titles.Size() == 1)
                return item;

            String subTitle = title.Substring(titles[0].Length() + 1);
            return item->CreateMenuItem(subTitle);
        }
        else // Root item missing
        {
            item = new MenuBarItem(titles[0], framework_, this);
            rootItems_[titles[0]] = item;

            if (titles.Size() > 1)
            {
                String subTitle = title.Substring(titles[0].Length() + 1);
                return item->CreateMenuItem(subTitle);
            }
        }
    }
    return item;
}

void MenuBar::RemoveMenuItem(const String &title)
{
    if (rootItems_.Contains(title))
    {
        rootItems_[title].Reset();
        rootItems_.Erase(title);
    }
}

UIElement* MenuBar::Root() const
{
    return bar_;
}

}