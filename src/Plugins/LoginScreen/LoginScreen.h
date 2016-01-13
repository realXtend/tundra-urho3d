// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IModule.h"
#include "SceneFwd.h"
#include "IAttribute.h"
#include "AttributeChangeType.h"
#include "InputFwd.h"
#include "Signals.h"
#include "LoginScreenApi.h"
#include "CoreStringUtils.h"
#include "UserConnectedResponseData.h"

#include <Urho3D/Input/Input.h>

namespace Urho3D
{
    class UIElement;
}

namespace Tundra
{

class LoginPanel;

/// Simple freelook camera functionality. Creates a camera to scene and allows it to be controlled with mouse & keys
class LOGINSCREEN_API LoginScreen : public IModule
{
    URHO3D_OBJECT(LoginScreen, IModule);

public:
    LoginScreen(Framework* owner);
    ~LoginScreen();

private:
	void Initialize() override;
	void Uninitialize() override;

	void OnConnected(UserConnectedResponseData *responseData);
	void OnDisconnected();

    LoginPanel *loginPanel_;
};

}
