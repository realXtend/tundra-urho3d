// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "CoreTypes.h"

namespace Urho3D
{
    struct TouchState;
}

namespace Tundra
{

class InputAPI;
class InputContext;
struct KeyPressInformation;
class KeyEvent;
class KeyEventSignal;
class MouseEvent;

typedef SharedPtr<InputContext> InputContextPtr;

typedef int Key;
typedef int KeySequence;
typedef VariantMap TouchEvent;
typedef Urho3D::StringHash UrhoEvent;

}

