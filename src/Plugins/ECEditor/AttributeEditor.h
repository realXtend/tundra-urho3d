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

class ECEDITOR_API IAttributeEditor : public Object
{
    URHO3D_OBJECT(IAttributeEditor, Object);

public:
    explicit IAttributeEditor(Framework *framework);
    virtual ~IAttributeEditor();

    virtual const String &Title() const;
    virtual void SetTitle(const String &text);

    UIElementPtr Widget() const;

protected:
    virtual void PreInitialize();
    virtual void Initialize();
    virtual void Update();

    UIElementPtr root_;
    TextPtr title_;

    Variant value_;
    Framework *framework_;
    VariantMap data_;
};

template <typename T>
class AttributeEditor : public IAttributeEditor
{
    URHO3D_OBJECT(AttributeEditor, IAttributeEditor);

public:
    AttributeEditor(Framework *framework) :
        IAttributeEditor(framework)
    {
        PreInitialize();
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
};

template<> void AttributeEditor<bool>::SetValue(const bool &value);
template<> const bool &AttributeEditor<bool>::Value() const;
template<> void AttributeEditor<bool>::Initialize();
template<> void AttributeEditor<bool>::Update();

template<> void AttributeEditor<Transform>::SetValue(const Transform &value);
template<> const Transform &AttributeEditor<Transform>::Value() const;
template<> void AttributeEditor<Transform>::Initialize();
template<> void AttributeEditor<Transform>::Update();

template<> void AttributeEditor<Vector3>::SetValue(const Vector3 &value);
template<> const Vector3 &AttributeEditor<Vector3>::Value() const;
template<> void AttributeEditor<Vector3>::Initialize();
template<> void AttributeEditor<Vector3>::Update();

template<> void AttributeEditor<String>::SetValue(const String &value);
template<> const String &AttributeEditor<String>::Value() const;
template<> void AttributeEditor<String>::Initialize();
template<> void AttributeEditor<String>::Update();

}
