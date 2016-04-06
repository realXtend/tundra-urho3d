/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   ECEditorWindow.h
    @brief  ECEditorWindow. */

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

/// Component editor ui element that displays signle component and it's attributes
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

    /// Factory method to create AttributeEditor for given attribute type.
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

/// Entity-component editor window.
class ECEDITOR_API ECEditorWindow : public Object
{
    URHO3D_OBJECT(ECEditorWindow, Object);

public:
    explicit ECEditorWindow(Framework *framework);
    virtual ~ECEditorWindow();

    void Show();
    void Hide();

    /// Set entity to editor.
    /** @param entity to display on editor.
    */
    void SetEntity(EntityPtr entity);

    /// Set entity to editor.
    /** @param id get entity by id.
    */
    void SetEntity(entity_id_t id);

    /// Scroll list view to given component
    void ScrollToComponent(IComponent *component);

    /// Clear UI
    void Clear();
    void Refresh();
    UIElement* Widget() const;

protected:
    void OnCloseClicked(StringHash eventType, VariantMap &eventData);

    ComponentContainerMap containers_;

    WindowPtr window_;
    ListViewPtr list_;
    ButtonWeakPtr closeButton_;
    
    EntityWeakPtr entity_;

    Framework *framework_;
};

}
