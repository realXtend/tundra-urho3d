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
typedef WeakPtr<Window> WindowWeakPtr;
typedef HashMap<String, TextWeakPtr> SceneContextItemMap;

class ECEDITOR_API SceneContextMenu : public Object
{
    URHO3D_OBJECT(SceneContextMenu, Object);

public:
    SceneContextMenu(Context* context);
    virtual ~SceneContextMenu();
    
    Text *GetItem(const String &id);
    Text *CreateItem(const String &id, const String &text);
    void Clear();

    UIElement *Widget();

    //Signal1<SceneContextMenu* ARG(SceneContextMenu)> OnSelect;

protected:

    void OnItemPressed(StringHash eventType, VariantMap& eventData);
    WindowWeakPtr window_;
    SceneContextItemMap contextItemMap_;
};

}
