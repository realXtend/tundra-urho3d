/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   ECEditor.h
    @brief  ECEditor core API. */

#pragma once

#include "ECEditorApi.h"
#include "CoreTypes.h"
#include "FrameworkFwd.h"
#include "Signals.h"

#include <Urho3D/Core/Object.h>
#include <Urho3D/Container/Ptr.h>

namespace Urho3D
{
    class Context;
    class DropDownList;
    class LineEdit;
    class Window;
    class CheckBox;
    class UIElement;
    class Text;
}

using namespace Urho3D;

namespace Tundra
{

typedef SharedPtr<DropDownList> DropDownListPtr;
typedef SharedPtr<LineEdit> LineEditPtr;
typedef SharedPtr<Window> WindowPtr;
typedef SharedPtr<CheckBox> CheckBoxPtr;
typedef SharedPtr<Text> TextPtr;

class ECEDITOR_API AddEntityDialog : public Object
{
    URHO3D_OBJECT(AddEntityDialog, Object);

public:
    explicit AddEntityDialog(Framework *framework);
    virtual ~AddEntityDialog();

    Signal2<AddEntityDialog * ARG(dialog), bool ARG(confirmed)> DialogClosed;

    UIElement *Widget() const;
    String Name() const;
    bool IsTemporary() const;
    bool IsLocal() const;

    void Show();
    void Hide();

protected:
    void OnButtonPressed(StringHash eventType, VariantMap &eventData);
    void OnCheckboxChanged(StringHash eventType, VariantMap &eventData);

    Framework *framework_;
    
    LineEditPtr nameLineEdit_;
    WindowPtr window_;
    CheckBoxPtr localCheckBox_;
    CheckBoxPtr temporaryCheckBox_;
    TextPtr localText_;
};

}
