/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   SceneStructureItem.h
    @brief  SceneStructureItem. */

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

/** Single SceneStructureWindow list ui item that dont hold parent child hierachy information, instead Urho's UIElements are used to keep track
    of this. Scene, Entity and Component can be attached to each item. This is need to track if the item is temparary or local.
*/
class ECEDITOR_API SceneStructureItem : public Object
{
    URHO3D_OBJECT(SceneStructureItem, Object);

public:

    explicit SceneStructureItem(Context *context, ListView *list, Object *object);
    virtual ~SceneStructureItem();

    /// Set header text
    void SetText(const String &text);

    /// Get SceneStructureItem root item
    UIElement *Widget() const;

    /// Set list item indent
    void SetIndent(int indent, int indentSpacing = 16);

    /// Set meta data
    void SetData(Object *obj);

    /// Get meta data
    Object *Data() const;

    /// Set header text color
    void SetColor(Color color);

    /// Refresh ui arrow
    void Refresh();

    /// Triggered when toggle arrow button is being pressed
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
