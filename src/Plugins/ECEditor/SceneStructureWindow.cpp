// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

#include "SceneStructureWindow.h"
#include "SceneStructureItem.h"
#include "SceneContextMenu.h"
#include "Framework.h"
#include "Scene.h"
#include "SceneAPI.h"
#include "AddComponentDialog.h"
#include "AddEntityDialog.h"
#include "SceneAPI.h"
#include "ECEditor.h"

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

#include <Urho3D/UI/FileSelector.h>

using namespace Urho3D;

namespace Tundra
{

SceneStructureWindow::SceneStructureWindow(Framework *framework, IModule *module) :
    Object(framework->GetContext()),
    owner_(module), framework_(framework),
    scene_(0), window_(0),
    listView_(0), contextMenu_(0),
    addComponentDialog_(0)
{
    XMLFile *style = context_->GetSubsystem<ResourceCache>()->GetResource<XMLFile>("Data/UI/DefaultStyle.xml");

    window_ = new Window(framework->GetContext());
    window_->SetLayout(LayoutMode::LM_VERTICAL, 2, IntRect(2, 2, 2, 2));
    window_->SetSize(IntVector2(300, 500));
    window_->SetMinSize(IntVector2(300, 500));
    window_->SetPosition(IntVector2(0, 100));
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
            Button *closeButton = new Button(framework->GetContext());
            closeButton->SetName("CloseButton");
            closeButton->SetStyle("CloseButton", style);
            closeButton->SetAlignment(HA_RIGHT, VA_CENTER);
            closeButton->SetPosition(IntVector2(-3, 0));
            topBar->AddChild(closeButton);

            SubscribeToEvent(closeButton, E_PRESSED, URHO3D_HANDLER(SceneStructureWindow, OnCloseClicked));

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
    listView_->SetName("SceneHierarchyList");
    listView_->SetStyle("HierarchyListView", style);
    //listView_->SetMultiselect(true);
    listView_->SetEnabled(true);
    listView_->SetFocusMode(FocusMode::FM_FOCUSABLE_DEFOCUSABLE);
    window_->AddChild(listView_);

    SubscribeToEvent(listView_, E_ITEMDOUBLECLICKED, URHO3D_HANDLER(SceneStructureWindow, OnItemDoubleClicked));
    SubscribeToEvent(listView_, E_ITEMCLICKED, URHO3D_HANDLER(SceneStructureWindow, OnItemClicked));
    SubscribeToEvent(listView_, E_SELECTIONCHANGED, URHO3D_HANDLER(SceneStructureWindow, OnSelectionChanged));
    SubscribeToEvent(E_UIMOUSECLICK, URHO3D_HANDLER(SceneStructureWindow, OnElementClicked));

    contextMenu_ = new SceneContextMenu(framework->GetContext());
    contextMenu_->Widget()->SetPosition(IntVector2(100, 100));
    contextMenu_->OnActionSelected.Connect(this, &SceneStructureWindow::OnActionSelected);
    GetSubsystem<UI>()->GetRoot()->AddChild(contextMenu_->Widget());

    addComponentDialog_ = AddComponentDialogPtr(new AddComponentDialog(framework_));
    addComponentDialog_->DialogClosed.Connect(this, &SceneStructureWindow::OnComponentDialogClosed);
    addComponentDialog_->Hide();

    addEntityDialog_ = AddEntityDialogPtr(new AddEntityDialog(framework_));
    addEntityDialog_->DialogClosed.Connect(this, &SceneStructureWindow::OnEntityDialogClosed);
    addEntityDialog_->Hide();
}

SceneStructureWindow::~SceneStructureWindow()
{
    Clear();
    addComponentDialog_.Reset();
    addEntityDialog_.Reset();
    contextMenu_.Reset();

    if (window_.Get())
        window_->Remove();
    window_.Reset();

    scene_.Reset();
}

SceneStructureItem *SceneStructureWindow::CreateItem(Object *obj, const String &text, SceneStructureItem *parent)
{
    SceneStructureItem *item = new SceneStructureItem(context_, listView_, obj);
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
    item->OnTogglePressed.Connect(this, &SceneStructureWindow::OnTogglePressed);

    ListViewItem info = ListViewItem();
    info.item_ = item;
    info.object_ = obj;
    listItems_.Push(info);

    return item;
}

void SceneStructureWindow::OnTogglePressed(SceneStructureItem *item)
{
    ToggleItem(item);
}

void SceneStructureWindow::OnElementClicked(StringHash /*eventType*/, VariantMap &eventData)
{
    if (eventData[Urho3D::MouseButtonDown::P_BUTTON].GetInt() != 4)
        return;

    UIElement *element = dynamic_cast<UIElement*>(eventData["Element"].GetPtr());
    if (element != NULL && element->GetParent() != NULL)
        element = element->GetParent();

    if (element == listView_)
    {
        ClearSelection();
        Urho3D::Input* input = GetSubsystem<Urho3D::Input>();
        ShowContextMenu(element, input->GetMousePosition().x_, input->GetMousePosition().y_);
    }
}

void SceneStructureWindow::OnItemClicked(StringHash /*eventType*/, VariantMap &eventData)
{
    UIElement *item = dynamic_cast<UIElement*>(eventData["Item"].GetPtr());
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
                SelectComponent(c);
            else if (Entity *e = dynamic_cast<Entity*>(data))
                SelectEntity(e);


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
            ToggleItem(FindItem(item));
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

void SceneStructureWindow::OnCloseClicked(StringHash /*eventType*/, VariantMap &/*eventData*/)
{
    Hide();
}

void SceneStructureWindow::OnComponentDialogClosed(AddComponentDialog *dialog, bool confirmed)
{
    if (confirmed == false || selectedEntities_.Size() == 0 || addComponentDialog_.Null())
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
    //dirty_ = true;
}

void SceneStructureWindow::OnEntityDialogClosed(AddEntityDialog *dialog, bool confirmed)
{
    if (confirmed == false || addEntityDialog_.Null())
        return;

    String name = addEntityDialog_->Name();
    bool replicated = !addEntityDialog_->IsLocal();
    bool temporary = addEntityDialog_->IsTemporary();

    Scene *scene = framework_->Scene()->MainCameraScene();
    Entity *entity = 0;
    if (replicated)
        entity = scene->CreateEntity();
    else
        entity = scene->CreateLocalEntity();
    entity->SetTemporary(temporary);
    entity->SetName(name);

    //dirty_ = true;
}

void SceneStructureWindow::SetShownScene(Scene *newScene)
{
    if (scene_)
    {
        scene_->ComponentAdded.Disconnect(this, &SceneStructureWindow::OnComponentAdded);
        scene_->ComponentRemoved.Disconnect(this, &SceneStructureWindow::OnComponentRemoved);
        scene_->EntityCreated.Disconnect(this, &SceneStructureWindow::OnEntityAdded);
        scene_->EntityRemoved.Disconnect(this, &SceneStructureWindow::OnEntityRemoved);
        scene_->AttributeChanged.Disconnect(this, &SceneStructureWindow::OnAttributeChanged);
    }

    scene_ = newScene;

    if (scene_)
    {
        scene_->ComponentAdded.Connect(this, &SceneStructureWindow::OnComponentAdded);
        scene_->ComponentRemoved.Connect(this, &SceneStructureWindow::OnComponentRemoved);
        scene_->EntityCreated.Connect(this, &SceneStructureWindow::OnEntityAdded);
        scene_->EntityRemoved.Connect(this, &SceneStructureWindow::OnEntityRemoved);
        scene_->AttributeChanged.Connect(this, &SceneStructureWindow::OnAttributeChanged);
    }

    Clear();
    dirty_ = true;
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
    SceneStructureItem *entityItem = FindItem(entity);
    // Entity already added
    if (entityItem)
        return;

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

void SceneStructureWindow::RemoveEntity(Entity *entity)
{
    SceneStructureItem *entityItem = FindItem(entity);
    if (entityItem)
        RemoveItem(entityItem);
}

void SceneStructureWindow::AddComponent(IComponent *component)
{
    SceneStructureItem *comp = FindItem(component);
    // Component already added
    if (comp)
        return;

    SceneStructureItem *entityItem = FindItem(component->ParentEntity());
    if (entityItem)
        CreateItem(component, component->TypeName(), entityItem);
}

void SceneStructureWindow::RemoveComponent(IComponent *component)
{
    SceneStructureItem *comp = FindItem(component);
    // If found remove component from list.
    if (comp)
    {
        RemoveItem(comp);
    }
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
        m = contextMenu_->CreateItem("copyComponent", "Copy");
        m = contextMenu_->CreateItem("pasteComponent", "Paste");
        //m = contextMenu_->CreateItem("editComponent", "Edit");
    }
    else if (dynamic_cast<Entity*>(obj) != NULL)
    {
        Menu *m = contextMenu_->CreateItem("removeEntity", "Remove");
        m = contextMenu_->CreateItem("addEntity", "New Entity...");
        m = contextMenu_->CreateItem("addComponent", "New Component");
        m = contextMenu_->CreateItem("editEntity", "Edit");
        m = contextMenu_->CreateItem("copyEntity", "Copy");
        m = contextMenu_->CreateItem("pasteEntity", "Paste");
    }
    else
    {
        Menu *m = contextMenu_->CreateItem("addEntity", "New Entity...");
    }

    contextMenu_->Widget()->SetPosition(x, y);
    contextMenu_->Widget()->BringToFront();
    contextMenu_->Open();
}

void SceneStructureWindow::EditSelection()
{
    if (selectedEntities_.Size() == 0)
        return;

    ECEditor *editor = dynamic_cast<ECEditor*>(owner_.Get());
    if (editor)
    {
        editor->OpenEntityEditor(selectedEntities_[0]);
    }
}

void SceneStructureWindow::OnComponentRemoved(Entity *entity, IComponent *component, AttributeChange::Type change)
{
    SceneStructureItem *comp = FindItem(component);
    // If found remove component from list.
    if (comp)
    {
        RemoveItem(comp);
    }
}

void SceneStructureWindow::OnComponentAdded(Entity *entity, IComponent *component, AttributeChange::Type change)
{
    SceneStructureItem *comp = FindItem(component);
    // Component already added
    if (comp)
        return;

    SceneStructureItem *entityItem = FindItem(entity);
    if (entityItem)
        CreateItem(component, component->TypeName(), entityItem);
}

void SceneStructureWindow::OnEntityRemoved(Entity *entity, AttributeChange::Type change)
{
    RemoveEntity(entity);
}

void SceneStructureWindow::OnEntityAdded(Entity *entity, AttributeChange::Type change)
{
    AddEntity(entity);
}

void SceneStructureWindow::OnAttributeChanged(IComponent *component, IAttribute *attribute, AttributeChange::Type type)
{
    if (component != 0 && component->TypeId() == 26)
        dirty_ = true;
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

    selectedComponents_.Clear();
    selectedEntities_.Clear();

    if (scene_ != NULL)
        scene_->EntityCreated.Disconnect(this, &SceneStructureWindow::OnEntityCreated);
}

void SceneStructureWindow::RefreshView()
{
    dirty_ = false;

    // TODO optimize the refresh code.
    Clear();

    if (scene_ == NULL)
        return;

    Vector<EntityPtr> entities = scene_->Entities().Values();
    SceneStructureItem *sceneItem = CreateItem(scene_, "Scene");

    for (unsigned i = 0; i < entities.Size(); ++i)
        AddEntity(entities[i]);

    scene_->EntityCreated.Disconnect(this, &SceneStructureWindow::OnEntityCreated);
    scene_->EntityCreated.Connect(this, &SceneStructureWindow::OnEntityCreated);
}

void SceneStructureWindow::Hide()
{
    if (window_.Get())
        window_->SetVisible(false);
    HideContextMenu();
}

void SceneStructureWindow::Show()
{
    if (window_.Get())
        window_->SetVisible(true);
    dirty_ = true;
}

void SceneStructureWindow::Update()
{
    if (dirty_ && window_.Get() && window_->IsVisible())
        RefreshView();
}

SceneStructureItem *SceneStructureWindow::FindItem(Object *obj)
{
    for (unsigned i = 0; i < listItems_.Size(); ++i)
    {
        if (listItems_[i].object_ == obj)
            return listItems_[i].item_;
    }
    return 0;
}

void SceneStructureWindow::ToggleItem(SceneStructureItem *item)
{
    if (listView_.Get())
    {
        unsigned index = listView_->FindItem(item->Widget());
        if (index != M_MAX_UNSIGNED)
            listView_->ToggleExpand(index);
        item->Refresh();
    }
}

void SceneStructureWindow::Copy(Entity *entity)
{
    if (entity)
    {
        Urho3D::XMLFile entityXml(context_);
        Urho3D::XMLElement base = entityXml.CreateRoot("scene");
        Urho3D::XMLElement entityElement = base.CreateChild("entity");
        entityElement.SetAttribute("id", String(entity->Id()));

        const Entity::ComponentMap &components = entity->Components();
        for (Entity::ComponentMap::ConstIterator i = components.Begin(); i != components.End(); ++i)
            i->second_->SerializeTo(entityXml, entityElement);

        GetSubsystem<Urho3D::UI>()->SetClipboardText(entityXml.ToString());
    }
}

void SceneStructureWindow::Copy(IComponent *component)
{
    if (component)
    {
        Urho3D::XMLFile entityXml(context_);
        Urho3D::XMLElement base = entityXml.CreateRoot("entity");
        base.SetAttribute("id", String(component->ParentEntity()->Id()));

        const Entity::ComponentMap &components = component->ParentEntity()->Components();
        for (Entity::ComponentMap::ConstIterator i = components.Begin(); i != components.End(); ++i)
        {
            if (i->second_ == component)
                i->second_->SerializeTo(entityXml, base);
        }
        GetSubsystem<Urho3D::UI>()->SetClipboardText(entityXml.ToString());
    }
}

void SceneStructureWindow::Paste()
{
    String clipboard = GetSubsystem<Urho3D::UI>()->GetClipboardText();
    if (!scene_ || clipboard.Empty())
        return;

    Urho3D::XMLFile xmlData(context_);
    if (!xmlData.FromString(clipboard))
    {
        /// @todo print error message.
        return;
    }

    Urho3D::XMLElement root = xmlData.GetRoot();
    if (root.GetName() == "scene")
    {
        scene_->CreateContentFromXml(xmlData, false, Tundra::AttributeChange::Default);
    }
    else if (root.GetName() == "entity")
    {
        if (selectedEntities_.Size() == 0 &&
            selectedComponents_.Size() == 0)
            return;

        String id = root.GetAttribute("id");
        if (id.Empty())
        {
            /// @todo print error message.
            return;
        }

        Entity *originalEntity = scene_->EntityById(ToInt(id));
        if (!originalEntity)
        {
            /// @todo print error message.
            return;
        }

        Urho3D::XMLElement componentElement = root.GetChild("component");
        if (!componentElement)
        {
            /// @todo print error message.
            return;
        }
        String typeName = componentElement.GetAttribute("type");

        Entity *entity;
        if (selectedEntities_.Size() > 0)
            entity = selectedEntities_[0];
        else
            entity = selectedComponents_[0]->ParentEntity();

        const Entity::ComponentMap &components = originalEntity->Components();
        for (Entity::ComponentMap::ConstIterator i = components.Begin(); i != components.End(); ++i)
        {
            if (i->second_->GetTypeName() != typeName)
                continue;

            ComponentPtr component = entity->GetOrCreateComponent(i->second_->TypeId(), i->second_->Name(), AttributeChange::Default);
            const AttributeVector &attributes = i->second_->Attributes();
            for (size_t j = 0; j < attributes.Size(); j++)
            {
                if (attributes[j])
                {
                    IAttribute *attribute = component->AttributeByName(attributes[j]->Name());
                    if (attribute)
                        attribute->FromString(attributes[j]->ToString(), AttributeChange::Default);
                }
            }
        }
    }

    dirty_ = true;
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

void SceneStructureWindow::RemoveItem(SceneStructureItem *item)
{
    int index = -1;
    for (uint i = 0; listItems_.Size(); ++i)
    {
        if (listItems_[i].item_ == item)
        {
            index = i;
            break;
        }
    }

    if (index > -1)
        listItems_.Remove(listItems_[index]);
}

void SceneStructureWindow::OnActionSelected(SceneContextMenu *contextMenu, String id)
{
    if (id == "addEntity")
    {
        addEntityDialog_->Show();
        IntVector2 pos = window_->GetPosition();
        pos.x_ += 100;
        addEntityDialog_->Widget()->SetPosition(pos);
    }
    else if (id == "removeEntity")
    {
        for (unsigned int i = 0; i < selectedEntities_.Size(); i++)
            selectedEntities_[i]->ParentScene()->RemoveEntity(selectedEntities_[i]->Id());
    }
    else if (id == "editEntity")
    {
        EditSelection();
    }
    else if (id == "copyEntity")
    {
        for (uint i = 0; i < selectedEntities_.Size(); i++)
            Copy(selectedEntities_[i]);
    }
    else if (id == "pasteEntity")
    {
        Paste();
    }
    else if (id == "addComponent")
    {
        addComponentDialog_->Show();
        IntVector2 pos = window_->GetPosition();
        pos.x_ += 100;
        addComponentDialog_->Widget()->SetPosition(pos);
    }
    if (id == "removeComponent")
    {
        for (unsigned int i = 0; i < selectedComponents_.Size(); i++)
            selectedComponents_[i]->ParentEntity()->RemoveComponent(selectedComponents_[i]->TypeName());
        //dirty_ = true;
    }
    else if (id == "copyComponent")
    {
        for (uint i = 0; i < selectedComponents_.Size(); i++)
            Copy(selectedComponents_[i]);
    }
    else if (id == "pasteComponent")
    {
        Paste();
    }
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