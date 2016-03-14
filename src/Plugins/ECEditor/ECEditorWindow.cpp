// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

#include "ECEditorWindow.h"
#include "Framework.h"
#include "Scene.h"
#include "SceneAPI.h"
#include "AttributeEditor.h"
#include "Math/Color.h"

#include <Urho3D/Core/Context.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/Resource/ResourceCache.h>

#include "LoggingFunctions.h"

#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIElement.h>
#include <Urho3D/UI/ListView.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/Window.h>

using namespace Urho3D;

namespace Tundra
{

ComponentContainer::ComponentContainer(Framework *framework, ComponentPtr component, int index) :
    Object(framework->GetContext()),
    framework_(framework),
    index_(index)
{
    XMLFile *style = context_->GetSubsystem<ResourceCache>()->GetResource<XMLFile>("Data/UI/DefaultStyle.xml");

    window_ = new Window(framework->GetContext());
    window_->SetLayout(LayoutMode::LM_VERTICAL, 2, IntRect(8, 8, 8, 8));
    window_->SetStyle("Window", style);
    window_->SetMovable(false);

    {
        UIElement *topBar = new UIElement(framework->GetContext());
        topBar->SetMinHeight(22);
        topBar->SetMaxHeight(22);
        window_->AddChild(topBar);

        {
            header_ = new Text(framework->GetContext());
            header_->SetStyle("Text", style);
            header_->SetName("WindowHeader");
            header_->SetText("Transform");
            header_->SetAlignment(HA_LEFT, VA_CENTER);
            header_->SetPosition(IntVector2(3, 0));
            topBar->AddChild(header_);
        }
    }

    UIElement *spacer = new UIElement(framework->GetContext());
    spacer->SetStyle("EditorSeparator", style);
    window_->AddChild(spacer);

    attributeContainer_ = new UIElement(framework->GetContext());
    attributeContainer_->SetLayout(LayoutMode::LM_VERTICAL, 2, IntRect(2, 2, 2, 2));
    window_->AddChild(attributeContainer_);

    {
        AttributeVector attributes = component->Attributes();
        for (unsigned int i = 0; i < attributes.Size(); ++i)
        {
            IAttributeEditor *editor = CreateAttributeEditor(framework_, attributes[i]);
            if (editor)
            {
                attributeEditors_[i] = editor;
                attributeContainer_->AddChild(editor->Widget());
            }
        }
    }

    window_->SetEnabled(false);
}

ComponentContainer::~ComponentContainer()
{
    for (unsigned int i = 0; i < attributeEditors_.Values().Size(); ++i)
        attributeEditors_.Values()[i].Reset();
}

void ComponentContainer::SetTitleText(const String &text)
{
    if (header_)
        header_->SetText(text);
}

String ComponentContainer::TitleText() const
{
    if (header_)
        return header_->GetText();
    return "";
}

UIElement *ComponentContainer::Widget() const
{
    return window_;
}

int ComponentContainer::Index() const
{
    return index_;
}

IAttributeEditor *ComponentContainer::CreateAttributeEditor(Framework *framework, IAttribute *attribute)
{
    if (attribute == NULL)
        return NULL;

    AttributeWeakPtr attributeWeakPtr = AttributeWeakPtr(attribute->Owner(), attribute);
    IAttributeEditor *editor = 0;
    u32 type = attribute->TypeId();
    switch (type)
    {
    case IAttribute::TypeId::StringId:
        editor = new AttributeEditor<String>(framework, attributeWeakPtr);
        break;
    case IAttribute::TypeId::Float3Id:
        editor = new AttributeEditor<float3>(framework, attributeWeakPtr);
        break;
    case IAttribute::TypeId::ColorId:
        editor = new AttributeEditor<Tundra::Color>(framework, attributeWeakPtr);
        break;
    case IAttribute::TypeId::BoolId:
        editor = new AttributeEditor<bool>(framework, attributeWeakPtr);
        break;
    case IAttribute::TypeId::RealId:
        editor = new AttributeEditor<float>(framework, attributeWeakPtr);
        break;
    case IAttribute::TypeId::IntId:
        editor = new AttributeEditor<int>(framework, attributeWeakPtr);
        break;
    case IAttribute::TypeId::TransformId:
        editor = new AttributeEditor<Transform>(framework, attributeWeakPtr);
        break;
	case IAttribute::TypeId::EntityReferenceId:
		editor = new AttributeEditor<EntityReference>(framework, attributeWeakPtr);
		break;
	case IAttribute::TypeId::AssetReferenceId:
		editor = new AttributeEditor<AssetReference>(framework, attributeWeakPtr);
		break;
    }

    if (editor != NULL)
        editor->SetTitle(attribute->Name());

    return editor;
}

ECEditorWindow::ECEditorWindow(Framework *framework) :
    Object(framework->GetContext()),
    framework_(framework)
{
    XMLFile *style = context_->GetSubsystem<ResourceCache>()->GetResource<XMLFile>("Data/UI/DefaultStyle.xml");

    window_ = new Window(framework->GetContext());
    window_->SetLayout(LayoutMode::LM_VERTICAL, 2, IntRect(2, 2, 2, 2));
    window_->SetSize(IntVector2(350, 500));
    window_->SetMinSize(IntVector2(350, 500));
    window_->SetStyle("Window", style);
    window_->SetMovable(true);
    window_->SetResizable(true);
    GetSubsystem<UI>()->GetRoot()->AddChild(window_);

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

            SubscribeToEvent(E_PRESSED, URHO3D_HANDLER(ECEditorWindow, OnCloseClicked));

            Text *windowHeader = new Text(framework->GetContext());
            windowHeader->SetStyle("Text", style);
            windowHeader->SetName("WindowHeader");
            windowHeader->SetText("Attribute Inspector");
            windowHeader->SetAlignment(HA_LEFT, VA_CENTER);
            windowHeader->SetPosition(IntVector2(3, 0));
            topBar->AddChild(windowHeader);
        }
    }

    list_ = new ListView(framework->GetContext());
    list_->SetName("HierarchyList");
    list_->SetHighlightMode(HighlightMode::HM_ALWAYS);
    list_->SetStyle("ListView", style);
    window_->AddChild(list_);
}

