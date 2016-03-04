// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

#include "AttributeEditor.h"
#include "Framework.h"

#include <Urho3D/Core/Context.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Resource/ResourceCache.h>

#include "LoggingFunctions.h"

#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIElement.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/CheckBox.h>
#include <Urho3D/UI/LineEdit.h>

namespace Tundra
{

IAttributeEditor::IAttributeEditor(Framework *framework) :
    Object(framework->GetContext()),
    framework_(framework)
{
}

IAttributeEditor::~IAttributeEditor()
{
    if (root_)
        root_->Remove();
    root_.Reset();
}

const String &IAttributeEditor::Title() const
{
    if (title_)
        title_->GetText();
    return "";
}

void IAttributeEditor::SetTitle(const String &text)
{
    if (title_)
        title_->SetText(text);
}

UIElementPtr IAttributeEditor::Widget() const
{
    return root_;
}

void IAttributeEditor::PreInitialize()
{

}

void IAttributeEditor::Initialize()
{
    XMLFile *style = context_->GetSubsystem<ResourceCache>()->GetResource<XMLFile>("Data/UI/DefaultStyle.xml");

    root_ = new UIElement(framework_->GetContext());
    root_->SetHeight(22);
    root_->SetLayout(LayoutMode::LM_HORIZONTAL, 2, IntRect(2, 0, 2, 0));

    title_ = new Text(framework_->GetContext());
    title_->SetMinWidth(90);
    title_->SetMaxWidth(90);
    title_->SetStyle("Text", style);
    root_->AddChild(title_);
}

void IAttributeEditor::Update()
{

}

//----------------------------BOOL ATTRIBUTE TYPE----------------------------

template<> void AttributeEditor<bool>::SetValue(const bool &value)
{
    value_ = value;
    Update();
}

template<> const bool &AttributeEditor<bool>::Value() const
{
    return value_.GetBool();
}

template<> void AttributeEditor<bool>::Initialize()
{
    IAttributeEditor::Initialize();
    XMLFile *style = context_->GetSubsystem<ResourceCache>()->GetResource<XMLFile>("Data/UI/DefaultStyle.xml");

    CheckBox *checkBox = new CheckBox(framework_->GetContext());
    checkBox->SetStyle("CheckBox", style);
    data_["check_box"] = checkBox;
    root_->AddChild(checkBox);
    Update();
}

template<> void AttributeEditor<bool>::Update()
{
    CheckBox *b = dynamic_cast<CheckBox*>(data_["check_box"].GetPtr());
    b->SetChecked(Value());
}

//----------------------------TRANSFORM ATTRIBUTE TYPE----------------------------

template<> void AttributeEditor<Transform>::SetValue(const Transform &value)
{
    VariantMap t;
    t["Pos"] = value.pos;
    t["Rot"] = value.rot;
    t["Scl"] = value.scale;
    value_ = t;
}

template<> const Transform &AttributeEditor<Transform>::Value() const
{
    VariantMap transform = value_.GetVariantMap();
    return Transform(transform["pos"].GetVector3(), transform["rot"].GetVector3(), transform["scl"].GetVector3());
}

template<> void AttributeEditor<Transform>::Initialize()
{
    IAttributeEditor::Initialize();
    XMLFile *style = context_->GetSubsystem<ResourceCache>()->GetResource<XMLFile>("Data/UI/DefaultStyle.xml");

    UIElement *v_element = new UIElement(framework_->GetContext());
    v_element->SetLayout(LayoutMode::LM_VERTICAL, 2);
    root_->AddChild(v_element);

    // POSITION
    {
        UIElement *pos_area = new UIElement(framework_->GetContext());
        pos_area->SetLayout(LayoutMode::LM_HORIZONTAL, 2);
        v_element->AddChild(pos_area);

        {
            Text *t = new Text(framework_->GetContext());
            t->SetStyle("Text", style);
            t->SetMaxWidth(12);
            t->SetMinWidth(12);
            t->SetText("x");
            pos_area->AddChild(t);

            LineEdit *e = new LineEdit(framework_->GetContext());
            e->SetStyle("LineEdit", style);
            pos_area->AddChild(e);
            e->SetCursorPosition(0);
            data_["x_pos_edit"] = e;

            t = new Text(framework_->GetContext());
            t->SetStyle("Text", style);
            t->SetMaxWidth(12);
            t->SetMinWidth(12);
            t->SetText("y");
            pos_area->AddChild(t);

            e = new LineEdit(framework_->GetContext());
            e->SetStyle("LineEdit", style);
            e->SetCursorPosition(0);
            pos_area->AddChild(e);
            data_["y_pos_edit"] = e;

            t = new Text(framework_->GetContext());
            t->SetStyle("Text", style);
            t->SetText("z");
            t->SetMaxWidth(12);
            t->SetMinWidth(12);
            pos_area->AddChild(t);

            e = new LineEdit(framework_->GetContext());
            e->SetStyle("LineEdit", style);
            e->SetCursorPosition(0);
            pos_area->AddChild(e);
            data_["z_pos_edit"] = e;
        }
    }

    // ROTATION
    {
        UIElement *rot_area = new UIElement(framework_->GetContext());
        rot_area->SetLayout(LayoutMode::LM_HORIZONTAL, 2);
        v_element->AddChild(rot_area);

        {
            Text *t = new Text(framework_->GetContext());
            t->SetStyle("Text", style);
            t->SetMaxWidth(12);
            t->SetMinWidth(12);
            t->SetText("x");
            rot_area->AddChild(t);

            LineEdit *e = new LineEdit(framework_->GetContext());
            e->SetStyle("LineEdit", style);
            rot_area->AddChild(e);
            e->SetCursorPosition(0);
            data_["x_rot_edit"] = e;

            t = new Text(framework_->GetContext());
            t->SetStyle("Text", style);
            t->SetMaxWidth(12);
            t->SetMinWidth(12);
            t->SetText("y");
            rot_area->AddChild(t);

            e = new LineEdit(framework_->GetContext());
            e->SetStyle("LineEdit", style);
            e->SetCursorPosition(0);
            rot_area->AddChild(e);
            data_["y_rot_edit"] = e;

            t = new Text(framework_->GetContext());
            t->SetStyle("Text", style);
            t->SetText("z");
            t->SetMaxWidth(12);
            t->SetMinWidth(12);
            rot_area->AddChild(t);

            e = new LineEdit(framework_->GetContext());
            e->SetStyle("LineEdit", style);
            e->SetCursorPosition(0);
            rot_area->AddChild(e);
            data_["z_rot_edit"] = e;
        }
    }

    // SCALE
    {
        UIElement *scl_area = new UIElement(framework_->GetContext());
        scl_area->SetLayout(LayoutMode::LM_HORIZONTAL, 2);
        v_element->AddChild(scl_area);

        {
            Text *t = new Text(framework_->GetContext());
            t->SetStyle("Text", style);
            t->SetMaxWidth(12);
            t->SetMinWidth(12);
            t->SetText("x");
            scl_area->AddChild(t);

            LineEdit *e = new LineEdit(framework_->GetContext());
            e->SetStyle("LineEdit", style);
            scl_area->AddChild(e);
            e->SetCursorPosition(0);
            data_["x_scl_edit"] = e;

            t = new Text(framework_->GetContext());
            t->SetStyle("Text", style);
            t->SetMaxWidth(12);
            t->SetMinWidth(12);
            t->SetText("y");
            scl_area->AddChild(t);

            e = new LineEdit(framework_->GetContext());
            e->SetStyle("LineEdit", style);
            e->SetCursorPosition(0);
            scl_area->AddChild(e);
            data_["y_scl_edit"] = e;

            t = new Text(framework_->GetContext());
            t->SetStyle("Text", style);
            t->SetText("z");
            t->SetMaxWidth(12);
            t->SetMinWidth(12);
            scl_area->AddChild(t);

            e = new LineEdit(framework_->GetContext());
            e->SetStyle("LineEdit", style);
            e->SetCursorPosition(0);
            scl_area->AddChild(e);
            data_["z_scl_edit"] = e;
        }
    }
    Update();
}

template<> void AttributeEditor<Transform>::Update()
{
    Transform v = Value();

    //POSITION
    Vector3 pos = v.pos;
    {
        LineEdit *e = dynamic_cast<LineEdit*>(data_["x_pos_edit"].GetPtr());
        e->SetText(String(pos.x_));
        e->SetCursorPosition(0);

        e = dynamic_cast<LineEdit*>(data_["y_pos_edit"].GetPtr());
        e->SetText(String(pos.y_));
        e->SetCursorPosition(0);

        e = dynamic_cast<LineEdit*>(data_["z_pos_edit"].GetPtr());
        e->SetText(String(pos.z_));
        e->SetCursorPosition(0);
    }

    // ROTATION
    Vector3 rot = v.rot;
    {
        LineEdit *e = dynamic_cast<LineEdit*>(data_["x_rot_edit"].GetPtr());
        e->SetText(String(rot.x_));
        e->SetCursorPosition(0);

        e = dynamic_cast<LineEdit*>(data_["y_rot_edit"].GetPtr());
        e->SetText(String(rot.y_));
        e->SetCursorPosition(0);

        e = dynamic_cast<LineEdit*>(data_["z_rot_edit"].GetPtr());
        e->SetText(String(rot.z_));
        e->SetCursorPosition(0);
    }

    // SCALE
    Vector3 scl = v.scale;
    {
        LineEdit *e = dynamic_cast<LineEdit*>(data_["x_scl_edit"].GetPtr());
        e->SetText(String(scl.x_));
        e->SetCursorPosition(0);

        e = dynamic_cast<LineEdit*>(data_["y_scl_edit"].GetPtr());
        e->SetText(String(scl.y_));
        e->SetCursorPosition(0);

        e = dynamic_cast<LineEdit*>(data_["z_scl_edit"].GetPtr());
        e->SetText(String(scl.z_));
        e->SetCursorPosition(0);
    }
}

//----------------------------VECTOR3 ATTRIBUTE TYPE----------------------------

template<> void AttributeEditor<Vector3>::SetValue(const Vector3 &value)
{
    value_ = value;
    Update();
}

template<> const Vector3 &AttributeEditor<Vector3>::Value() const
{
    return value_.GetVector3();
}

template<> void AttributeEditor<Vector3>::Initialize()
{
    IAttributeEditor::Initialize();
    XMLFile *style = context_->GetSubsystem<ResourceCache>()->GetResource<XMLFile>("Data/UI/DefaultStyle.xml");

    UIElement *element = new UIElement(framework_->GetContext());
    
    element->SetLayout(LayoutMode::LM_HORIZONTAL, 2);
    {
        
        Text *t = new Text(framework_->GetContext());
        t->SetStyle("Text", style);
        t->SetMaxWidth(12);
        t->SetMinWidth(12);
        t->SetText("x");
        element->AddChild(t);

        LineEdit *e = new LineEdit(framework_->GetContext());
        e->SetStyle("LineEdit", style);
        element->AddChild(e);
        e->SetCursorPosition(0);
        data_["x_edit"] = e;

        t = new Text(framework_->GetContext());
        t->SetStyle("Text", style);
        t->SetMaxWidth(12);
        t->SetMinWidth(12);
        t->SetText("y");
        element->AddChild(t);

        e = new LineEdit(framework_->GetContext());
        e->SetStyle("LineEdit", style);
        e->SetCursorPosition(0);
        element->AddChild(e);
        data_["y_edit"] = e;

        t = new Text(framework_->GetContext());
        t->SetStyle("Text", style);
        t->SetText("z");
        t->SetMaxWidth(12);
        t->SetMinWidth(12);
        element->AddChild(t);

        e = new LineEdit(framework_->GetContext());
        e->SetStyle("LineEdit", style);
        e->SetCursorPosition(0);
        element->AddChild(e);
        data_["z_edit"] = e;
    }
    root_->AddChild(element);
    Update();
}

template<> void AttributeEditor<Vector3>::Update()
{
    Vector3 v = Value();

    LineEdit *e = dynamic_cast<LineEdit*>(data_["x_edit"].GetPtr());
    e->SetText(String(v.x_));
    e->SetCursorPosition(0);

    e = dynamic_cast<LineEdit*>(data_["y_edit"].GetPtr());
    e->SetText(String(v.y_));
    e->SetCursorPosition(0);

    e = dynamic_cast<LineEdit*>(data_["z_edit"].GetPtr());
    e->SetText(String(v.z_));
    e->SetCursorPosition(0);
}

//----------------------------STRING ATTRIBUTE TYPE----------------------------

template<> void AttributeEditor<String>::SetValue(const String &value)
{
    value_ = value;
    Update();
}

template<> const String &AttributeEditor<String>::Value() const
{
    return value_.GetString();
}

template<> void AttributeEditor<String>::Initialize()
{
    IAttributeEditor::Initialize();
    XMLFile *style = context_->GetSubsystem<ResourceCache>()->GetResource<XMLFile>("Data/UI/DefaultStyle.xml");

    LineEdit *e = new LineEdit(framework_->GetContext());
    e->SetName("LineEdit");
    e->SetStyle("LineEdit", style);
    e->SetCursorPosition(0);
    data_["line_edit"] = e;
    root_->AddChild(e);
    Update();
}

template<> void AttributeEditor<String>::Update()
{
    LineEdit *e = dynamic_cast<LineEdit*>(root_->GetChild("LineEdit", true));
    e->SetText(Value());
    e->SetCursorPosition(0);
}

}