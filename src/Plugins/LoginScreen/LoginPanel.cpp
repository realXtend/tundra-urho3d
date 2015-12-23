// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "Win.h"
#include "LoginPanel.h"

#include "Framework.h"
#include "ConfigAPI.h"
#include "FrameAPI.h"
#include "CoreStringUtils.h"
#include "LoggingFunctions.h"
#include "TundraLogic.h"
#include "Client.h"

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Graphics/GraphicsEvents.h>
#include <Urho3D/UI/UIEvents.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Core/Profiler.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/BorderImage.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/ScrollView.h>
#include <Urho3D/UI/LineEdit.h>
#include <Urho3D/UI/DropDownList.h>

using namespace Urho3D;

namespace Tundra
{

LoginPanel::LoginPanel(Framework *framework) :
    Object(framework->GetContext()),
    framework_(framework)
{
    framework_->Frame()->DelayedExecute(0.0f).Connect(this, &LoginPanel::CreateMenu);
}

LoginPanel::~LoginPanel()
{
    if (menuRoot.NotNull())
        menuRoot->Remove();
}

void LoginPanel::Show()
{
    if (menuRoot.NotNull())
        menuRoot->SetVisible(true);
}

void LoginPanel::Hide()
{
    if (menuRoot.NotNull())
        menuRoot->SetVisible(false);
}

void LoginPanel::CreateMenu(float /*time*/)
{
    XMLFile *style = context_->GetSubsystem<ResourceCache>()->GetResource<XMLFile>("Data/UI/LoginStyle.xml");

    LogWarning("Creating login menu!");
    LogWarning(String(style != NULL));
    LogWarning(GetSubsystem<UI>()->GetRoot()->GetSize().ToString());

    menuRoot = new UIElement(context_);
    menuRoot->SetPriority(10);
    menuRoot->SetVisible(true);
    menuRoot->SetSize(GetSubsystem<UI>()->GetRoot()->GetSize());
    GetSubsystem<UI>()->GetRoot()->AddChild(menuRoot);

    SharedPtr<BorderImage> menu(new BorderImage(context_));
    menu->SetStyle("LoginMenu", style);
    menu->SetLayoutMode(LayoutMode::LM_VERTICAL);
    menu->SetSize(IntVector2(440, 220));
    menu->SetAlignment(HA_CENTER, VA_CENTER);
    menu->SetLayoutSpacing(12);
    menuRoot->AddChild(menu);
    
    { // Menu Content
        SharedPtr<BorderImage> logoArea(new BorderImage(context_));
        logoArea->SetName("LogoArea");
        logoArea->SetStyle("TundraLogo", style);
        logoArea->SetMinSize(IntVector2(0, 100));
        logoArea->SetMaxSize(IntVector2(400, 100));
        logoArea->SetHorizontalAlignment(HA_CENTER);
        logoArea->SetImageRect(IntRect(-2, -7, 334, 77));
        menu->AddChild(logoArea);

        { // Login Info
            SharedPtr<UIElement> loginArea(new UIElement(context_));
            loginArea->SetName("LoginArea");
            loginArea->SetLayoutBorder(IntRect(5, 0, 5, 0));
            loginArea->SetLayoutMode(LayoutMode::LM_VERTICAL);
            loginArea->SetMaxSize(IntVector2(400, 2000));
            loginArea->SetHorizontalAlignment(HA_CENTER);
            loginArea->SetLayoutSpacing(2);
            menu->AddChild(loginArea);

            { // Server Address Area
                SharedPtr<UIElement> serverAddressArea(new UIElement(context_));
                serverAddressArea->SetName("ServerAddressArea");
                serverAddressArea->SetLayoutBorder(IntRect(5, 0, 5, 0));
                serverAddressArea->SetMinSize(IntVector2(0, 22));
                serverAddressArea->SetMaxSize(IntVector2(2000, 22));
                serverAddressArea->SetLayoutMode(LayoutMode::LM_HORIZONTAL);
                loginArea->AddChild(serverAddressArea);

                SharedPtr<Text> serverAddressLabel(new Text(context_));
                serverAddressLabel->SetName("ServerAddressLabel");
                serverAddressLabel->SetStyle("TextMonospace", style);
                serverAddressLabel->SetText("Server Address");
                serverAddressArea->AddChild(serverAddressLabel);

                SharedPtr<LineEdit> serverAddressLineEdit(new LineEdit(context_));
                serverAddressLineEdit->SetName("ServerAddressLineEdit");
                serverAddressLineEdit->SetStyle("LineEdit", style);
                serverAddressArea->AddChild(serverAddressLineEdit);
                address = serverAddressLineEdit;
            }

            { // UserName Area
                SharedPtr<UIElement> UserNameArea(new UIElement(context_));
                UserNameArea->SetName("UserNameArea");
                UserNameArea->SetLayoutBorder(IntRect(5, 0, 5, 0));
                UserNameArea->SetMinSize(IntVector2(0, 22));
                UserNameArea->SetMaxSize(IntVector2(2000, 22));
                UserNameArea->SetLayoutMode(LayoutMode::LM_HORIZONTAL);
                loginArea->AddChild(UserNameArea);

                SharedPtr<Text> UserNameLabel(new Text(context_));
                UserNameLabel->SetName("UserNameLabel");
                UserNameLabel->SetStyle("TextMonospace", style);
                UserNameLabel->SetText("Username");
                UserNameArea->AddChild(UserNameLabel);

                SharedPtr<LineEdit> UserNameLineEdit(new LineEdit(context_));
                UserNameLineEdit->SetName("UserNameEdit");
                UserNameLineEdit->SetStyle("LineEdit", style);
                UserNameArea->AddChild(UserNameLineEdit);
                username = UserNameLineEdit;
            }

            { // Password Area
                SharedPtr<UIElement> PasswordArea(new UIElement(context_));
                PasswordArea->SetName("UserNameArea");
                PasswordArea->SetLayoutBorder(IntRect(5, 0, 5, 0));
                PasswordArea->SetMinSize(IntVector2(0, 22));
                PasswordArea->SetMaxSize(IntVector2(2000, 22));
                PasswordArea->SetLayoutMode(LayoutMode::LM_HORIZONTAL);
                loginArea->AddChild(PasswordArea);

                SharedPtr<Text> PasswordLabel(new Text(context_));
                PasswordLabel->SetName("UserNameLabel");
                PasswordLabel->SetStyle("TextMonospace", style);
                PasswordLabel->SetText("Password");
                PasswordArea->AddChild(PasswordLabel);

                SharedPtr<LineEdit> PasswordLineEdit(new LineEdit(context_));
                PasswordLineEdit->SetName("UserNameEdit");
                PasswordLineEdit->SetStyle("LineEdit", style);
                PasswordLineEdit->SetEchoCharacter('*');
                PasswordArea->AddChild(PasswordLineEdit);
                password = PasswordLineEdit;
            }
        }

        SharedPtr<UIElement> buttonArea(new UIElement(context_));
        buttonArea->SetName("ButtonArea");
        buttonArea->SetMinSize(IntVector2(0, 32));
        buttonArea->SetMaxSize(IntVector2(400, 32));
        buttonArea->SetLayoutMode(LayoutMode::LM_FREE);
        menu->AddChild(buttonArea);

        { // Connect Button
            SharedPtr<Button> loginButton(new Button(context_));
            loginButton->SetName("LoginButton");
            loginButton->SetPosition(IntVector2(-80, -3));
            loginButton->SetHorizontalAlignment(HorizontalAlignment::HA_RIGHT);
            buttonArea->AddChild(loginButton);
            this->loginButton = loginButton;
            SubscribeToEvent(loginButton.Get(), E_RELEASED, URHO3D_HANDLER(LoginPanel, OnConnectPressed));

            SharedPtr<Text> buttonLabel(new Text(context_));
            buttonLabel->SetText("Connect");
            buttonLabel->SetInternal(true);
            loginButton->AddChild(buttonLabel);
            
            loginButton->SetStyle("Button", style);
        }

        { // Exit Button
            SharedPtr<Button> exitButton(new Button(context_));
            exitButton->SetName("ExitButton");
            exitButton->SetPosition(IntVector2(6, -3));
            exitButton->SetHorizontalAlignment(HorizontalAlignment::HA_RIGHT);
            buttonArea->AddChild(exitButton);
            this->exitButton = exitButton;
            SubscribeToEvent(exitButton.Get(), E_RELEASED, URHO3D_HANDLER(LoginPanel, OnExitPressed));

            SharedPtr<Text> buttonLabel(new Text(context_));
            buttonLabel->SetText("Exit");
            buttonLabel->SetInternal(true);
            exitButton->AddChild(buttonLabel);

            exitButton->SetStyle("Button", style);
        }

        { // Protocol drop menu
            protocol = new DropDownList(context_);
            protocol->SetName("ProtocolDropDownList");
            protocol->SetSize(IntVector2(80, 22));
            protocol->SetPosition(IntVector2(30, -3));
            protocol->SetHorizontalAlignment(HorizontalAlignment::HA_LEFT);
            protocol->SetResizePopup(true);
            buttonArea->AddChild(protocol);
            protocol->SetStyle("DropDownList", style);

            Text *text = new Text(context_);
            text->SetText("udp");
            text->SetStyleAuto();
            //text->SetStyle("Text", style);
            text->SetSize(IntVector2(80, 22));
            text->SetSelectionColor(Color(0.5, 1.0, 0.5, 0.5));
            text->SetHoverColor(Color(0.5, 0.5, 1.0, 0.5));
            protocol->AddItem(text);

            text = new Text(context_);
            text->SetText("tcp");
            text->SetStyleAuto();
            //text->SetStyle("Text", style);
            text->SetSize(IntVector2(80, 22));
            text->SetSelectionColor(Color(0.5, 1.0, 0.5, 0.5));
            text->SetHoverColor(Color(0.5, 0.5, 1.0, 0.5));
            protocol->AddItem(text);
        }
    }
}

bool LoginPanel::ReadLoginInformation(LoginPanel::LoginInformation &info)
{
    if (address.Null())
        return false;

    // Check if server address is containing port parameter
    Vector<String> addressData = address->GetText().Trimmed().Split(':');
    if (addressData.Size() == 0)
        return false;
    
    for (int i = 0; i < addressData.Size(); ++i)
        LogWarning(String(i) + " " + addressData[i]);

    info.serverAddress = addressData[0].Length() > 0 ? addressData[0] : "127.0.0.1";
    info.port = addressData.Size() > 1 ? ToUInt(addressData[1]) : 2345;
    info.username = username->GetText();
    info.protocol = "udp";
    Text *selectedElement = static_cast<Text*>(protocol->GetSelectedItem());
    if (selectedElement != NULL)
        info.protocol = selectedElement->GetText();

    return true;
}

void LoginPanel::OnConnectPressed(StringHash /*eventType*/, VariantMap& /*eventData*/)
{
    LoginInformation connectionInfo;
    if (ReadLoginInformation(connectionInfo))
    {
        TundraLogic *logic = framework_->Module<TundraLogic>();
        if (logic == NULL)
            return;

        SharedPtr<Client> clientPtr = logic->Client();
        if (clientPtr.Null())
            return;

        clientPtr->Login(connectionInfo.serverAddress, (unsigned short)connectionInfo.port, connectionInfo.username, password->GetText(), connectionInfo.protocol);
        password->SetText("");
    }
}

void LoginPanel::OnExitPressed(StringHash /*eventType*/, VariantMap& /*eventData*/)
{
    framework_->Exit();
}

}