ECEditorWindow::~ECEditorWindow()
{
    Clear();
    if (window_)
        window_->Remove();
    window_.Reset();
    entity_.Reset();
}

void ECEditorWindow::Show()
{
    window_->SetVisible(true);
}

void ECEditorWindow::Hide()
{
    window_->SetVisible(false);
}

void ECEditorWindow::AddEntity(EntityPtr entity, bool updateUi)
{
    if (!entity)
        return;

    entity_ = EntityWeakPtr(entity);
    if (updateUi)
        Refresh();
}

void ECEditorWindow::AddEntity(entity_id_t id, bool updateUi)
{
    EntityPtr entity = framework_->Scene()->MainCameraScene()->EntityById(id);
    if (entity != NULL)
        AddEntity(entity, updateUi);
}

void ECEditorWindow::Clear()
{
    ComponentContainerPtr comp;
    for (unsigned int i = 0; i < containers_.Values().Size(); ++i)
    {
        comp = containers_.Values()[i];
        list_->RemoveItem(comp->Widget());
        comp.Reset();
    }
    containers_.Clear();
}

void ECEditorWindow::Refresh()
{
    Clear();

    if (!entity_.Get())
        return;
    
    Entity::ComponentMap components = entity_->Components();
    ComponentContainer *container = 0;
    ComponentPtr comp;
    for (unsigned int i = 0; i < components.Values().Size(); ++i)
    {
        comp = components.Values()[i];
        if (comp == NULL)
            continue;

        //int index = GetComponentIndex(comp);

        int index = 6400;
        if (comp->TypeName() == "Name")
            index = 0;
        else if (comp->TypeName() == "Placeable")
            index = 1;

        container = new ComponentContainer(framework_, comp, index);
        container->SetTitleText(comp->GetTypeName());

        list_->InsertItem(index, container->Widget());
        containers_[comp] = container;
    }
}

UIElement *ECEditorWindow::Widget() const
{
    return window_;
}

void ECEditorWindow::OnCloseClicked(StringHash /*eventType*/, VariantMap &/*eventData*/)
{
    Hide();
}

}