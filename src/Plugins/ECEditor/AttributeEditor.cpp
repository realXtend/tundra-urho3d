// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

#include "AttributeEditor.h"
#include "Framework.h"
#include "IComponent.h"

#include <Urho3D/Core/Context.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Resource/ResourceCache.h>

#include "LoggingFunctions.h"

#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/UIElement.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/CheckBox.h>
#include <Urho3D/UI/LineEdit.h>

namespace Tundra
{

IAttributeEditor::IAttributeEditor(Framework *framework, AttributeWeakPtr attribute) :
    Object(framework->GetContext()),
    framework_(framework),
    ingoreAttributeChange_(false),
    intialized_(false)
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

bool IAttributeEditor::HasAttribute(IAttribute *attribute) const
{
    return attributeWeakPtr_.Get() == NULL ? false : attribute == attributeWeakPtr_.Get();
}

void IAttributeEditor::AddAttribute(AttributeWeakPtr attribute)
{
    if (attributeWeakPtr_.Get() != NULL)
        attributeWeakPtr_.owner.Lock()->AttributeChanged.Disconnect(this, &IAttributeEditor::OnAttributeChanged);

    attributeWeakPtr_ = attribute;
    attributeWeakPtr_.owner.Lock()->AttributeChanged.Connect(this, &IAttributeEditor::OnAttributeChanged);
    SetValue();
}

void IAttributeEditor::RemoveAttribute()
{
    if (attributeWeakPtr_.Get() != NULL)
        attributeWeakPtr_.owner.Lock()->AttributeChanged.Disconnect(this, &IAttributeEditor::OnAttributeChanged);

    attributeWeakPtr_ = AttributeWeakPtr();
}

void IAttributeEditor::OnAttributeChanged(IAttribute *attribute, AttributeChange::Type changeType)
{
    if (HasAttribute(attribute))
        SetValue();
}

void IAttributeEditor::Initialize()
{
    Urho3D::XMLFile *style = context_->GetSubsystem<Urho3D::ResourceCache>()->GetResource<Urho3D::XMLFile>("Data/UI/DefaultStyle.xml");

    root_ = new Urho3D::UIElement(framework_->GetContext());
    root_->SetHeight(22);
    root_->SetLayout(Urho3D::LayoutMode::LM_HORIZONTAL, 2, Urho3D::IntRect(2, 0, 2, 0));

    title_ = new Urho3D::Text(framework_->GetContext());
    title_->SetMinWidth(120);
    title_->SetMaxWidth(120);
    title_->SetStyle("SmallText", style);
    root_->AddChild(title_);

    intialized_ = true;
}

void IAttributeEditor::Update()
{

}

void IAttributeEditor::SetValue()
{

}

//----------------------------BOOL ATTRIBUTE TYPE----------------------------

template<> void AttributeEditor<bool>::SetValue(bool value)
{
    value_ = value;
    Update();
}

template<> bool AttributeEditor<bool>::Value() const
{
    return value_.GetBool();
}

template<> void AttributeEditor<bool>::Initialize()
{
    IAttributeEditor::Initialize();
    Urho3D::XMLFile *style = context_->GetSubsystem<Urho3D::ResourceCache>()->GetResource<Urho3D::XMLFile>("Data/UI/DefaultStyle.xml");

    Urho3D::CheckBox *checkBox = new Urho3D::CheckBox(framework_->GetContext());
    checkBox->SetStyle("CheckBoxSmall", style);
    data_["check_box"] = checkBox;
    root_->AddChild(checkBox);

    SubscribeToEvent(Urho3D::E_TOGGLED, URHO3D_HANDLER(AttributeEditor<bool>, OnUIChanged));
    
    Update();
}

template<> void AttributeEditor<bool>::Update()
{
    Urho3D::CheckBox *b = dynamic_cast<Urho3D::CheckBox*>(data_["check_box"].GetPtr());
    b->SetChecked(Value());
}

template<> void AttributeEditor<bool>::OnUIChanged(StringHash /*eventType*/, VariantMap &eventData)
{
    if (attributeWeakPtr_.Get() == NULL)
        return;

    Urho3D::CheckBox *check = dynamic_cast<Urho3D::CheckBox*>(eventData["Element"].GetPtr());
    if (check != NULL &&
        check == data_["check_box"].GetPtr())
    {
        ingoreAttributeChange_ = true;

        bool checked = check->IsChecked();
        Attribute<bool> *attr = dynamic_cast<Attribute<bool> *>(attributeWeakPtr_.Get());
        if (attr != NULL)
            attr->Set(checked);

        ingoreAttributeChange_ = false;
    }
}

template<> void AttributeEditor<bool>::SetValue()
{
    if (!ingoreAttributeChange_ && attributeWeakPtr_.Get() != NULL)
    {
        Attribute<bool> *attr = dynamic_cast<Attribute<bool> *>(attributeWeakPtr_.Get());
        if (attr == NULL)
            return;

        SetValue(attr->Get());
    }
}

//----------------------------FLOAT ATTRIBUTE TYPE----------------------------

template<> void AttributeEditor<float>::SetValue(float value)
{
    value_ = value;
    Update();
}

template<> float AttributeEditor<float>::Value() const
{
    return value_.GetFloat();
}

template<> void AttributeEditor<float>::Initialize()
{
    IAttributeEditor::Initialize();
    Urho3D::XMLFile *style = context_->GetSubsystem<Urho3D::ResourceCache>()->GetResource<Urho3D::XMLFile>("Data/UI/DefaultStyle.xml");

    Urho3D::LineEdit *e = new Urho3D::LineEdit(framework_->GetContext());
    e->SetName("LineEdit");
    e->SetStyle("LineEditSmall", style);
    e->SetCursorPosition(0);
    data_["line_edit"] = e;
    root_->AddChild(e);

    SubscribeToEvent(Urho3D::E_TEXTFINISHED, URHO3D_HANDLER(AttributeEditor<float>, OnUIChanged));

    Update();
}

template<> void AttributeEditor<float>::Update()
{
    if (!intialized_)
        return;

    Urho3D::LineEdit *e = dynamic_cast<Urho3D::LineEdit*>(data_["line_edit"].GetPtr());
    e->SetText(String(Value()));
    e->SetCursorPosition(0);
}

template<> void AttributeEditor<float>::OnUIChanged(StringHash /*eventType*/, VariantMap &eventData)
{
    if (attributeWeakPtr_.Get() == NULL)
        return;

    Urho3D::LineEdit *element = dynamic_cast<Urho3D::LineEdit*>(eventData["Element"].GetPtr());
    if (element != NULL &&
        element == data_["line_edit"].GetPtr())
    {
        ingoreAttributeChange_ = true;

        String value = element->GetText();
        Attribute<float> *attr = dynamic_cast<Attribute<float> *>(attributeWeakPtr_.Get());
        if (attr != NULL)
            attr->Set(ToFloat(value));

        ingoreAttributeChange_ = false;
    }
}

template<> void AttributeEditor<float>::SetValue()
{
    if (!ingoreAttributeChange_ && attributeWeakPtr_.Get() != NULL)
    {
        Attribute<float> *attr = dynamic_cast<Attribute<float> *>(attributeWeakPtr_.Get());
        if (attr == NULL)
            return;

        SetValue(attr->Get());
    }
}

//----------------------------INT ATTRIBUTE TYPE----------------------------

template<> void AttributeEditor<int>::SetValue(int value)
{
    value_ = value;
    Update();
}

template<> int AttributeEditor<int>::Value() const
{
    return value_.GetInt();
}

template<> void AttributeEditor<int>::Initialize()
{
    IAttributeEditor::Initialize();
    Urho3D::XMLFile *style = context_->GetSubsystem<Urho3D::ResourceCache>()->GetResource<Urho3D::XMLFile>("Data/UI/DefaultStyle.xml");

    Urho3D::LineEdit *e = new Urho3D::LineEdit(framework_->GetContext());
    e->SetName("LineEdit");
    e->SetStyle("LineEditSmall", style);
    e->SetCursorPosition(0);
    data_["line_edit"] = e;
    root_->AddChild(e);

    SubscribeToEvent(Urho3D::E_TEXTFINISHED, URHO3D_HANDLER(AttributeEditor<int>, OnUIChanged));

    Update();
}

template<> void AttributeEditor<int>::Update()
{
    if (!intialized_)
        return;

    Urho3D::LineEdit *e = dynamic_cast<Urho3D::LineEdit*>(data_["line_edit"].GetPtr());
    e->SetText(String(Value()));
    e->SetCursorPosition(0);
}

template<> void AttributeEditor<int>::OnUIChanged(StringHash /*eventType*/, VariantMap &eventData)
{
    if (attributeWeakPtr_.Get() == NULL)
        return;

    Urho3D::LineEdit *element = dynamic_cast<Urho3D::LineEdit*>(eventData["Element"].GetPtr());
    if (element != NULL &&
        element == data_["line_edit"].GetPtr())
    {
        ingoreAttributeChange_ = true;

        String value = element->GetText();
        Attribute<int> *attr = dynamic_cast<Attribute<int> *>(attributeWeakPtr_.Get());
        if (attr != NULL)
            attr->Set(ToInt(value));

        ingoreAttributeChange_ = false;
    }
}

template<> void AttributeEditor<int>::SetValue()
{
    if (!ingoreAttributeChange_ && attributeWeakPtr_.Get() != NULL)
    {
        Attribute<int> *attr = dynamic_cast<Attribute<int> *>(attributeWeakPtr_.Get());
        if (attr == NULL)
            return;

        SetValue(attr->Get());
    }
}

//----------------------------TRANSFORM ATTRIBUTE TYPE----------------------------

template<> void AttributeEditor<Transform>::SetValue(Transform value)
{
    VariantMap t;
    t["Pos"] = value.pos;
    t["Rot"] = value.rot;
    t["Scl"] = value.scale;
    value_ = t;
}

template<> Transform AttributeEditor<Transform>::Value() const
{
    VariantMap t = value_.GetVariantMap();
    return Transform(t["Pos"].GetVector3(), t["Rot"].GetVector3(), t["Scl"].GetVector3());
}

template<> void AttributeEditor<Transform>::Initialize()
{
    IAttributeEditor::Initialize();
    Urho3D::XMLFile *style = context_->GetSubsystem<Urho3D::ResourceCache>()->GetResource<Urho3D::XMLFile>("Data/UI/DefaultStyle.xml");

    Urho3D::UIElement *v_element = new Urho3D::UIElement(framework_->GetContext());
    v_element->SetLayout(Urho3D::LayoutMode::LM_VERTICAL, 2);
    root_->AddChild(v_element);

    // POSITION
    {
        Urho3D::UIElement *pos_area = new Urho3D::UIElement(framework_->GetContext());
        pos_area->SetLayout(Urho3D::LayoutMode::LM_HORIZONTAL, 2);
        v_element->AddChild(pos_area);

        {
            Urho3D::Text *t = new Urho3D::Text(framework_->GetContext());
            t->SetStyle("SmallText", style);
            t->SetMaxWidth(12);
            t->SetMinWidth(12);
            t->SetText("x");
            pos_area->AddChild(t);

            Urho3D::LineEdit *e = new Urho3D::LineEdit(framework_->GetContext());
            e->SetStyle("LineEditSmall", style);
            pos_area->AddChild(e);
            e->SetCursorPosition(0);
            data_["x_pos_edit"] = e;

            t = new Urho3D::Text(framework_->GetContext());
            t->SetStyle("SmallText", style);
            t->SetMaxWidth(12);
            t->SetMinWidth(12);
            t->SetText("y");
            pos_area->AddChild(t);

            e = new Urho3D::LineEdit(framework_->GetContext());
            e->SetStyle("LineEditSmall", style);
            e->SetCursorPosition(0);
            pos_area->AddChild(e);
            data_["y_pos_edit"] = e;

            t = new Urho3D::Text(framework_->GetContext());
            t->SetStyle("SmallText", style);
            t->SetText("z");
            t->SetMaxWidth(12);
            t->SetMinWidth(12);
            pos_area->AddChild(t);

            e = new Urho3D::LineEdit(framework_->GetContext());
            e->SetStyle("LineEditSmall", style);
            e->SetCursorPosition(0);
            pos_area->AddChild(e);
            data_["z_pos_edit"] = e;
        }
    }

    // ROTATION
    {
        Urho3D::UIElement *rot_area = new Urho3D::UIElement(framework_->GetContext());
        rot_area->SetLayout(Urho3D::LayoutMode::LM_HORIZONTAL, 2);
        v_element->AddChild(rot_area);

        {
            Urho3D::Text *t = new Urho3D::Text(framework_->GetContext());
            t->SetStyle("SmallText", style);
            t->SetMaxWidth(12);
            t->SetMinWidth(12);
            t->SetText("x");
            rot_area->AddChild(t);

            Urho3D::LineEdit *e = new Urho3D::LineEdit(framework_->GetContext());
            e->SetStyle("LineEditSmall", style);
            rot_area->AddChild(e);
            e->SetCursorPosition(0);
            data_["x_rot_edit"] = e;

            t = new Urho3D::Text(framework_->GetContext());
            t->SetStyle("SmallText", style);
            t->SetMaxWidth(12);
            t->SetMinWidth(12);
            t->SetText("y");
            rot_area->AddChild(t);

            e = new Urho3D::LineEdit(framework_->GetContext());
            e->SetStyle("LineEditSmall", style);
            e->SetCursorPosition(0);
            rot_area->AddChild(e);
            data_["y_rot_edit"] = e;

            t = new Urho3D::Text(framework_->GetContext());
            t->SetStyle("SmallText", style);
            t->SetText("z");
            t->SetMaxWidth(12);
            t->SetMinWidth(12);
            rot_area->AddChild(t);

            e = new Urho3D::LineEdit(framework_->GetContext());
            e->SetStyle("LineEditSmall", style);
            e->SetCursorPosition(0);
            rot_area->AddChild(e);
            data_["z_rot_edit"] = e;
        }
    }

    // SCALE
    {
        Urho3D::UIElement *scl_area = new Urho3D::UIElement(framework_->GetContext());
        scl_area->SetLayout(Urho3D::LayoutMode::LM_HORIZONTAL, 2);
        v_element->AddChild(scl_area);

        {
            Urho3D::Text *t = new Urho3D::Text(framework_->GetContext());
            t->SetStyle("SmallText", style);
            t->SetMaxWidth(12);
            t->SetMinWidth(12);
            t->SetText("x");
            scl_area->AddChild(t);

            Urho3D::LineEdit *e = new Urho3D::LineEdit(framework_->GetContext());
            e->SetStyle("LineEditSmall", style);
            scl_area->AddChild(e);
            e->SetCursorPosition(0);
            data_["x_scl_edit"] = e;

            t = new Urho3D::Text(framework_->GetContext());
            t->SetStyle("SmallText", style);
            t->SetMaxWidth(12);
            t->SetMinWidth(12);
            t->SetText("y");
            scl_area->AddChild(t);

            e = new Urho3D::LineEdit(framework_->GetContext());
            e->SetStyle("LineEditSmall", style);
            e->SetCursorPosition(0);
            scl_area->AddChild(e);
            data_["y_scl_edit"] = e;

            t = new Urho3D::Text(framework_->GetContext());
            t->SetStyle("SmallText", style);
            t->SetText("z");
            t->SetMaxWidth(12);
            t->SetMinWidth(12);
            scl_area->AddChild(t);

            e = new Urho3D::LineEdit(framework_->GetContext());
            e->SetStyle("LineEditSmall", style);
            e->SetCursorPosition(0);
            scl_area->AddChild(e);
            data_["z_scl_edit"] = e;
        }
    }

    SubscribeToEvent(Urho3D::E_TEXTFINISHED, URHO3D_HANDLER(AttributeEditor<Transform>, OnUIChanged));
    Update();
}

template<> void AttributeEditor<Transform>::Update()
{
    if (!intialized_)
        return;

    const Transform &v = Value();

    //POSITION
    Urho3D::Vector3 pos = v.pos;
    {
        Urho3D::LineEdit *e = dynamic_cast<Urho3D::LineEdit*>(data_["x_pos_edit"].GetPtr());
        e->SetText(String(pos.x_));
        e->SetCursorPosition(0);

        e = dynamic_cast<Urho3D::LineEdit*>(data_["y_pos_edit"].GetPtr());
        e->SetText(String(pos.y_));
        e->SetCursorPosition(0);

        e = dynamic_cast<Urho3D::LineEdit*>(data_["z_pos_edit"].GetPtr());
        e->SetText(String(pos.z_));
        e->SetCursorPosition(0);
    }

    // ROTATION
    Urho3D::Vector3 rot = v.rot;
    {
        Urho3D::LineEdit *e = dynamic_cast<Urho3D::LineEdit*>(data_["x_rot_edit"].GetPtr());
        e->SetText(String(rot.x_));
        e->SetCursorPosition(0);

        e = dynamic_cast<Urho3D::LineEdit*>(data_["y_rot_edit"].GetPtr());
        e->SetText(String(rot.y_));
        e->SetCursorPosition(0);

        e = dynamic_cast<Urho3D::LineEdit*>(data_["z_rot_edit"].GetPtr());
        e->SetText(String(rot.z_));
        e->SetCursorPosition(0);
    }

    // SCALE
    Urho3D::Vector3 scl = v.scale;
    {
        Urho3D::LineEdit *e = dynamic_cast<Urho3D::LineEdit*>(data_["x_scl_edit"].GetPtr());
        e->SetText(String(scl.x_));
        e->SetCursorPosition(0);

        e = dynamic_cast<Urho3D::LineEdit*>(data_["y_scl_edit"].GetPtr());
        e->SetText(String(scl.y_));
        e->SetCursorPosition(0);

        e = dynamic_cast<Urho3D::LineEdit*>(data_["z_scl_edit"].GetPtr());
        e->SetText(String(scl.z_));
        e->SetCursorPosition(0);
    }
}

template<> void AttributeEditor<Transform>::OnUIChanged(StringHash /*eventType*/, VariantMap &eventData)
{
    if (attributeWeakPtr_.Get() == NULL)
        return;

    Urho3D::LineEdit *element = dynamic_cast<Urho3D::LineEdit*>(eventData["Element"].GetPtr());
    if (element == NULL)
        return;

    ingoreAttributeChange_ = true;

    if (element == data_["x_pos_edit"].GetPtr() || element == data_["y_pos_edit"].GetPtr() ||  element == data_["z_pos_edit"].GetPtr())
    {
        String value = element->GetText();
        Attribute<Transform> *attr = dynamic_cast<Attribute<Transform> *>(attributeWeakPtr_.Get());
        if (attr != NULL)
        {
            Transform t = attr->Get();
            t.pos.x = Urho3D::ToFloat(dynamic_cast<Urho3D::LineEdit*>(data_["x_pos_edit"].GetPtr())->GetText());
            t.pos.y = Urho3D::ToFloat(dynamic_cast<Urho3D::LineEdit*>(data_["y_pos_edit"].GetPtr())->GetText());
            t.pos.z = Urho3D::ToFloat(dynamic_cast<Urho3D::LineEdit*>(data_["z_pos_edit"].GetPtr())->GetText());
            attr->Set(t);
        }
    }
    else if (element == data_["x_rot_edit"].GetPtr() || element == data_["y_rot_edit"].GetPtr() || element == data_["z_rot_edit"].GetPtr())
    {
        String value = element->GetText();
        Attribute<Transform> *attr = dynamic_cast<Attribute<Transform> *>(attributeWeakPtr_.Get());
        if (attr != NULL)
        {
            Transform t = attr->Get();
            t.rot.x = Urho3D::ToFloat(dynamic_cast<Urho3D::LineEdit*>(data_["x_rot_edit"].GetPtr())->GetText());
            t.rot.y = Urho3D::ToFloat(dynamic_cast<Urho3D::LineEdit*>(data_["y_rot_edit"].GetPtr())->GetText());
            t.rot.z = Urho3D::ToFloat(dynamic_cast<Urho3D::LineEdit*>(data_["z_rot_edit"].GetPtr())->GetText());
            attr->Set(t);
        }
    }
    else if (element == data_["x_scl_edit"].GetPtr() || element == data_["y_scl_edit"].GetPtr() || element == data_["z_scl_edit"].GetPtr())
    {
        String value = element->GetText();
        Attribute<Transform> *attr = dynamic_cast<Attribute<Transform> *>(attributeWeakPtr_.Get());
        if (attr != NULL)
        {
            Transform t = attr->Get();
            t.scale.x = Urho3D::ToFloat(dynamic_cast<Urho3D::LineEdit*>(data_["x_scl_edit"].GetPtr())->GetText());
            t.scale.y = Urho3D::ToFloat(dynamic_cast<Urho3D::LineEdit*>(data_["y_scl_edit"].GetPtr())->GetText());
            t.scale.z = Urho3D::ToFloat(dynamic_cast<Urho3D::LineEdit*>(data_["z_scl_edit"].GetPtr())->GetText());
            attr->Set(t);
        }
    }

    ingoreAttributeChange_ = false;
}

template<> void AttributeEditor<Transform>::SetValue()
{
    if (!ingoreAttributeChange_ && attributeWeakPtr_.Get() != NULL)
    {
        Attribute<Transform> *attr = dynamic_cast<Attribute<Transform> *>(attributeWeakPtr_.Get());
        if (attr == NULL)
            return;

        SetValue(attr->Get());
    }
}

//----------------------------VECTOR3 ATTRIBUTE TYPE----------------------------

template<> void AttributeEditor<float3>::SetValue(float3 value)
{
    value_ = value;
    Update();
}

template<> float3 AttributeEditor<float3>::Value() const
{
    return value_.GetVector3();
}

template<> void AttributeEditor<float3>::Initialize()
{
    IAttributeEditor::Initialize();
    Urho3D::XMLFile *style = context_->GetSubsystem<Urho3D::ResourceCache>()->GetResource<Urho3D::XMLFile>("Data/UI/DefaultStyle.xml");

    Urho3D::UIElement *element = new Urho3D::UIElement(framework_->GetContext());
    
    element->SetLayout(Urho3D::LayoutMode::LM_HORIZONTAL, 2);
    {
        Urho3D::Text *t = new Urho3D::Text(framework_->GetContext());
        t->SetStyle("SmallText", style);
        t->SetMaxWidth(12);
        t->SetMinWidth(12);
        t->SetText("x");
        element->AddChild(t);

        Urho3D::LineEdit *e = new Urho3D::LineEdit(framework_->GetContext());
        e->SetStyle("LineEditSmall", style);
        element->AddChild(e);
        e->SetCursorPosition(0);
        data_["x_edit"] = e;

        t = new Urho3D::Text(framework_->GetContext());
        t->SetStyle("SmallText", style);
        t->SetMaxWidth(12);
        t->SetMinWidth(12);
        t->SetText("y");
        element->AddChild(t);

        e = new Urho3D::LineEdit(framework_->GetContext());
        e->SetStyle("LineEditSmall", style);
        e->SetCursorPosition(0);
        element->AddChild(e);
        data_["y_edit"] = e;

        t = new Urho3D::Text(framework_->GetContext());
        t->SetStyle("SmallText", style);
        t->SetText("z");
        t->SetMaxWidth(12);
        t->SetMinWidth(12);
        element->AddChild(t);

        e = new Urho3D::LineEdit(framework_->GetContext());
        e->SetStyle("LineEditSmall", style);
        e->SetCursorPosition(0);
        element->AddChild(e);
        data_["z_edit"] = e;
    }
    root_->AddChild(element);

    SubscribeToEvent(Urho3D::E_TEXTFINISHED, URHO3D_HANDLER(AttributeEditor<float3>, OnUIChanged));

    Update();
}

template<> void AttributeEditor<float3>::Update()
{
    if (!intialized_)
        return;

    float3 v = Value();

    Urho3D::LineEdit *e = dynamic_cast<Urho3D::LineEdit*>(data_["x_edit"].GetPtr());
    e->SetText(String(v.x));
    e->SetCursorPosition(0);

    e = dynamic_cast<Urho3D::LineEdit*>(data_["y_edit"].GetPtr());
    e->SetText(String(v.y));
    e->SetCursorPosition(0);

    e = dynamic_cast<Urho3D::LineEdit*>(data_["z_edit"].GetPtr());
    e->SetText(String(v.z));
    e->SetCursorPosition(0);
}

template<> void AttributeEditor<float3>::OnUIChanged(StringHash /*eventType*/, VariantMap &eventData)
{
    if (attributeWeakPtr_.Get() == NULL)
        return;

    Urho3D::LineEdit *element = dynamic_cast<Urho3D::LineEdit*>(eventData["Element"].GetPtr());
    if (element == NULL)
        return;

    ingoreAttributeChange_ = true;

    if (element == data_["x_edit"].GetPtr() || element == data_["y_edit"].GetPtr() || element == data_["z_edit"].GetPtr())
    {
        String value = element->GetText();
        Attribute<float3> *attr = dynamic_cast<Attribute<float3> *>(attributeWeakPtr_.Get());
        if (attr != NULL)
        {
            float3 v = attr->Get();
            v.x = Urho3D::ToFloat(dynamic_cast<Urho3D::LineEdit*>(data_["x_edit"].GetPtr())->GetText());
            v.y = Urho3D::ToFloat(dynamic_cast<Urho3D::LineEdit*>(data_["y_edit"].GetPtr())->GetText());
            v.z = Urho3D::ToFloat(dynamic_cast<Urho3D::LineEdit*>(data_["z_edit"].GetPtr())->GetText());
            attr->Set(v);
        }
    }

    ingoreAttributeChange_ = false;
}

template<> void AttributeEditor<float3>::SetValue()
{
    if (!ingoreAttributeChange_ && attributeWeakPtr_.Get() != NULL)
    {
        Attribute<float3> *attr = dynamic_cast<Attribute<float3> *>(attributeWeakPtr_.Get());
        if (attr == NULL)
            return;

        SetValue(attr->Get());
    }
}

//----------------------------COLOR ATTRIBUTE TYPE----------------------------

template<> void AttributeEditor<Color>::SetValue(Color value)
{
    value_ = value;
    Update();
}

template<> Color AttributeEditor<Color>::Value() const
{
    return value_.GetColor();
}

template<> void AttributeEditor<Color>::Initialize()
{
    IAttributeEditor::Initialize();
    Urho3D::XMLFile *style = context_->GetSubsystem<Urho3D::ResourceCache>()->GetResource<Urho3D::XMLFile>("Data/UI/DefaultStyle.xml");

    Urho3D::UIElement *element = new Urho3D::UIElement(framework_->GetContext());

    element->SetLayout(Urho3D::LayoutMode::LM_HORIZONTAL, 2);
    {
        Urho3D::Text *t = new Urho3D::Text(framework_->GetContext());
        t->SetStyle("SmallText", style);
        t->SetMaxWidth(12);
        t->SetMinWidth(12);
        t->SetText("r");
        element->AddChild(t);

        Urho3D::LineEdit *e = new Urho3D::LineEdit(framework_->GetContext());
        e->SetStyle("LineEditSmall", style);
        element->AddChild(e);
        e->SetCursorPosition(0);
        data_["r_edit"] = e;

        t = new Urho3D::Text(framework_->GetContext());
        t->SetStyle("SmallText", style);
        t->SetText("g");
        t->SetMaxWidth(12);
        t->SetMinWidth(12);
        element->AddChild(t);

        e = new Urho3D::LineEdit(framework_->GetContext());
        e->SetStyle("LineEditSmall", style);
        e->SetCursorPosition(0);
        element->AddChild(e);
        data_["g_edit"] = e;

        t = new Urho3D::Text(framework_->GetContext());
        t->SetStyle("SmallText", style);
        t->SetMaxWidth(12);
        t->SetMinWidth(12);
        t->SetText("b");
        element->AddChild(t);

        e = new Urho3D::LineEdit(framework_->GetContext());
        e->SetStyle("LineEditSmall", style);
        e->SetCursorPosition(0);
        element->AddChild(e);
        data_["b_edit"] = e;

        t = new Urho3D::Text(framework_->GetContext());
        t->SetStyle("SmallText", style);
        t->SetText("a");
        t->SetMaxWidth(12);
        t->SetMinWidth(12);
        element->AddChild(t);

        e = new Urho3D::LineEdit(framework_->GetContext());
        e->SetStyle("LineEditSmall", style);
        e->SetCursorPosition(0);
        element->AddChild(e);
        data_["a_edit"] = e;
    }
    root_->AddChild(element);

    SubscribeToEvent(Urho3D::E_TEXTFINISHED, URHO3D_HANDLER(AttributeEditor<Color>, OnUIChanged));

    Update();
}

template<> void AttributeEditor<Color>::Update()
{
    if (!intialized_)
        return;

    Color c = Value();

    Urho3D::LineEdit *e = dynamic_cast<Urho3D::LineEdit*>(data_["r_edit"].GetPtr());
    e->SetText(String(c.r));
    e->SetCursorPosition(0);

    e = dynamic_cast<Urho3D::LineEdit*>(data_["g_edit"].GetPtr());
    e->SetText(String(c.g));
    e->SetCursorPosition(0);

    e = dynamic_cast<Urho3D::LineEdit*>(data_["b_edit"].GetPtr());
    e->SetText(String(c.b));
    e->SetCursorPosition(0);

    e = dynamic_cast<Urho3D::LineEdit*>(data_["a_edit"].GetPtr());
    e->SetText(String(c.a));
    e->SetCursorPosition(0);
}

template<> void AttributeEditor<Color>::OnUIChanged(StringHash /*eventType*/, VariantMap &eventData)
{
    if (attributeWeakPtr_.Get() == NULL)
        return;

    Urho3D::LineEdit *element = dynamic_cast<Urho3D::LineEdit*>(eventData["Element"].GetPtr());
    if (element == NULL)
        return;

    ingoreAttributeChange_ = true;

    if (element == data_["r_edit"].GetPtr() || element == data_["g_edit"].GetPtr() ||
        element == data_["b_edit"].GetPtr() || element == data_["a_edit"].GetPtr())
    {
        String value = element->GetText();
        Attribute<Color> *attr = dynamic_cast<Attribute<Color> *>(attributeWeakPtr_.Get());
        if (attr != NULL)
        {
            Color c = attr->Get();
            c.r = Urho3D::ToFloat(dynamic_cast<Urho3D::LineEdit*>(data_["r_edit"].GetPtr())->GetText());
            c.g = Urho3D::ToFloat(dynamic_cast<Urho3D::LineEdit*>(data_["g_edit"].GetPtr())->GetText());
            c.b = Urho3D::ToFloat(dynamic_cast<Urho3D::LineEdit*>(data_["b_edit"].GetPtr())->GetText());
            c.a = Urho3D::ToFloat(dynamic_cast<Urho3D::LineEdit*>(data_["a_edit"].GetPtr())->GetText());
            attr->Set(c);
        }
    }

    ingoreAttributeChange_ = false;
}

template<> void AttributeEditor<Color>::SetValue()
{
    if (!ingoreAttributeChange_ && attributeWeakPtr_.Get() != NULL)
    {
        Attribute<Color> *attr = dynamic_cast<Attribute<Color> *>(attributeWeakPtr_.Get());
        if (attr == NULL)
            return;

        SetValue(attr->Get());
    }
}

//----------------------------STRING ATTRIBUTE TYPE----------------------------

template<> void AttributeEditor<String>::SetValue(String value)
{
    value_ = value;
    Update();
}

template<> String AttributeEditor<String>::Value() const
{
    return value_.GetString();
}

template<> void AttributeEditor<String>::Initialize()
{
    IAttributeEditor::Initialize();
    Urho3D::XMLFile *style = context_->GetSubsystem<Urho3D::ResourceCache>()->GetResource<Urho3D::XMLFile>("Data/UI/DefaultStyle.xml");

    Urho3D::LineEdit *e = new Urho3D::LineEdit(framework_->GetContext());
    e->SetName("LineEdit");
    e->SetStyle("LineEditSmall", style);
    e->SetCursorPosition(0);
    data_["line_edit"] = e;
    root_->AddChild(e);
    Update();

    SubscribeToEvent(Urho3D::E_TEXTFINISHED, URHO3D_HANDLER(AttributeEditor<String>, OnUIChanged));
}

template<> void AttributeEditor<String>::Update()
{
    if (!intialized_)
        return;

    Urho3D::LineEdit *e = dynamic_cast<Urho3D::LineEdit*>(data_["line_edit"].GetPtr());
    e->SetText(Value());
    e->SetCursorPosition(0);
}

template<> void AttributeEditor<String>::OnUIChanged(StringHash /*eventType*/, VariantMap &eventData)
{
    if (attributeWeakPtr_.Get() == NULL)
        return;

    Urho3D::LineEdit *element = dynamic_cast<Urho3D::LineEdit*>(eventData["Element"].GetPtr());
    if (element != NULL &&
        element == data_["line_edit"].GetPtr())
    {
        ingoreAttributeChange_ = true;

        String value = element->GetText();
        Attribute<String> *attr = dynamic_cast<Attribute<String> *>(attributeWeakPtr_.Get());
        if (attr != NULL)
            attr->Set(value);

        ingoreAttributeChange_ = false;
    }
}

template<> void AttributeEditor<String>::SetValue()
{
    if (!ingoreAttributeChange_ && attributeWeakPtr_.Get() != NULL)
    {
        Attribute<String> *attr = dynamic_cast<Attribute<String> *>(attributeWeakPtr_.Get());
        if (attr == NULL)
            return;

        SetValue(attr->Get());
    }
}

}