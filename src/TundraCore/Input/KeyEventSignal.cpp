// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "KeyEventSignal.h"
#include "InputContext.h"


namespace Tundra
{

KeyEventSignal::KeyEventSignal(InputContext* owner, KeySequence sequence) : Object(owner->GetContext()), keySequence(sequence)
{
}

}
