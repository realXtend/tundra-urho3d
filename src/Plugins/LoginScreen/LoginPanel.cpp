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
    if (menuRoot_.NotNull())
        menuRoot_->Remove();
}

void LoginPanel::Show()
{
    if (menuRoot_.NotNull())
    {
        menuRoot_->SetVisible(true);
        ShowMessage("");
    }
}

void LoginPanel::Hide()
{
    if (menuRoot_.NotNull())
        menuRoot_->SetVisible(false);
}

void LoginPanel::ShowMessage(const String& message)
{
    if (messageText_.Null())
        return;

    // Cut the message string if its too long for the ui panel to display.
    if (message.Length() > 112)
        messageText_->SetText(message.Substring(0, 112));
    else
        messageText_->SetText(message);
}

void LoginPanel::HideMessage(float /*time*/)
{
    ShowMessage("");
}

void LoginPanel::CreateMenu(float /*time*/)
{
    XMLFile *style = context_->GetSubsystem<ResourceCache>()->GetResource<XMLFile>("Data/UI/LoginStyle.xml");

    menuRoot_ = new UIElement(context_);
    menuRoot_->SetPriority(10);
    menuRoot_->SetVisible(true);
    menuRoot_->SetAlignment(HA_CENTER, VA_CENTER);
    menuRoot_->SetSize(GetSubsystem<UI>()->GetRoot()->GetSize());
    GetSubsystem<UI>()->GetRoot()->AddChild(menuRoot_);

    BorderImage *menu = new BorderImage(context_);
    menu->SetStyle("LoginMenu", style);
    menu->SetLayoutMode(LayoutMode::LM_VERTICAL);
    menu->SetSize(IntVector2(440, 220));
    menu->SetMaxSize(IntVector2(440, 300));
    menu->SetAlignment(HA_CENTER, VA_CENTER);
    menu->SetLayoutSpacing(12);
    menuRoot_->AddChild(menu);
    
    { // Menu Content
        BorderImage *logoArea = new BorderImage(context_);
        logoArea->SetName("LogoArea");
        logoArea->SetStyle("TundraLogo", style);
        logoArea->SetMinSize(IntVector2(0, 100));
        logoArea->SetMaxSize(IntVector2(400, 100));
        logoArea->SetHorizontalAlignment(HA_CENTER);
        logoArea->SetImageRect(IntRect(-2, -7, 334, 77));
        menu->AddChild(logoArea);

        { // Login Info
            UIElement *loginArea = new UIElement(context_);
            loginArea->SetName("LoginArea");
            loginArea->SetLayoutBorder(IntRect(5, 0, 5, 0));
            loginArea->SetLayoutMode(LayoutMode::LM_VERTICAL);
            loginArea->SetMaxSize(IntVector2(400, 2000));
            loginArea->SetHorizontalAlignment(HA_CENTER);
            loginArea->SetLayoutSpacing(2);
            menu->AddChild(loginArea);

            { // Server Address Area
                UIElement *serverAddressArea = new UIElement(context_);
                serverAddressArea->SetName("ServerAddressArea");
                serverAddressArea->SetLayoutBorder(IntRect(5, 0, 5, 0));
                serverAddressArea->SetMinSize(IntVector2(0, 22));
                serverAddressArea->SetMaxSize(IntVector2(2000, 22));
                serverAddressArea->SetLayoutMode(LayoutMode::LM_HORIZONTAL);
                loginArea->AddChild(serverAddressArea);

                Text *serverAddressLabel = new Text(context_);
                serverAddressLabel->SetName("ServerAddressLabel");
                serverAddressLabel->SetStyle("TextMonospace", style);
                serverAddressLabel->SetText("Server Address");
                serverAddressArea->AddChild(serverAddressLabel);

                address_ = new LineEdit(context_);
                address_->SetName("ServerAddressLineEdit");
                address_->SetStyle("LineEdit", style);
                serverAddressArea->AddChild(address_);
            }

            { // UserName Area
                UIElement *UserNameArea = new UIElement(context_);
                UserNameArea->SetName("UserNameArea");
                UserNameArea->SetLayoutBorder(IntRect(5, 0, 5, 0));
                UserNameArea->SetMinSize(IntVector2(0, 22));
                UserNameArea->SetMaxSize(IntVector2(2000, 22));
                UserNameArea->SetLayoutMode(LayoutMode::LM_HORIZONTAL);
                loginArea->AddChild(UserNameArea);

                Text *UserNameLabel = new Text(context_);
                UserNameLabel->SetName("UserNameLabel");
                UserNameLabel->SetStyle("TextMonospace", style);
                UserNameLabel->SetText("Username");
                UserNameArea->AddChild(UserNameLabel);

                username_ = new LineEdit(context_);
                username_->SetName("UserNameEdit");
                username_->SetStyle("LineEdit", style);
                UserNameArea->AddChild(username_);
            }

            { // Password Area
                UIElement *PasswordArea = new UIElement(context_);
                PasswordArea->SetName("UserNameArea");
                PasswordArea->SetLayoutBorder(IntRect(5, 0, 5, 0));
                PasswordArea->SetMinSize(IntVector2(0, 22));
                PasswordArea->SetMaxSize(IntVector2(2000, 22));
                PasswordArea->SetLayoutMode(LayoutMode::LM_HORIZONTAL);
                loginArea->AddChild(PasswordArea);

                Text *PasswordLabel = new Text(context_);
                PasswordLabel->SetName("UserNameLabel");
                PasswordLabel->SetStyle("TextMonospace", style);
                PasswordLabel->SetText("Password");
                PasswordArea->AddChild(PasswordLabel);

                password_ = new LineEdit(context_);
                password_->SetName("UserNameEdit");
                password_->SetStyle("LineEdit", style);
                password_->SetEchoCharacter('*');
                PasswordArea->AddChild(password_);
            }
        }

        messages_ = new UIElement(context_);
        messages_->SetName("MessageArea");
        messages_->SetMinHeight(48);
        messages_->SetMaxSize(IntVector2(380, 48));
        messages_->SetLayoutMode(LayoutMode::LM_VERTICAL);
        messages_->SetAlignment(HA_CENTER, VA_TOP);
        menu->AddChild(messages_);

        {
            messageText_ = new Text(context_);
            messageText_->SetName("MessageLabel");
            messageText_->SetStyle("TextMonospace", style);
            messageText_->SetVerticalAlignment(VA_TOP);
            messages_->AddChild(messageText_);
            messageText_->SetWordwrap(true);
        }

        UIElement *buttonArea = new UIElement(context_);
        buttonArea->SetName("ButtonArea");
        buttonArea->SetMinSize(IntVector2(0, 32));
        buttonArea->SetMaxSize(IntVector2(400, 32));
        buttonArea->SetLayoutMode(LayoutMode::LM_FREE);
        menu->AddChild(buttonArea);

        { // Connect Button
            loginButton_ = new Button(context_);
            loginButton_->SetName("LoginButton");
            loginButton_->SetPosition(IntVector2(-80, -3));
            loginButton_->SetHorizontalAlignment(HorizontalAlignment::HA_RIGHT);
            buttonArea->AddChild(loginButton_);
            SubscribeToEvent(loginButton_.Get(), E_RELEASED, URHO3D_HANDLER(LoginPanel, OnConnectPressed));

            Text *buttonLabel = new Text(context_);
            buttonLabel->SetText("Connect");
            buttonLabel->SetInternal(true);
            loginButton_->AddChild(buttonLabel);

            loginButton_->SetStyle("Button", style);
        }

        { // Exit Button
            exitButton_ = new Button(context_);
            exitButton_->SetName("ExitButton");
            exitButton_->SetPosition(IntVector2(6, -3));
            exitButton_->SetHorizontalAlignment(HorizontalAlignment::HA_RIGHT);
            buttonArea->AddChild(exitButton_);
            SubscribeToEvent(exitButton_, E_RELEASED, URHO3D_HANDLER(LoginPanel, OnExitPressed));

            Text *buttonLabel = new Text(context_);
            buttonLabel->SetText("Exit");
            buttonLabel->SetInternal(true);
            exitButton_->AddChild(buttonLabel);

            exitButton_->SetStyle("Button", style);
        }

        { // Protocol drop menu
            protocol_ = new DropDownList(context_);
            protocol_->SetName("ProtocolDropDownList");
            protocol_->SetSize(IntVector2(80, 22));
            protocol_->SetPosition(IntVector2(30, -3));
            protocol_->SetHorizontalAlignment(HorizontalAlignment::HA_LEFT);
            protocol_->SetResizePopup(true);
            buttonArea->AddChild(protocol_);
            protocol_->SetStyle("DropDownList", style);

            Text *text = new Text(context_);
            text->SetText("udp");
            text->SetStyleAuto();
            text->SetSize(IntVector2(80, 22));
            text->SetSelectionColor(Color(0.5, 1.0, 0.5, 0.5));
            text->SetHoverColor(Color(0.5, 0.5, 1.0, 0.5));
            protocol_->AddItem(text);

            text = new Text(context_);
            text->SetText("tcp");
            text->SetStyleAuto();
            text->SetSize(IntVector2(80, 22));
            text->SetSelectionColor(Color(0.5, 1.0, 0.5, 0.5));
            text->SetHoverColor(Color(0.5, 0.5, 1.0, 0.5));
            protocol_->AddItem(text);
        }

    }

    ReadConfig();
    UpdateUI();
}

