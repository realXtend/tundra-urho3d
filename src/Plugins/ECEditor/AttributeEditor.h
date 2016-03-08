/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   ECEditor.h
    @brief  ECEditor core API. */

#pragma once

#include "ECEditorApi.h"
#include "CoreTypes.h"
#include "FrameworkFwd.h"
#include "Signals.h"
#include "Math/Transform.h"
#include "IAttribute.h"
#include "Signals.h"

#include <Urho3D/Core/Object.h>
#include <Urho3D/Container/Ptr.h>

namespace Urho3D
{
    class UIElement;
    class Text;
    class LineEdit;
    class CheckBox;
}

using namespace Urho3D;

namespace Tundra
{

typedef SharedPtr<Text> TextPtr;
typedef SharedPtr<UIElement> UIElementPtr;
typedef Vector<AttributeWeakPtr> AttributeWeakPtrVector;

class ECEDITOR_API IAttributeEditor : public Object
{
    URHO3D_OBJECT(IAttributeEditor, Object);

public:
    explicit IAttributeEditor(Framework *framework);
    virtual ~IAttributeEditor();

    virtual const String &Title() const;
    virtual void SetTitle(const String &text);

    UIElementPtr Widget() const;

    bool HasAttribute(IAttribute *attribute) const;
    void AddAttribute(AttributeWeakPtr attribute);
    void RemoveAttribute();

protected:
    /// Listens if attribute value have changed outside from the editor and update changes into UI.
    virtual void OnAttributeChanged(IAttribute *attribute, AttributeChange::Type changeType);

    /// Initialize attribute editor UI elements and register to UI events.
    virtual void Initialize();
    
    /// Reads attribute value from attribute and sets it to ui.
    virtual void Update();
    
    /// Reads value from UI element and set it to attribute.
    virtual void SetValue();

    UIElementPtr root_;
    TextPtr title_;
    Variant value_;
    Framework *framework_;
    VariantMap data_;
    AttributeWeakPtr attributeWeakPtr_;
    bool ingoreAttributeChange_;
};

template <typename T>
class AttributeEditor : public IAttributeEditor
{
    URHO3D_OBJECT(AttributeEditor, IAttributeEditor);

public:
    AttributeEditor(Framework *framework) :
        IAttributeEditor(framework)
    {
        Initialize();
    }

    ~AttributeEditor() override
    {
        
    }

    void SetValue(const T &value);
    const T &Value() const;

protected:
    void Initialize() override;
    void Update() override;
    void OnUIChanged(StringHash eventType, VariantMap &eventData);
    void SetValue() override;
};

template<> void AttributeEditor<bool>::SetValue(const bool &value);
template<> const bool &AttributeEditor<bool>::Value() const;
template<> void AttributeEditor<bool>::Initialize();
template<> void AttributeEditor<bool>::Update();
template<> void AttributeEditor<bool>::OnUIChanged(StringHash eventType, VariantMap &eventData);
template<> void AttributeEditor<bool>::SetValue();

template<> void AttributeEditor<float>::SetValue(const float &value);
template<> const float &AttributeEditor<float>::Value() const;
template<> void AttributeEditor<float>::Initialize();
template<> void AttributeEditor<float>::Update();
template<> void AttributeEditor<float>::OnUIChanged(StringHash eventType, VariantMap &eventData);
template<> void AttributeEditor<float>::SetValue();

template<> void AttributeEditor<int>::SetValue(const int &value);
template<> const int &AttributeEditor<int>::Value() const;
template<> void AttributeEditor<int>::Initialize();
template<> void AttributeEditor<int>::Update();
template<> void AttributeEditor<int>::OnUIChanged(StringHash eventType, VariantMap &eventData);
template<> void AttributeEditor<int>::SetValue();

template<> void AttributeEditor<Transform>::SetValue(const Transform &value);
template<> const Transform &AttributeEditor<Transform>::Value() const;
template<> void AttributeEditor<Transform>::Initialize();
template<> void AttributeEditor<Transform>::Update();
template<> void AttributeEditor<Transform>::OnUIChanged(StringHash eventType, VariantMap &eventData);
template<> void AttributeEditor<Transform>::SetValue();

template<> void AttributeEditor<Vector3>::SetValue(const Vector3 &value);
template<> const Vector3 &AttributeEditor<Vector3>::Value() const;
template<> void AttributeEditor<Vector3>::Initialize();
template<> void AttributeEditor<Vector3>::Update();
template<> void AttributeEditor<Vector3>::OnUIChanged(StringHash eventType, VariantMap &eventData);
template<> void AttributeEditor<Vector3>::SetValue();

template<> void AttributeEditor<String>::SetValue(const String &value);
template<> const String &AttributeEditor<String>::Value() const;
template<> void AttributeEditor<String>::Initialize();
template<> void AttributeEditor<String>::Update();
template<> void AttributeEditor<String>::OnUIChanged(StringHash eventType, VariantMap &eventData);
template<> void AttributeEditor<String>::SetValue();

}
