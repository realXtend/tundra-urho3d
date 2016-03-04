// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

#include "AddComponentDialog.h"
#include "Framework.h"
#include "SceneAPI.h"

#include <Urho3D/Core/Context.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Engine/Engine.h>

#include "LoggingFunctions.h"

#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/DropDownList.h>
#include <Urho3D/UI/LineEdit.h>
#include <Urho3D/UI/Window.h>
#include <Urho3D/UI/CheckBox.h>

namespace Tundra
{

AddComponentDialog::AddComponentDialog(Framework *framework) :
    Object(framework->GetContext()),
    framework_(framework)
{
    XMLFile *style = context_->GetSubsystem<ResourceCache>()->GetResource<XMLFile>("Data/UI/DefaultStyle.xml");

    window_ = new Window(framework->GetContext());
    window_->SetLayout(LayoutMode::LM_VERTICAL, 8, IntRect(8, 8, 8, 8));
    window_->SetSize(IntVector2(340, 170));
    window_->SetMinSize(IntVector2(340, 170));
    window_->SetMovable(true);
    window_->SetStyle("Window", style);
    GetSubsystem<UI>()->GetRoot()->AddChild(window_);

    {
        // TOPBAR
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

            SubscribeToEvent(button, E_RELEASED, URHO3D_HANDLER(AddComponentDialog, OnButtonPressed));

            Text *windowHeader = new Text(framework->GetContext());
            windowHeader->SetStyle("Text", style);
            windowHeader->SetName("WindowHeader");
            windowHeader->SetText("Add New Component");
            windowHeader->SetAlignment(HA_LEFT, VA_CENTER);
            windowHeader->SetPosition(IntVector2(3, 0));
            topBar->AddChild(windowHeader);
        }
    }

    UIElement *content = new UIElement(framework->GetContext());
    content->SetStyle("Window", style);
    content->SetLayout(LayoutMode::LM_VERTICAL, 2, IntRect(2, 2, 2, 2));
    window_->AddChild(content);

    {
        UIElement *contentOne = new UIElement(framework->GetContext());
        contentOne->SetMinHeight(22);
        contentOne->SetMaxHeight(22);
        content->AddChild(contentOne);

        {
            Text *label = new Text(framework->GetContext());
            label->SetStyle("Text", style);
            label->SetName("Label");
            label->SetText("Component");
            label->SetAlignment(HA_LEFT, VA_CENTER);
            label->SetPosition(IntVector2(12, 0));
            contentOne->AddChild(label);

            dropDownList_ = DropDownListPtr(new DropDownList(framework->GetContext()));
            dropDownList_->SetStyle("DropDownList", style);
            dropDownList_->SetName("ComponentDropDownList");
            dropDownList_->SetAlignment(HA_RIGHT, VA_CENTER);
            dropDownList_->SetPosition(IntVector2(-16, 0));
            dropDownList_->SetSize(IntVector2(182, 22));
            dropDownList_->SetResizePopup(true);
            contentOne->AddChild(dropDownList_);

            SceneAPI* sceneAPI = framework_->Scene();
            StringVector componentTypes = sceneAPI->ComponentTypes();
            for (unsigned int i = 0; i < componentTypes.Size(); ++i)
            {
                String componentType = componentTypes[i];
                Text *label = new Text(framework->GetContext());
                label->SetStyle("FileSelectorListText", style);
                label->SetName(componentType);
                label->SetText(componentType);

                dropDownList_->AddItem(label);
            }

        }

        UIElement *contentTwo = new UIElement(framework->GetContext());
        contentTwo->SetMinHeight(22);
        contentTwo->SetMaxHeight(22);
        content->AddChild(contentTwo);

        {
            Text *label = new Text(framework->GetContext());
            label->SetStyle("Text", style);
            label->SetName("nameLabel");
            label->SetText("Name");
            label->SetAlignment(HA_LEFT, VA_CENTER);
            label->SetPosition(IntVector2(12, 0));
            contentTwo->AddChild(label);

            nameLineEdit_ = LineEditPtr(new LineEdit(framework->GetContext()));
            nameLineEdit_->SetStyle("LineEdit", style);
            nameLineEdit_->SetAlignment(HA_RIGHT, VA_CENTER);
            nameLineEdit_->SetPosition(IntVector2(-16, 0));
            nameLineEdit_->SetSize(IntVector2(182, 22));
            contentTwo->AddChild(nameLineEdit_);
        }

        UIElement *contentThree = new UIElement(framework->GetContext());
        contentThree->SetMinHeight(22);
        contentThree->SetMaxHeight(22);
        content->AddChild(contentThree);

        {
            Text *label = new Text(framework->GetContext());
            label->SetStyle("Text", style);
            label->SetName("localLabel");
            label->SetText("Local");
            label->SetAlignment(HA_LEFT, VA_CENTER);
            label->SetPosition(IntVector2(12, 0));
            contentThree->AddChild(label);

            UIElement *localArea = new UIElement(framework->GetContext());
            localArea->SetMinHeight(22);
            localArea->SetMaxHeight(22);
            localArea->SetAlignment(HA_RIGHT, VA_CENTER);
            localArea->SetPosition(IntVector2(-26, 0));
            localArea->SetSize(IntVector2(172, 22));
            contentThree->AddChild(localArea);

            {
                localCheckBox_ = CheckBoxPtr(new CheckBox(framework->GetContext()));
                localCheckBox_->SetStyle("CheckBox", style);
                localCheckBox_->SetName("localCheckBox");
                localCheckBox_->SetAlignment(HA_LEFT, VA_CENTER);
                localArea->AddChild(localCheckBox_);

                SubscribeToEvent(localCheckBox_, E_TOGGLED, URHO3D_HANDLER(AddComponentDialog, OnCheckboxChanged));

                localText_ = TextPtr(new Text(framework->GetContext()));
                localText_->SetStyle("Text", style);
                localText_->SetText("Creating as Replicated");
                localText_->SetAlignment(HA_LEFT, VA_CENTER);
                localText_->SetPosition(IntVector2(22, 0));
                localArea->AddChild(localText_);
            }
        }

        UIElement *contentFour = new UIElement(framework->GetContext());
        contentFour->SetMinHeight(22);
        contentFour->SetMaxHeight(22);
        content->AddChild(contentFour);

        {
            Text *label = new Text(framework->GetContext());
            label->SetStyle("Text", style);
            label->SetName("TemporaryLabel");
            label->SetText("Temporary");
            label->SetAlignment(HA_LEFT, VA_CENTER);
            label->SetPosition(IntVector2(12, 0));
            contentFour->AddChild(label);

            UIElement *temporaryArea = new UIElement(framework->GetContext());
            temporaryArea->SetMinHeight(22);
            temporaryArea->SetMaxHeight(22);
            temporaryArea->SetAlignment(HA_RIGHT, VA_CENTER);
            temporaryArea->SetPosition(IntVector2(-26, 0));
            temporaryArea->SetSize(IntVector2(172, 22));
            contentFour->AddChild(temporaryArea);

            {
                temporaryCheckBox_ = CheckBoxPtr(new CheckBox(framework->GetContext()));
                temporaryCheckBox_->SetStyle("CheckBox", style);
                temporaryCheckBox_->SetName("temporaryCheckBox");
                temporaryCheckBox_->SetAlignment(HA_LEFT, VA_CENTER);
                temporaryArea->AddChild(temporaryCheckBox_);
            }
        }
    }

