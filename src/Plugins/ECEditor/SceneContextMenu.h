/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   ECEditor.h
    @brief  ECEditor core API. */

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
    
    Menu *GetItem(const String &id);
    Menu *CreateItem(const String &id, const String &text);
    void Clear();

    UIElement *Widget();

    bool IsVisible() const;
    void Open();
    void Close();

    Signal2<SceneContextMenu*, String> OnActionSelected;

protected:
    String GetItemId(Menu *menu);

    void OnItemPressed(StringHash eventType, VariantMap& eventData);

    WindowWeakPtr window_;
    SceneContextItemMap contextItemMap_;
};

}

