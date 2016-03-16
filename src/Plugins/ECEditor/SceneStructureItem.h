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
    class ListView;
    class XMLFile;
}

using namespace Urho3D;

namespace Tundra
{

class SceneStructureWindow;

typedef WeakPtr<Button> ButtonWeakPtr;
typedef WeakPtr<Text> TextWeakPtr;
typedef WeakPtr<Object> ObjectWeakPtr;
typedef WeakPtr<ListView> ListViewWeakPtr;

class ECEDITOR_API SceneStructureItem : public Object
{
    URHO3D_OBJECT(SceneStructureItem, Object);

public:

    explicit SceneStructureItem(Context *context, ListView *list, Object *object);
    virtual ~SceneStructureItem();

    void SetText(const String &text);
    UIElement *Widget() const;
    void SetIndent(int indent, int indentSpacing = 16);

    void SetData(Object *obj);
    Object *Data() const;

    void SetColor(Color color);

    void Refresh();

    Signal1<SceneStructureItem* ARG(SceneStructureItem)> OnTogglePressed;

protected:

    void OnItemPressed(StringHash eventType, VariantMap& eventData);
    
    XMLFile *style_;
    ListViewWeakPtr list_;
    TextWeakPtr text_;
    ButtonWeakPtr toggleButton_;
    ObjectWeakPtr data_;
};

}
