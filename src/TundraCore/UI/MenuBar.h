// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"
#include "FrameworkFwd.h"
#include "CoreTypes.h"

#include <Urho3D/Container/HashMap.h>
#include <Urho3D/Container/Ptr.h>
#include <Urho3D/UI/UIElement.h>


namespace Urho3D
{
class Menu;
class Window;
}

using namespace Urho3D;

namespace Tundra
{

class MenuBarItem;
typedef SharedPtr<MenuBarItem> MenuBarItemPtr;

/// Top menu bar object
class TUNDRACORE_API MenuBar : public Object
{
    URHO3D_OBJECT(MenuBar, Object);

public:
    MenuBar(Framework *framework);
    virtual ~MenuBar();

    /// Creates panel ui.
    void Create();

    /// Show menu bar ui element
    void Show();

    /// Hide menu bar ui element
    void Hide();

    /// Hide all menu popups.
    void Close();

    /// Search for MenuBarItem by it's title
    /** Supports both search by title and hierarchical search
    e.g. Find("File") or Find("File/Save Scene")
    @param title search title
    @return MenuBarItem or null if not found
    */
    MenuBarItem* Find(const String &title);

    /// Get MenuBarItem from MenuBar
    MenuBarItem* GetMenuItem(const String &title);

    /// Construct a new MenuBarItem
    MenuBarItem* CreateMenuItem(const String &title);

    /// Remove child MenuItem from MenuBar
    void RemoveMenuItem(const String &title);

    /// Get MenuBar's root UiElement
    UIElement* Root() const;

protected:
    Framework *framework_;

private:

    HashMap<String, MenuBarItemPtr> rootItems_;
    SharedPtr<Menu> bar_;
};

}
