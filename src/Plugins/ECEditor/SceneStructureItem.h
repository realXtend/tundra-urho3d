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

namespace Urho3D
{
    class Text;
    class Button;
    class UIElement;
}

using namespace Urho3D;

namespace Tundra
{

class SceneStructureWindow;

typedef WeakPtr<Button> ButtonWeakPtr;
typedef WeakPtr<Text> TextWeakPtr;
typedef WeakPtr<Object> ObjectWeakPtr;

class ECEDITOR_API SceneStructureItem : public Object
{
    URHO3D_OBJECT(SceneStructureItem, Object);

public:
    enum ItemType
    {
        Entity = 0,
        Component,
        Attribute
    };

    SceneStructureItem(Context* context);
    virtual ~SceneStructureItem();

    void SetText(const String &text);
    UIElement *Widget() const;
    void SetIndent(int indent, int indentSpacing = 16);

    void SetType(ItemType type);
    ItemType Type() const;

    void SetData(Object *obj);
    Object *Data() const;

    Signal1<SceneStructureItem* ARG(SceneStructureItem)> OnTogglePressed;

protected:

    void OnItemPressed(StringHash eventType, VariantMap& eventData);
    
    TextWeakPtr text_;
    ButtonWeakPtr toggleButton_;
    ItemType type_;
    ObjectWeakPtr data_;
};

}
