// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"
#include "InputFwd.h"
#include "Signals.h"

#include <Urho3D/Core/Object.h>

namespace Tundra
{

/// A signal object for input events to a specific key on the keyboard.
class TUNDRACORE_API KeyEventSignal : public Object
{
    URHO3D_OBJECT(KeyEventSignal, Object);

    Signal1<KeyEvent*> SequencePressed;
    Signal1<KeyEvent*> SequenceReleased;

public:
    explicit KeyEventSignal(InputContext* owner, KeySequence sequence);

    /// This is the key sequence that this key signal is triggered for.
    const KeySequence keySequence;

    void OnKeyPressed(KeyEvent *key) { SequencePressed.Emit(key); }
    void OnKeyReleased(KeyEvent *key) { SequenceReleased.Emit(key); }
};
}