    {
        UIElement *bottomBar = new UIElement(framework->GetContext());
        bottomBar->SetMinHeight(30);
        bottomBar->SetMaxHeight(30);
        bottomBar->SetLayoutSpacing(12);
        bottomBar->SetLayoutBorder(IntRect(2, 2, 2, 2));
        bottomBar->SetLayout(LayoutMode::LM_HORIZONTAL, 8, IntRect(16, 2, 16, 2));
        window_->AddChild(bottomBar);

        {
            Button *button = new Button(framework->GetContext());
            button->SetName("AddButton");
            Text *text = new Text(framework->GetContext());
            text->SetText("Add");
            text->SetAlignment(HA_CENTER, VA_CENTER);
            text->SetInternal(true);

            button->AddChild(text);
            button->SetStyle("Button", style);
            button->SetAlignment(HA_RIGHT, VA_CENTER);
            bottomBar->AddChild(button);

            SubscribeToEvent(button, E_RELEASED, URHO3D_HANDLER(AddComponentDialog, OnButtonPressed));

            button = new Button(framework->GetContext());
            button->SetName("CancelButton");
            text = new Text(framework->GetContext());
            text->SetText("Cancel");
            text->SetAlignment(HA_CENTER, VA_CENTER);
            text->SetInternal(true);

            button->AddChild(text);
            button->SetStyle("Button", style);
            button->SetAlignment(HA_LEFT, VA_CENTER);
            bottomBar->AddChild(button);

            SubscribeToEvent(button, E_RELEASED, URHO3D_HANDLER(AddComponentDialog, OnButtonPressed));
        }
    }

    GetSubsystem<UI>()->SetFocusElement(nameLineEdit_);
}

AddComponentDialog::~AddComponentDialog()
{
    dropDownList_.Reset();
    nameLineEdit_.Reset();
    localText_.Reset();

    if (window_.NotNull())
        window_->Remove();
    window_.Reset();
}

UIElement *AddComponentDialog::Widget() const
{
    return window_;
}

String AddComponentDialog::Name() const
{
    if (nameLineEdit_)
        return nameLineEdit_->GetText();
    return "";
}

String AddComponentDialog::SelectedComponentType() const
{
    UIElement *element = dropDownList_->GetSelectedItem();
    if (element != NULL)
        return element->GetName();
    return "";
}

bool AddComponentDialog::IsTemporary() const
{
    return temporaryCheckBox_->IsChecked();
}

bool AddComponentDialog::IsLocal() const
{
    return localCheckBox_->IsChecked();
}

void AddComponentDialog::Show()
{
    window_->SetVisible(true);
    window_->SetModal(true);
}

void AddComponentDialog::Hide()
{
    window_->SetVisible(false);
    window_->SetModal(false);
}

void AddComponentDialog::OnButtonPressed(StringHash /*eventType*/, VariantMap &eventData)
{
    Hide();

    UIElement *element = dynamic_cast<UIElement*>(eventData["Element"].GetPtr());
    if (element == NULL)
        return;

    if (element->GetName() == "AddButton")
        DialogClosed.Emit(this, true);
    else if (element->GetName() == "CancelButton" || element->GetName() == "CloseButton")
        DialogClosed.Emit(this, false);
}

void AddComponentDialog::OnCheckboxChanged(StringHash /*eventType*/, VariantMap &eventData)
{
    CheckBox *check = dynamic_cast<CheckBox*>(eventData["Element"].GetPtr());
    if (check != NULL && check->GetName() == "localCheckBox")
    {
        bool checked = check->IsChecked();
        String t = checked ? "Creating as Local" : "Creating as Replicated";
        localText_->SetText(t);
    }
}
}