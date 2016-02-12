// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "Win.h"
#include "LoginScreen.h"
#include "Framework.h"
#include "Entity.h"
#include "TundraLogic.h"
#include "Client.h"
#include "LoginPanel.h"

namespace Tundra
{

LoginScreen::LoginScreen(Framework* owner) :
    IModule("LoginScreen", owner)
{
    Initialize();
}

LoginScreen::~LoginScreen()
{
    if (loginPanel_ != NULL)
        delete loginPanel_;
}

void LoginScreen::Initialize()
{
	// Check if own avatar was created
	TundraLogic* logic = framework->Module<TundraLogic>();
	if (!logic)
		return;

	Client* client = logic->Client();
	if (!client)
		return;

	client->Connected.Connect(this, &LoginScreen::OnConnected);
	client->Disconnected.Connect(this, &LoginScreen::OnDisconnected);
    loginPanel_ = new LoginPanel(framework);
}

void LoginScreen::Uninitialize()
{
	
}

void LoginScreen::OnConnected(UserConnectedResponseData* /*responseData*/)
{
    if (loginPanel_ != NULL)
    {
        loginPanel_->Hide();
        loginPanel_->WriteConfig();
    }
}

void LoginScreen::OnDisconnected()
{
    if (loginPanel_ != NULL)
        loginPanel_->Show();
}

}

extern "C"
{

DLLEXPORT void TundraPluginMain(Tundra::Framework *fw)
{
    fw->RegisterModule(new Tundra::LoginScreen(fw));
}

}
