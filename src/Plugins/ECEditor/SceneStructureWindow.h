/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   SceneStructureWindow.h
    @brief  SceneStructureWindow core API. */

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

class TreeView;
class SceneStructureItem;
class SceneContextMenu;
class ECEditorCommandStack;

typedef WeakPtr<Entity> EntityWeakPtr;
typedef WeakPtr<IComponent> ComponentWeakPtr;

typedef WeakPtr<SceneContextMenu> SceneContextMenuWeakPtr;
typedef WeakPtr<TreeView> TreeViewWeakPtr;
typedef WeakPtr<UIElement> UIElementWeakPtr;
typedef WeakPtr<Window> UIWindowWeakPtr;
typedef WeakPtr<ListView> ListViewWeakPtr;
typedef WeakPtr<Object> ObjectWeakPtr;
typedef SharedPtr<SceneStructureItem> SceneStructureItemPtr;
typedef SharedPtr<ECEditorCommandStack> CommandStackPtr;

class ECEDITOR_API SceneStructureWindow : public Object
{
    URHO3D_OBJECT(SceneStructureWindow, Object);

public:
    struct ListViewItem
    {
        SceneStructureItemPtr item_;
        ObjectWeakPtr object_;
    };

    SceneStructureWindow(Framework *framework);
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

    SceneStructureItem *FindItem(Object *obj);

protected:
    SceneStructureItem *FindItem(UIElement *element);
    SceneStructureItem *CreateItem(Object *obj, const String &text, SceneStructureItem *parent = 0);

    void OnSceneCreated(Scene* scene, Tundra::AttributeChange::Type change);
    void OnEntityCreated(Entity* entity, AttributeChange::Type change);
    void OnComponentCreated(IComponent* component, AttributeChange::Type change);

    void AddEntity(Entity *entity);
    void AddComponent(IComponent *component);
    void AddScene(Scene *scene);

    void HideContextMenu();
    void ShowContextMenu(Object *obj, int x, int y);

    // Events
    void OnTogglePressed(SceneStructureItem *item);
    void OnItemClicked(StringHash eventType, VariantMap &eventData);
    void OnItemDoubleClicked(StringHash eventType, VariantMap &eventData);
    void OnContextMenuHide(StringHash eventType, VariantMap &eventData);
    void OnSelectionChanged(StringHash eventType, VariantMap &eventData);

    void OnActionSelected(SceneContextMenu *contextMenu, String id);

    bool ComponentSelected(IComponent *component) const;
    bool EntitySelected(Entity *component) const;
    void ClearSelection();
    void SelectEntity(Entity *entity);
    void SelectComponent(IComponent *component);

    Framework *framework_;

private:
    Scene *scene_;

    UIWindowWeakPtr window_;
    SceneContextMenuWeakPtr contextMenu_;
    ListViewWeakPtr listView_;
    Vector<ListViewItem> listItems_;

    Vector<EntityWeakPtr> selectedEntities_;
    Vector<ComponentWeakPtr> selectedComponents_;
};

}