void LoginPanel::ReadUI()
{
    Vector<String> addressData = address_->GetText().Trimmed().Split(':');
    loginInfo_.serverAddress = addressData[0].Length() > 0 ? addressData[0] : "127.0.0.1";
    loginInfo_.port = addressData.Size() > 1 ? ToUInt(addressData[1]) : 2345;
    loginInfo_.username = username_->GetText();
    loginInfo_.protocol = "udp";
    Text *selectedElement = static_cast<Text*>(protocol_->GetSelectedItem());
    if (selectedElement != NULL)
        loginInfo_.protocol = selectedElement->GetText();
}

void LoginPanel::UpdateUI()
{
    address_->SetText(loginInfo_.serverAddress + ":" + String(loginInfo_.port));
    username_->SetText(loginInfo_.username);

    PODVector<UIElement *> items = protocol_->GetItems();
    Text* textItem;
    for (uint i = 0; i < items.Size(); ++i)
    {
        textItem = dynamic_cast<Text*>(items[i]);
        if (textItem != NULL && loginInfo_.protocol == textItem->GetText())
        {
            protocol_->SetSelection(i);
            break;
        }
    }
}

void LoginPanel::WriteConfig()
{
    HashMap<String, Variant> data;
    ConfigFile &f = framework_->Config()->GetFile(ConfigAPI::FILE_FRAMEWORK);
    data["login_server"] = loginInfo_.serverAddress + ":" + String(loginInfo_.port);
    data["login_username"] = loginInfo_.username;
    data["login_protocol"] = loginInfo_.protocol;
    f.Set(ConfigAPI::SECTION_CLIENT, data);
}

