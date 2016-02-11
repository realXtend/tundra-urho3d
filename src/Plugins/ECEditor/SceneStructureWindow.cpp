// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

#include "SceneStructureWindow.h"
#include "SceneStructureItem.h"
#include "SceneContextMenu.h"
#include "Framework.h"
#include "Scene.h"
#include "SceneAPI.h"

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
            button->SetPosition(IntVector2(-3.0, 0.0));
            topBar->AddChild(button);

            Text *windowHeader = new Text(framework->GetContext());
            windowHeader->SetStyle("Text", style);
            windowHeader->SetName("WindowHeader");
            windowHeader->SetText("Scene Editor");
            windowHeader->SetAlignment(HA_LEFT, VA_CENTER);
            windowHeader->SetPosition(IntVector2(3.0, 0.0));
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

    {
        UIElement *bottomBar = new UIElement(framework->GetContext());
        bottomBar->SetMinHeight(30);
        bottomBar->SetMaxHeight(30);
        bottomBar->SetLayout(LayoutMode::LM_HORIZONTAL, 12, IntRect(12, 2, 12, 2));
        window_->AddChild(bottomBar);

        {
            Button *button = new Button(framework->GetContext());
            button->SetName("UndoButton");
            button->SetStyle("Button", style);
            button->SetMinWidth(50);
            button->SetMaxWidth(50);
            button->SetVerticalAlignment(VA_CENTER);
            bottomBar->AddChild(button);

            Text *text = new Text(framework->GetContext());
            text->SetText("Undo");
            text->SetStyle("Text", style);
            text->SetAlignment(HA_CENTER, VA_CENTER);
            button->AddChild(text);

            button = new Button(framework->GetContext());
            button->SetName("RedoButton");
            button->SetStyle("Button", style);
            button->SetMinWidth(50);
            button->SetMaxWidth(50);
            button->SetVerticalAlignment(VA_CENTER);
            bottomBar->AddChild(button);

            text = new Text(framework->GetContext());
            text->SetText("Redo");
            text->SetStyle("Text", style);
            text->SetAlignment(HA_CENTER, VA_CENTER);
            button->AddChild(text);
        }
    }

    contextMenu_ = new SceneContextMenu(framework->GetContext());
    contextMenu_->Widget()->SetStyle("Window", style);
    
    Menu *m = contextMenu_->CreateItem("newEntity", "New entity...");
    m = contextMenu_->CreateItem("saveScene", "Save scene as...");
    m = contextMenu_->CreateItem("exportScene", "Export...");
    m = contextMenu_->CreateItem("importScene", "Import...");
    m = contextMenu_->CreateItem("openScene", "Open new Scene...");

    contextMenu_->Widget()->SetPosition(IntVector2(100, 100));
    GetSubsystem<UI>()->GetRoot()->AddChild(contextMenu_->Widget());

    framework->Scene()->SceneCreated.Connect(this, &SceneStructureWindow::OnSceneCreated);
}

SceneStructureWindow::~SceneStructureWindow()
{
    for (unsigned i = 0; i < listItems_.Size(); ++i)
    {
        listItems_[i].item_.Reset();
        listItems_[i].object_.Reset();
    }
    listItems_.Clear();

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
    if (dynamic_cast<IComponent*>(obj) != NULL)
        item->SetType(SceneStructureItem::ItemType::Component);
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

void SceneStructureWindow::OnItemClicked(StringHash eventType, VariantMap &eventData)
{
    UIElement *item = dynamic_cast<Text*>(eventData["Item"].GetPtr());
    if (item != NULL)
    {
        int button = eventData[Urho3D::MouseButtonDown::P_BUTTON].GetInt();
        if (button == 1) // LEFT BUTTON
        {
            /*unsigned index = listView_->FindItem(item);
            if (index != M_MAX_UNSIGNED)
                listView_->ToggleSelection(index);*/
            HideContextMenu();
        }
        else if (button == 4) // RIGHT BUTTON
        {
            Urho3D::Input* input = GetSubsystem<Urho3D::Input>();
            contextMenu_->Widget()->SetPosition(input->GetMousePosition().x_, input->GetMousePosition().y_);
            contextMenu_->Widget()->BringToFront();
            contextMenu_->Open();
        }
    }
}

void SceneStructureWindow::OnItemDoubleClicked(StringHash eventType, VariantMap &eventData)
{
    UIElement *item = dynamic_cast<Text*>(eventData["Item"].GetPtr());
    if (item != NULL)
    {
        int button = eventData[Urho3D::MouseButtonDown::P_BUTTON].GetInt();
        if (button == 1)
        {
            unsigned index = listView_->FindItem(item);
            if (index != M_MAX_UNSIGNED)
                listView_->ToggleExpand(index);
        }
        else if (button == 4)
        {

        }
    }
}

void SceneStructureWindow::OnContextMenuHide(StringHash /*eventType*/, VariantMap &/*eventData*/)
{
    HideContextMenu();
}

void SceneStructureWindow::OnSceneCreated(Scene* scene, AttributeChange::Type change)
{
    SceneMap scenes = framework_->Scene()->Scenes();
    LogWarning(String(scenes.Size()));
    if (scenes.Values().Size() > 0)
        SetShownScene(scenes.Values()[0]);
}

void SceneStructureWindow::SetShownScene(Scene *newScene)
{
    LogWarning("Add scene");
    scene_ = newScene;
    if (scene_ == NULL)
        return;

    Vector<EntityPtr> entities = scene_->Entities().Values();
    SceneStructureItem *sceneItem = CreateItem(scene_, "Scene");
    sceneItem->SetType(SceneStructureItem::ItemType::Entity);
    
    //Entity*, AttributeChange::Type> EntityCreated;

    for (unsigned i = 0; i < entities.Size(); ++i)
    {
        AddEntity(entities[i]);
    }

    scene_->EntityCreated.Connect(this, &SceneStructureWindow::OnEntityCreated);
}

void SceneStructureWindow::OnEntityCreated(Entity* entity, AttributeChange::Type change)
{
    AddEntity(entity);
}

void SceneStructureWindow::OnComponentCreated(IComponent* component, AttributeChange::Type change)
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

    entity->ComponentAdded.Connect(this, &SceneStructureWindow::OnComponentCreated);
}

void SceneStructureWindow::AddComponent(IComponent *component)
{
    SceneStructureItem *item = FindItem(component->ParentEntity());
    CreateItem(component, component->TypeName(), item);
}

void SceneStructureWindow::AddScene(Scene *scene)
{

}

void SceneStructureWindow::HideContextMenu()
{
    if (contextMenu_.NotNull())
        contextMenu_->Close();
}

void SceneStructureWindow::ShowContextMenu(int x, int y)
{

}

void SceneStructureWindow::Clear()
{
    
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

}