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
    /// Data object holding ui ListItem pointer and entity or component pointer.
    struct ListViewItem
    {
        SceneStructureItemPtr item_;
        ObjectWeakPtr object_;

        bool operator!=(const ListViewItem& rhs)
        {
            return item_ != rhs.item_ || object_ != rhs.object_;
        }
    };

    explicit SceneStructureWindow(Framework *framework, IModule *owner);
    virtual ~SceneStructureWindow();

    /// Sets new scene to be shown in the list.
    /** Populates list view with entities.
    If scene is set to null, the view is cleared.
    @param newScene Scene. */
    void SetShownScene(Scene *newScene);

    Scene *ShownScene() const
    {
        return scene_;
    }

    /// Clear all ui and data objects from the editor but keep shown scene.
    void Clear();

    /// Refresh the UI elemnts
    /** @TODO optimize refresh code, current version will recreate all ui elements
    */
    void RefreshView();

    /// Hide editor window
    void Hide();

    /// Show editor window
    void Show();

    /// Check if editor window needs refreshing.
    void Update();

    /// Search for SceneStrucutreItem for given Entity or Component ptr.
    /** @param obj Entity or Component ptr.
        @return Return scene structure item if found otherwise return null.
    */
    SceneStructureItem *FindItem(Object *obj);

    /// Expand or Reduce SceneStructureItem in ListView.
    /** @param item toggle item target
    */
    void ToggleItem(SceneStructureItem *item);

protected:
    void Copy(Entity *entity);
    void Copy(IComponent *component);
    void Paste();

    SceneStructureItem *FindItem(UIElement *element);
    void RemoveItem(SceneStructureItem *item);

    SceneStructureItem *CreateItem(Object *obj, const String &text, SceneStructureItem *parent = 0);

    void OnEntityCreated(Entity* entity, AttributeChange::Type change);
    void OnComponentCreated(IComponent* component, AttributeChange::Type change);

    void AddEntity(Entity *entity);
    void RemoveEntity(Entity *entity);
    void AddComponent(IComponent *component);
    void RemoveComponent(IComponent *component);
    void AddScene(Scene *scene);

    void HideContextMenu();
    void ShowContextMenu(Object *obj, int x, int y);

    void EditEntity(Entity *entity);
    void EditComponent(IComponent *component);

    // Tundra scene changed
    void OnComponentRemoved(Entity *entity, IComponent *component, AttributeChange::Type change);
    void OnComponentAdded(Entity *entity, IComponent *component, AttributeChange::Type change);
    void OnEntityRemoved(Entity *entity, AttributeChange::Type change);
    void OnEntityAdded(Entity *entity, AttributeChange::Type change);
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
    bool EntitySelected(Entity *entity) const;
    void ClearSelection();
    void SelectEntity(Entity *entity);
    void SelectComponent(IComponent *component);

    Framework *framework_;

private:
    /// Selcted scene that this window is editing.
    SceneWeakPtr scene_;
    /// Compoent selection dialog window
    AddComponentDialogPtr addComponentDialog_;
    /// Entity selection dialog window
    AddEntityDialogPtr addEntityDialog_;
    /// Right click context menu
    SceneContextMenuPtr contextMenu_;
    /// Scene strucutre window root object
    UIWindowWeakPtr window_;
    /// Scene strucuture list element
    ListViewWeakPtr listView_;
    /// List item data object
    Vector<ListViewItem> listItems_;
    /// Array of selected entities
    Vector<EntityWeakPtr> selectedEntities_;
    /// Array of selected components
    Vector<ComponentWeakPtr> selectedComponents_;
    /// ECEditor module
    ModuleWeakPtr owner_;
    /*
    If Scene entity, component or attribute has changed in some way,
    mark the editor as dirty and update the editor window in next frame.
    */
    bool dirty_;
};

}
