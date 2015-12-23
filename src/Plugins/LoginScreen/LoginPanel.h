// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "LoginScreenApi.h"
#include "FrameworkFwd.h"
#include "CoreTypes.h"

#include <Urho3D/Container/Ptr.h>
#include <Urho3D/Container/RefCounted.h>
#include <Urho3D/UI/UIElement.h>

namespace Urho3D
{
    class UIElement;
    class Text;
    class Button;
    class LineEdit;
    class DropDownList;
}

namespace Tundra
{

typedef WeakPtr<Urho3D::UIElement> UIElementWeakPtr;
typedef WeakPtr<Urho3D::Button> ButtonWeakPtr;
typedef WeakPtr<Urho3D::LineEdit> LineEditWeakPtr;
typedef WeakPtr<Urho3D::DropDownList> DropListWeakPtr;

class LOGINSCREEN_API LoginPanel : public Object
{
    URHO3D_OBJECT(LoginPanel, Object);

public:
    /// Construct.
    LoginPanel(Framework *framework);
    /// Destruct.
    ~LoginPanel();
    
    /// Show login menu
    void Show();

    /// Hide login menu
    void Hide();

private:
    /// Instantate login menu UIElements
    void CreateMenu(float time);

    /// Data structure for required login infomation
    struct LoginInformation
    {
        String serverAddress;
        String username;
        String protocol;
        uint port;
    };

    /// Read & validate login information from the UI elements and write it into LoginInfomation object
    bool ReadLoginInformation(LoginInformation &info);

    /// Triggered when connect button is pressed
    void OnConnectPressed(StringHash eventType, VariantMap& eventData);

    /// Triggered when exit button is pressed
    void OnExitPressed(StringHash eventType, VariantMap& eventData);

    UIElementWeakPtr menuRoot;
    ButtonWeakPtr loginButton;
    ButtonWeakPtr exitButton;
    LineEditWeakPtr address;
    LineEditWeakPtr username;
    LineEditWeakPtr password;
    DropListWeakPtr protocol;

	Framework *framework_;
};

}