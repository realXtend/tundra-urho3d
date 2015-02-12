// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "KeyEvent.h"
#include "InputAPI.h"

namespace Tundra
{

KeyEvent::KeyEvent(Object *owner) :
    Object(owner->GetContext()),
    keyCode(0),
    keyPressCount(0),
    modifiers(0),
    eventType(KeyEventInvalid),
    handled(false),
    timestamp(0.f),
    sequence(0)
{
}

}
