// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

#include "SceneStructureWindow.h"
#include "SceneStructureItem.h"
#include "SceneContextMenu.h"
#include "Framework.h"
#include "Scene.h"
#include "SceneAPI.h"
#include "AddComponentDialog.h"

#include <Urho3D/Engine/Engine.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Core/Context.h>

#include "LoggingFunctions.h"

#include <Urho3D/UI/UI.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/UI/UIElement.h>
#include <Urho3D/UI/Window.h>
#include <Urho3D/UI/ListView.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/Text.h>

using namespace Urho3D;

namespace Tundra
{

SceneStructureWindow::SceneStructureWindow(Framework *framework) :
    Object(framework->GetContext()),
    framework_(framework)
{
    XMLFile *style = context_->GetSubsystem<ResourceCache>()->GetResource<XMLFile>("Data/UI/DefaultStyle.xml");

    window_ = new Window(framework->GetContext());
    window_->SetLayout(LayoutMode::LM_VERTICAL, 2, IntRect(2, 2, 2, 2));
    window_->SetSize(IntVector2(300, 500));
    window_->SetMinSize(IntVector2(300, 500));
    window_->SetStyle("Window", style);
    window_->SetMovable(true);
    window_->SetResizable(true);
    GetSubsystem<UI>()->GetRoot()->AddChild(window_);

    SubscribeToEvent(window_, E_POSITIONED, URHO3D_HANDLER(SceneStructureWindow, OnContextMenuHide));

    {
        UIElement *topBar = new UIElement(framework->GetContext());
        topBar->SetMinHeight(22);
        topBar->SetMaxHeight(22);
        window_->AddChild(topBar);

        {
            Button *button = new Button(framework->GetContext());
            button->SetName("CloseButton");
            button->SetStyle("CloseButton", style);
            button->SetAlignment(HA_RIGHT, VA_CENTER);
            button->SetPosition(IntVector2(-3, 0));
            topBar->AddChild(button);

            Text *windowHeader = new Text(framework->GetContext());
            windowHeader->SetStyle("Text", style);
            windowHeader->SetName("WindowHeader");
            windowHeader->SetText("Scene Editor");
            windowHeader->SetAlignment(HA_LEFT, VA_CENTER);
            windowHeader->SetPosition(IntVector2(3, 0));
            topBar->AddChild(windowHeader);
        }
    }

    listView_ = new ListView(framework->GetContext());
    listView_->SetStyle("HierarchyListView", style);
    listView_->SetMultiselect(true);
    listView_->SetEnabled(true);
    listView_->SetFocusMode(FocusMode::FM_FOCUSABLE_DEFOCUSABLE);
    window_->AddChild(listView_);

    SubscribeToEvent(listView_, E_ITEMDOUBLECLICKED, URHO3D_HANDLER(SceneStructureWindow, OnItemDoubleClicked));
    SubscribeToEvent(listView_, E_ITEMCLICKED, URHO3D_HANDLER(SceneStructureWindow, OnItemClicked));
    SubscribeToEvent(listView_, E_SELECTIONCHANGED, URHO3D_HANDLER(SceneStructureWindow, OnSelectionChanged));

    {
        UIElement *bottomBar = new UIElement(framework->GetContext());
        bottomBar->SetMinHeight(30);
        bottomBar->SetMaxHeight(30);
        bottomBar->SetLayout(LayoutMode::LM_HORIZONTAL, 12, IntRect(12, 2, 12, 2));
        window_->AddChild(bottomBar);

        {
            Button *button = new Button(framework->GetContext());
            button->SetName("UndoButton");
            Text *text = new Text(framework->GetContext());
            text->SetText("Undo");
            text->SetAlignment(HA_CENTER, VA_CENTER);
            text->SetInternal(true);
            
            button->AddChild(text);
            button->SetStyle("Button", style);
            button->SetMinWidth(50);
            button->SetMaxWidth(50);
            button->SetVerticalAlignment(VA_CENTER);
            bottomBar->AddChild(button);

            button = new Button(framework->GetContext());
            button->SetName("RedoButton");
            button->SetVerticalAlignment(VA_CENTER);
            text = new Text(framework->GetContext());
            text->SetText("Redo");
            text->SetInternal(true);

            button->AddChild(text);
            button->SetStyle("Button", style);
            button->SetMinWidth(50);
            button->SetMaxWidth(50);
            text->SetAlignment(HA_CENTER, VA_CENTER);
            bottomBar->AddChild(button);
        }
    }

    contextMenu_ = new SceneContextMenu(framework->GetContext());
    contextMenu_->Widget()->SetStyle("Window", style);

    contextMenu_->Widget()->SetPosition(IntVector2(100, 100));
    contextMenu_->OnActionSelected.Connect(this, &SceneStructureWindow::OnActionSelected);
    GetSubsystem<UI>()->GetRoot()->AddChild(contextMenu_->Widget());

    addComponentDialog_ = AddComponentDialogPtr(new AddComponentDialog(framework_));
    addComponentDialog_->DialogClosed.Connect(this, &SceneStructureWindow::OnComponentDialogClosed);
    addComponentDialog_->Hide();

    framework->Scene()->SceneCreated.Connect(this, &SceneStructureWindow::OnSceneCreated);
}

SceneStructureWindow::~SceneStructureWindow()
{
    Clear();
    addComponentDialog_.Reset();

    if (contextMenu_.NotNull())
        contextMenu_->Widget()->Remove();
    contextMenu_.Reset();

    if (window_.NotNull())
        window_->Remove();
    window_.Reset();
}

SceneStructureItem *SceneStructureWindow::CreateItem(Object *obj, const String &text, SceneStructureItem *parent)
{
    SceneStructureItem *item = new SceneStructureItem(context_);
    if (parent == NULL)
    {
        listView_->InsertItem(listView_->GetNumItems(), item->Widget());
    }
    else
    {
        listView_->InsertItem(listView_->FindItem(parent->Widget()), item->Widget(), parent->Widget());
        item->SetIndent(parent->Widget()->GetIndent() + listView_->GetBaseIndent(), listView_->GetIndentSpacing());
    }
    item->SetText(text);
    if (IComponent *comp = dynamic_cast<IComponent*>(obj))
    {
        item->SetType(SceneStructureItem::ItemType::Component);
        if (comp->ParentEntity() != NULL)
        {
            if (comp->ParentEntity()->IsTemporary())
                item->SetColor(Color(0.9, 0.3, 0.3));
            else if (comp->ParentEntity()->IsLocal())
                item->SetColor(Color(0.3, 0.3, 0.9));
        }
    }
    else if (Entity *entity = dynamic_cast<Entity*>(obj))
    {
        item->SetType(SceneStructureItem::ItemType::Entity);
        if (entity->IsTemporary())
            item->SetColor(Color(0.9, 0.3, 0.3));
        else if (entity->IsLocal())
            item->SetColor(Color(0.3, 0.3, 0.9));
    }

    item->SetData(obj);
    item->OnTogglePressed.Connect(this, &SceneStructureWindow::OnTogglePressed);

    ListViewItem info = ListViewItem();
    info.item_ = item;
    info.object_ = obj;
    listItems_.Push(info);

    return item;
}

void SceneStructureWindow::OnTogglePressed(SceneStructureItem *item)
{
    if (listView_.NotNull())
    {
        unsigned index = listView_->FindItem(item->Widget());
        if (index != M_MAX_UNSIGNED)
            listView_->ToggleExpand(index);
    }
}

void SceneStructureWindow::OnItemClicked(StringHash /*eventType*/, VariantMap &eventData)
{
    UIElement *item = dynamic_cast<Text*>(eventData["Item"].GetPtr());
    if (item != NULL)
    {
        int button = eventData[Urho3D::MouseButtonDown::P_BUTTON].GetInt();
        if (button == 1) // LEFT BUTTON
        {
            HideContextMenu();
        }
        else if (button == 4) // RIGHT BUTTON
        {
            ClearSelection();
            listView_->SetSelection(listView_->FindItem(item));
            SceneStructureItem *strucutureItem = FindItem(item);
            Object *data = strucutureItem->Data();
            if (IComponent *c = dynamic_cast<IComponent*>(data))
            {
                SelectComponent(c);
            }
            else if (Entity *e = dynamic_cast<Entity*>(data))
            {
                SelectEntity(e);
            }


            if (strucutureItem != NULL)
            {
                Urho3D::Input* input = GetSubsystem<Urho3D::Input>();
                ShowContextMenu(strucutureItem->Data(), input->GetMousePosition().x_, input->GetMousePosition().y_);
            }
        }
    }
}

void SceneStructureWindow::OnItemDoubleClicked(StringHash /*eventType*/, VariantMap &eventData)
{
    UIElement *item = dynamic_cast<Text*>(eventData["Item"].GetPtr());
    if (item != NULL)
    {
        int button = eventData[Urho3D::MouseButtonDown::P_BUTTON].GetInt();
        if (button == 1) // LEFT BUTTON
        {
            unsigned index = listView_->FindItem(item);
            if (index != M_MAX_UNSIGNED)
                listView_->ToggleExpand(index);
        }
    }
}

void SceneStructureWindow::OnContextMenuHide(StringHash /*eventType*/, VariantMap &/*eventData*/)
{
    HideContextMenu();
}

void SceneStructureWindow::OnSelectionChanged(StringHash /*eventType*/, VariantMap &/*eventData*/)
{
    if (listView_.Null())
        return;

    PODVector<UIElement *> elements = listView_->GetSelectedItems();
}

void SceneStructureWindow::OnComponentDialogClosed(AddComponentDialog *dialog, bool okPressed)
{
    if (okPressed == false || selectedEntities_.Size() == 0 || addComponentDialog_.Null())
        return;

    String componentType = addComponentDialog_->SelectedComponentType();
    String name = addComponentDialog_->Name();
    bool replicated = !addComponentDialog_->IsLocal();
    bool temporary = addComponentDialog_->IsTemporary();

    if (componentType.Length() == 0)
        return;

    for (unsigned int i = 0; i < selectedEntities_.Size(); ++i)
    {
        EntityWeakPtr entity = selectedEntities_[i];
        if (entity.NotNull())
        {
            ComponentPtr comp = entity->CreateComponent(componentType, name, AttributeChange::Default, replicated);
            comp->SetTemporary(temporary);
        }
    }
    RefreshView();
}

void SceneStructureWindow::OnSceneCreated(Scene* /*scene*/, AttributeChange::Type /*change*/)
{
    SceneMap scenes = framework_->Scene()->Scenes();
    if (scenes.Values().Size() > 0)
        SetShownScene(scenes.Values()[0]);
}

void SceneStructureWindow::SetShownScene(Scene *newScene)
{
    scene_ = newScene;
    Clear();
    if (scene_ == NULL)
        return;

    RefreshView();
}

void SceneStructureWindow::OnEntityCreated(Entity* entity, AttributeChange::Type /*change*/)
{
    AddEntity(entity);
}

void SceneStructureWindow::OnComponentCreated(IComponent* component, AttributeChange::Type /*change*/)
{
    AddComponent(component);
}

void SceneStructureWindow::AddEntity(Entity *entity)
{
    String name = entity->Name();
    if (name.Length() == 0)
        name = "(no name)";
    CreateItem(entity, String(entity->Id()) + " " + name, FindItem(entity->ParentScene()));

    HashMap<component_id_t, ComponentPtr>::ConstIterator iter = entity->Components().Begin();
    while (iter != entity->Components().End())
    {
        AddComponent(iter->second_);
        iter.GotoNext();
    }

    entity->ComponentAdded.Disconnect(this, &SceneStructureWindow::OnComponentCreated);
    entity->ComponentAdded.Connect(this, &SceneStructureWindow::OnComponentCreated);
}

void SceneStructureWindow::AddComponent(IComponent *component)
{
    SceneStructureItem *item = FindItem(component->ParentEntity());
    CreateItem(component, component->TypeName(), item);
}

void SceneStructureWindow::AddScene(Scene *scene)
{
    scene_ = scene;
}

void SceneStructureWindow::HideContextMenu()
{
    if (contextMenu_.NotNull())
        contextMenu_->Close();
}

void SceneStructureWindow::ShowContextMenu(Object *obj, int x, int y)
{
    if (contextMenu_.Null())
        return;

    contextMenu_->Clear();
    if (dynamic_cast<IComponent*>(obj) != NULL)
    {
        Menu *m = contextMenu_->CreateItem("removeComponent", "Remove");
        m = contextMenu_->CreateItem("editComponent", "Edit");
    }
    else if (dynamic_cast<Entity*>(obj) != NULL)
    {
        Menu *m = contextMenu_->CreateItem("removeEntity", "Remove");
        m = contextMenu_->CreateItem("addComponent", "New Component");
        m = contextMenu_->CreateItem("editEntity", "Edit");
    }

    contextMenu_->Widget()->SetPosition(x, y);
    contextMenu_->Widget()->BringToFront();
    contextMenu_->Open();
}

void SceneStructureWindow::Clear()
{
    Vector<ListViewItem>::Iterator iter = listItems_.Begin();
    while (iter != listItems_.End())
    {
        iter->item_.Reset();
        iter->object_.Reset();
        iter++;
    }
    listItems_.Clear();

    scene_->EntityCreated.Disconnect(this, &SceneStructureWindow::OnEntityCreated);
}

void SceneStructureWindow::RefreshView()
{
    // TODO optimize the refresh code.
    Clear();
    if (scene_ == NULL)
        return;

    Vector<EntityPtr> entities = scene_->Entities().Values();
    SceneStructureItem *sceneItem = CreateItem(scene_, "Scene");
    sceneItem->SetType(SceneStructureItem::ItemType::Entity);

    for (unsigned i = 0; i < entities.Size(); ++i)
    {
        AddEntity(entities[i]);
    }

    scene_->EntityCreated.Connect(this, &SceneStructureWindow::OnEntityCreated);
}

SceneStructureItem *SceneStructureWindow::FindItem(Object *obj)
{
    for (unsigned i = 0; i < listItems_.Size(); ++i)
    {
        if (listItems_[i].object_ == obj)
            return listItems_[i].item_;
    }
    return NULL;
}

SceneStructureItem *SceneStructureWindow::FindItem(UIElement *element)
{
    for (unsigned i = 0; i < listItems_.Size(); ++i)
    {
        if (listItems_[i].item_->Widget() == element)
            return listItems_[i].item_;
    }
    return NULL;
}

void SceneStructureWindow::OnActionSelected(SceneContextMenu *contextMenu, String id)
{
    if (id == "addComponent")
    {
        for (unsigned int i = 0; i < selectedComponents_.Size(); i++)
            selectedComponents_[i]->ParentEntity()->RemoveComponent(selectedComponents_[i]->TypeName());
    }
    if (id == "removeComponent")
    {
        for (unsigned int i = 0; i < selectedComponents_.Size(); i++)
            selectedComponents_[i]->ParentEntity()->RemoveComponent(selectedComponents_[i]->TypeName());
    }
    else if (id == "removeEntity")
    {
        for (unsigned int i = 0; i < selectedEntities_.Size(); i++)
            selectedEntities_[i]->ParentScene()->RemoveEntity(selectedEntities_[i]->Id());
    }
    else if (id == "addComponent")
    {
        addComponentDialog_->Show();
        IntVector2 pos = window_->GetPosition();
        pos.x_ += 100;
        addComponentDialog_->Widget()->SetPosition(pos);
    }
    RefreshView();
}

bool SceneStructureWindow::ComponentSelected(IComponent *component) const
{
    for (unsigned int i = 0; i < selectedComponents_.Size(); ++i)
    {
        if (component == selectedComponents_[i])
            return true;
    }
    return false;
}

bool SceneStructureWindow::EntitySelected(Entity *entity) const
{
    for (unsigned int i = 0; i < selectedEntities_.Size(); ++i)
    {
        if (entity == selectedEntities_[i])
            return true;
    }
    return false;
}

void SceneStructureWindow::SelectEntity(Entity *entity)
{
    if (!EntitySelected(entity))
    {
        selectedEntities_.Push(EntityWeakPtr(entity));
    }
}

void SceneStructureWindow::SelectComponent(IComponent *component)
{
    if (!ComponentSelected(component))
    {
        selectedComponents_.Push(ComponentWeakPtr(component));
    }
}

void SceneStructureWindow::ClearSelection()
{
    selectedEntities_.Clear();
    selectedComponents_.Clear();
}

}