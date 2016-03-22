/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   IAttributeEditor.h
    @brief  IAttributeEditor. */

#pragma once

#include "ECEditorApi.h"
#include "CoreTypes.h"
#include "FrameworkFwd.h"
#include "Signals.h"
#include "Math/Transform.h"
#include "Math/Color.h"
#include "IAttribute.h"
#include "Signals.h"
#include "EntityReference.h"
#include "AssetReference.h"

#include <Urho3D/Core/Object.h>
#include <Urho3D/Container/Ptr.h>

namespace Urho3D
{
    class UIElement;
    class Text;
    class LineEdit;
    class CheckBox;
}

namespace Tundra
{

typedef SharedPtr<Urho3D::Text> TextPtr;
typedef SharedPtr<Urho3D::UIElement> UIElementPtr;
typedef Vector<AttributeWeakPtr> AttributeWeakPtrVector;

class ECEDITOR_API IAttributeEditor : public Object
{
    URHO3D_OBJECT(IAttributeEditor, Object);

public:
    explicit IAttributeEditor(Framework *framework, AttributeWeakPtr attribute);
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
    bool intialized_;
};

template <typename T>
class AttributeEditor : public IAttributeEditor
{
    URHO3D_OBJECT(AttributeEditor, IAttributeEditor);

public:
    AttributeEditor(Framework *framework, AttributeWeakPtr attribute) :
        IAttributeEditor(framework, attribute)
    {
        Initialize();
        AddAttribute(attribute);
        Update();
    }

    ~AttributeEditor() override
    {
        
    }

    void SetValue(T value);
    T Value() const;

protected:
    void Initialize() override;
    void Update() override;
    void OnUIChanged(StringHash eventType, VariantMap &eventData);
    void SetValue() override;
};

template<> void AttributeEditor<bool>::SetValue(bool value);
template<> bool AttributeEditor<bool>::Value() const;
template<> void AttributeEditor<bool>::Initialize();
template<> void AttributeEditor<bool>::Update();
template<> void AttributeEditor<bool>::OnUIChanged(StringHash eventType, VariantMap &eventData);
template<> void AttributeEditor<bool>::SetValue();

template<> void AttributeEditor<float>::SetValue(float value);
template<> float AttributeEditor<float>::Value() const;
template<> void AttributeEditor<float>::Initialize();
template<> void AttributeEditor<float>::Update();
template<> void AttributeEditor<float>::OnUIChanged(StringHash eventType, VariantMap &eventData);
template<> void AttributeEditor<float>::SetValue();

template<> void AttributeEditor<int>::SetValue(int value);
template<> int AttributeEditor<int>::Value() const;
template<> void AttributeEditor<int>::Initialize();
template<> void AttributeEditor<int>::Update();
template<> void AttributeEditor<int>::OnUIChanged(StringHash eventType, VariantMap &eventData);
template<> void AttributeEditor<int>::SetValue();

template<> void AttributeEditor<Transform>::SetValue(Transform value);
template<> Transform AttributeEditor<Transform>::Value() const;
template<> void AttributeEditor<Transform>::Initialize();
template<> void AttributeEditor<Transform>::Update();
template<> void AttributeEditor<Transform>::OnUIChanged(StringHash eventType, VariantMap &eventData);
template<> void AttributeEditor<Transform>::SetValue();

template<> void AttributeEditor<float3>::SetValue(float3 value);
template<> float3 AttributeEditor<float3>::Value() const;
template<> void AttributeEditor<float3>::Initialize();
template<> void AttributeEditor<float3>::Update();
template<> void AttributeEditor<float3>::OnUIChanged(StringHash eventType, VariantMap &eventData);
template<> void AttributeEditor<float3>::SetValue();

template<> void AttributeEditor<Color>::SetValue(Color value);
template<> Color AttributeEditor<Color>::Value() const;
template<> void AttributeEditor<Color>::Initialize();
template<> void AttributeEditor<Color>::Update();
template<> void AttributeEditor<Color>::OnUIChanged(StringHash eventType, VariantMap &eventData);
template<> void AttributeEditor<Color>::SetValue();

template<> void AttributeEditor<String>::SetValue(String value);
template<> String AttributeEditor<String>::Value() const;
template<> void AttributeEditor<String>::Initialize();
template<> void AttributeEditor<String>::Update();
template<> void AttributeEditor<String>::OnUIChanged(StringHash eventType, VariantMap &eventData);
template<> void AttributeEditor<String>::SetValue();

template<> void AttributeEditor<EntityReference>::SetValue(EntityReference value);
template<> EntityReference AttributeEditor<EntityReference>::Value() const;
template<> void AttributeEditor<EntityReference>::Initialize();
template<> void AttributeEditor<EntityReference>::Update();
template<> void AttributeEditor<EntityReference>::OnUIChanged(StringHash eventType, VariantMap &eventData);
template<> void AttributeEditor<EntityReference>::SetValue();

template<> void AttributeEditor<AssetReference>::SetValue(AssetReference value);
template<> AssetReference AttributeEditor<AssetReference>::Value() const;
template<> void AttributeEditor<AssetReference>::Initialize();
template<> void AttributeEditor<AssetReference>::Update();
template<> void AttributeEditor<AssetReference>::OnUIChanged(StringHash eventType, VariantMap &eventData);
template<> void AttributeEditor<AssetReference>::SetValue();

template<> void AttributeEditor<AssetReferenceList>::SetValue(AssetReferenceList value);
template<> AssetReferenceList AttributeEditor<AssetReferenceList>::Value() const;
template<> void AttributeEditor<AssetReferenceList>::Initialize();
template<> void AttributeEditor<AssetReferenceList>::Update();
template<> void AttributeEditor<AssetReferenceList>::OnUIChanged(StringHash eventType, VariantMap &eventData);
template<> void AttributeEditor<AssetReferenceList>::SetValue();

}
