/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   ECEditor.h
    @brief  ECEditor core API. */

#pragma once

#include "ECEditorApi.h"
#include "CoreTypes.h"
#include "FrameworkFwd.h"
#include "IAttribute.h"

#include <Urho3D/Core/Object.h>
#include <Urho3D/Container/Ptr.h>

namespace Urho3D
{
    class Window;
    class UIElement;
    class ListView;
    class Text;
	class Button;
}

using namespace Urho3D;

namespace Tundra
{

class Entity;
class IComponent;
class ComponentEditor;
class IAttributeEditor;

typedef SharedPtr<IComponent> ComponentPtr;
typedef SharedPtr<Entity> EntityPtr;
typedef SharedPtr<ListView> ListViewPtr;
typedef SharedPtr<UIElement> UIElementPtr;
typedef SharedPtr<IAttributeEditor> AttributeEditorPtr;

typedef WeakPtr<IComponent> ComponentWeakPtr;
typedef WeakPtr<Entity> EntityWeakPtr;
typedef WeakPtr<Text> TextWeakPtr;
typedef WeakPtr<Button> ButtonWeakPtr;
typedef HashMap<entity_id_t, EntityWeakPtr> EntityMap;
typedef HashMap<int, AttributeEditorPtr> AttributeEditorMap;

typedef SharedPtr<Window> WindowPtr;

class ECEDITOR_API ComponentContainer : public Object
{
    URHO3D_OBJECT(ComponentContainer, Object);

public:
    explicit ComponentContainer(Framework *framework, ComponentPtr component, int index);
    virtual ~ComponentContainer();

    void SetTitleText(const String &text);
    String TitleText() const;

    UIElement *Widget() const;
    int Index() const;

    static IAttributeEditor *CreateAttributeEditor(Framework *framework, IAttribute *attribute);

protected:
    WindowPtr window_;
    UIElementPtr attributeContainer_;
    TextWeakPtr header_;
    ComponentWeakPtr component_;
    AttributeEditorMap attributeEditors_;

    // Defines component position in ECEDitorWindow
    int index_;

    Framework *framework_;
};

typedef SharedPtr<ComponentContainer> ComponentContainerPtr;
typedef HashMap<ComponentWeakPtr, ComponentContainerPtr> ComponentContainerMap;
typedef Vector<ComponentContainerPtr> ComponentContainerVector;

class ECEDITOR_API ECEditorWindow : public Object
{
    URHO3D_OBJECT(ECEditorWindow, Object);

public:
    explicit ECEditorWindow(Framework *framework);
    virtual ~ECEditorWindow();

    void Show();
    void Hide();

    void AddEntity(EntityPtr entity, bool updateUi = true);
    void AddEntity(entity_id_t id, bool updateUi = true);

    void Clear();
    void Refresh();
    UIElement* Widget() const;

protected:
    void OnCloseClicked(StringHash eventType, VariantMap &eventData);

    ComponentContainerMap containers_;
    ComponentContainerVector components_;

    WindowPtr window_;
    ListViewPtr list_;
	ButtonWeakPtr closeButton_;
    
    EntityWeakPtr entity_;

    Framework *framework_;
};

}