void LoginPanel::ReadConfig()
{
    ConfigFile &f = framework_->Config()->GetFile(ConfigAPI::FILE_FRAMEWORK);

    String server_address = f.Get(ConfigAPI::SECTION_CLIENT, "login_server", "127.0.0.1").ToString();
    Vector<String> addressData = server_address.Trimmed().Split(':');
    loginInfo_.serverAddress = addressData[0];
    loginInfo_.port = 2345;
    if (addressData.Size() > 1)
        loginInfo_.port = ToUInt(addressData[1]);

    String username = f.Get(ConfigAPI::SECTION_CLIENT, "login_username", "").ToString();
    loginInfo_.username = username;

    String protocol = f.Get(ConfigAPI::SECTION_CLIENT, "login_protocol", "udp").ToString();
    loginInfo_.protocol = protocol;
}

void LoginPanel::OnConnectPressed(StringHash /*eventType*/, VariantMap& /*eventData*/)
{
    ReadUI();

    TundraLogic *logic = framework_->Module<TundraLogic>();
    if (logic == NULL)
        return;

    SharedPtr<Client> clientPtr = logic->Client();
    if (clientPtr.Null())
        return;

    ShowMessage("Connecting...");
    clientPtr->Login(loginInfo_.serverAddress, (unsigned short)loginInfo_.port, loginInfo_.username, password_->GetText(), loginInfo_.protocol);
    clientPtr->LoginFailed.Connect(this, &LoginPanel::OnConnectionFailed);
    password_->SetText("");
}

void LoginPanel::OnConnectionFailed(const String& reason)
{
    TundraLogic *logic = framework_->Module<TundraLogic>();
    if (logic == NULL)
        return;

    SharedPtr<Client> clientPtr = logic->Client();
    if (clientPtr.Null())
        return;

    ShowMessage("Failed to connect: " + clientPtr->LoginProperty("LoginFailed").ToString());

    framework_->Frame()->DelayedExecute(5).Connect(this, &LoginPanel::HideMessage);
}

void LoginPanel::OnExitPressed(StringHash /*eventType*/, VariantMap& /*eventData*/)
{
    framework_->Exit();
}

}