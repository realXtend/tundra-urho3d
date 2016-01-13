// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "LoginScreenApi.h"
#include "FrameworkFwd.h"
#include "CoreTypes.h"

#include <Urho3D/Container/Ptr.h>

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
typedef WeakPtr<Urho3D::Text> TextWeakPtr;

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

    /// Write login information to config file.
    void WriteConfig();

    /// Read login information from config file.
    void ReadConfig();

    /// Read login information from the UI and update the LoginInfomation object
    void ReadUI();

    // Read connection information from LoginInfomation object and update the UI
    void UpdateUI();

private:
    /// Display login message in panel.
    void ShowMessage(const String& reason);

    /// Hide login message area
    void HideMessage(float time);

    /// Instantate login menu UIElements
    void CreateMenu(float time);

    /// Data structure for required login information
    struct LoginInformation
    {
        String serverAddress;
        String username;
        String protocol;
        uint port;
    };

    /// Triggered when connect button is pressed
    void OnConnectPressed(StringHash eventType, VariantMap& eventData);

    /// Triggered when exit button is pressed
    void OnExitPressed(StringHash eventType, VariantMap& eventData);

    void OnConnectionFailed(const String& reason);

    UIElementWeakPtr menuRoot_;
    ButtonWeakPtr loginButton_;
    ButtonWeakPtr exitButton_;
    LineEditWeakPtr address_;
    LineEditWeakPtr username_;
    LineEditWeakPtr password_;
    DropListWeakPtr protocol_;
    UIElementWeakPtr messages_;
    TextWeakPtr messageText_;

    LoginInformation loginInfo_;
	Framework *framework_;
};

}