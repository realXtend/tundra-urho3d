/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   SceneStructureWindow.h
    @brief  SceneStructureWindow. */

#pragma once

#include "ECEditorApi.h"
#include "CoreTypes.h"
#include "FrameworkFwd.h"
#include "IAttribute.h"
#include "IComponent.h"

#include <Urho3D/Container/Vector.h>
#include <Urho3D/Core/Object.h>

namespace Urho3D
{
class UIElement;
class Window;
class ListView;
class Button;
}

using namespace Urho3D;

namespace Tundra
{

class Scene;
class Entity;
class IComponent;
class IModule;

class TreeView;
class SceneStructureItem;
class SceneContextMenu;
class ECEditorCommandStack;
class AddComponentDialog;
class AddEntityDialog;

typedef WeakPtr<IModule> ModuleWeakPtr;
typedef WeakPtr<Entity> EntityWeakPtr;
typedef WeakPtr<IComponent> ComponentWeakPtr;

typedef SharedPtr<SceneContextMenu> SceneContextMenuPtr;
typedef WeakPtr<TreeView> TreeViewWeakPtr;
typedef WeakPtr<UIElement> UIElementWeakPtr;
typedef WeakPtr<Window> UIWindowWeakPtr;
typedef WeakPtr<Button> ButtonWeakPtr;
typedef WeakPtr<ListView> ListViewWeakPtr;
typedef WeakPtr<Object> ObjectWeakPtr;
typedef SharedPtr<SceneStructureItem> SceneStructureItemPtr;
typedef SharedPtr<ECEditorCommandStack> CommandStackPtr;
typedef SharedPtr<AddComponentDialog> AddComponentDialogPtr;
typedef SharedPtr<AddEntityDialog> AddEntityDialogPtr;

class ECEDITOR_API SceneStructureWindow : public Object
{
    URHO3D_OBJECT(SceneStructureWindow, Object);

public:
    struct ListViewItem
    {
        SceneStructureItemPtr item_;
        ObjectWeakPtr object_;
    };

    explicit SceneStructureWindow(Framework *framework, IModule *owner);
    virtual ~SceneStructureWindow();

    /// Sets new scene to be shown in the tree view.
    /** Populates tree view with entities.
    If scene is set to null, the tree view is cleared and previous signal connections are disconnected.
    @param newScene Scene. */
    void SetShownScene(Scene *newScene);

    Scene *ShownScene() const
    {
        return scene_;
    }

    void Clear();
    void RefreshView();

    void Hide();
    void Show();

    void Update();

    SceneStructureItem *FindItem(Object *obj);

    void ToggleItem(SceneStructureItem *item);

protected:
    SceneStructureItem *FindItem(UIElement *element);
    SceneStructureItem *CreateItem(Object *obj, const String &text, SceneStructureItem *parent = 0);

    void OnEntityCreated(Entity* entity, AttributeChange::Type change);
    void OnComponentCreated(IComponent* component, AttributeChange::Type change);

    void AddEntity(Entity *entity);
    void AddComponent(IComponent *component);
    void AddScene(Scene *scene);

    void HideContextMenu();
    void ShowContextMenu(Object *obj, int x, int y);

    void EditSelection();

    // Tundra scene changed
    void OnComponentChanged(Entity *entity, IComponent *component, AttributeChange::Type change);
    void OnEntityChanged(Entity *entity, AttributeChange::Type change);
    void OnAttributeChanged(IComponent *component, IAttribute *attribute, AttributeChange::Type type);

    // Urho3D UI Events
    void OnTogglePressed(SceneStructureItem *item);
    void OnElementClicked(StringHash eventType, VariantMap &eventData);
    void OnItemClicked(StringHash eventType, VariantMap &eventData);
    void OnItemDoubleClicked(StringHash eventType, VariantMap &eventData);
    void OnContextMenuHide(StringHash eventType, VariantMap &eventData);
    void OnSelectionChanged(StringHash eventType, VariantMap &eventData);
    void OnCloseClicked(StringHash eventType, VariantMap &eventData);

    void OnComponentDialogClosed(AddComponentDialog *dialog, bool confirmed);
    void OnEntityDialogClosed(AddEntityDialog *dialog, bool confirmed);

    void OnActionSelected(SceneContextMenu *contextMenu, String id);

    bool ComponentSelected(IComponent *component) const;
    bool EntitySelected(Entity *component) const;
    void ClearSelection();
    void SelectEntity(Entity *entity);
    void SelectComponent(IComponent *component);

    Framework *framework_;

private:
    SceneWeakPtr scene_;

    AddComponentDialogPtr addComponentDialog_;
    AddEntityDialogPtr addEntityDialog_;
    SceneContextMenuPtr contextMenu_;

    UIWindowWeakPtr window_;
    ButtonWeakPtr closeButton_;
    ListViewWeakPtr listView_;
    Vector<ListViewItem> listItems_;

    Vector<EntityWeakPtr> selectedEntities_;
    Vector<ComponentWeakPtr> selectedComponents_;

    ModuleWeakPtr owner_;

    /*
    If Scene entity, component or attribute has changed in some way mark the editor as dirty and
    update the editor in next frame.
    */
    bool dirty_;
};

}
