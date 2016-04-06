/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   SceneContextMenu.h
    @brief  SceneContextMenu. */

#pragma once

#include "ECEditorApi.h"
#include "CoreTypes.h"
#include "FrameworkFwd.h"
#include "Signals.h"

#include <Urho3D/Core/Object.h>
#include <Urho3D/Container/Ptr.h>
#include <Urho3D/Container/HashMap.h>
#include <Urho3D/UI/UIElement.h>
#include <Urho3D/UI/Window.h>

namespace Urho3D
{
    class Menu;
    class Window;
    class Text;
}

using namespace Urho3D;

namespace Tundra
{

typedef WeakPtr<UIElement> UIElementWeakPtr;
typedef WeakPtr<Text> TextWeakPtr;
typedef WeakPtr<Menu> MenuWeakPtr;
typedef WeakPtr<Window> WindowWeakPtr;
typedef HashMap<String, MenuWeakPtr> SceneContextItemMap;

class ECEDITOR_API SceneContextMenu : public Object
{
    URHO3D_OBJECT(SceneContextMenu, Object);

public:
    explicit SceneContextMenu(Context* context);
    virtual ~SceneContextMenu();
    
    /// Get Menu item by given id
    Menu *GetItem(const String &id);

    /// Create new context menu item by given id and header text.
    Menu *CreateItem(const String &id, const String &text);

    /// Clear context menu ui
    void Clear();

    /// Get Context Menu ui root object.
    UIElement *Widget();

    /// Is context menu visible
    bool IsVisible() const;

    /// Set context menu visible
    void Open();

    /// Hide context menu
    void Close();

    /// Triggered when conext menu item is selected by user
    Signal2<SceneContextMenu*, String> OnActionSelected;

protected:
    String GetItemId(Menu *menu);

    void OnItemPressed(StringHash eventType, VariantMap& eventData);

    WindowWeakPtr window_;
    SceneContextItemMap contextItemMap_;
};

}

