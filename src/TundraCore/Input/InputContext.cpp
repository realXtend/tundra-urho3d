// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "InputContext.h"
#include "InputAPI.h"
#include "KeyEvent.h"
#include "MouseEvent.h"
#include "KeyEventSignal.h"
#include "Framework.h"
#include "FrameAPI.h"

#include <Input.h>

namespace Tundra
{

InputContext::InputContext(InputAPI *owner, const char *name_, int priority_) :
    Object(owner->GetContext()),
    inputApi(owner),
    name(name_),
    priority(priority_)
{
}

InputContext::~InputContext()
{
    for(auto iter = registeredKeyEventSignals.Begin();
        iter != registeredKeyEventSignals.End(); ++iter)
    {
        delete iter->second_;
    }
}

KeyEventSignal *InputContext::RegisterKeyEvent(KeySequence keySequence)
{
    auto iter = registeredKeyEventSignals.Find(keySequence);
    if (iter != registeredKeyEventSignals.End())
        return iter->second_;
    KeyEventSignal *signal = new KeyEventSignal(this, keySequence);
    registeredKeyEventSignals[keySequence] = signal;
    return signal;
}

void InputContext::UnregisterKeyEvent(KeySequence keySequence)
{
    auto iter = registeredKeyEventSignals.Find(keySequence);
    if (iter != registeredKeyEventSignals.End())
    {
        delete iter->second_;
        registeredKeyEventSignals.Erase(iter);
    }
}

void InputContext::TriggerKeyEvent(KeyEvent &key)
{
    auto keySignal = registeredKeyEventSignals.Find(key.sequence);
    switch(key.eventType)
    {
    case KeyEvent::KeyPressed:
        // 1. First emit the generic KeyEventReceived signal that receives all event types for all key codes.
        KeyEventReceived.Emit(&key);
        // 2. Emit the event type -specific signal for all key codes.
        KeyPressed.Emit(&key);
        // 3. Emit the key code -specific signal for specific event.
        if (keySignal != registeredKeyEventSignals.End())
            keySignal->second_->OnKeyPressed(&key);
        break;
    case KeyEvent::KeyDown:
        if (!IsKeyDownImmediate(key.keyCode))
            break; // If we've received a keydown for a key we haven't gotten a corresponding press for before, ignore this event.

        KeyEventReceived.Emit(&key); // 1.
        KeyDown.Emit(&key); // 2.
        break;
    case KeyEvent::KeyReleased:
        if (!IsKeyDownImmediate(key.keyCode))
            break; // If we've received a keydown for a key we haven't gotten a corresponding press for before, ignore this event.

        KeyEventReceived.Emit(&key); // 1.
        KeyReleased.Emit(&key); // 2.
        if (keySignal != registeredKeyEventSignals.End())
            keySignal->second_->OnKeyReleased(&key); // 3.
        break;
    default:
        assert(false);
        break;
    }

    // Update the buffered API.
    KeyPressInformation info;
    info.keyState = key.eventType;
    info.keyPressCount = key.keyPressCount;
    newKeyEvents[key.keyCode] = info;

    // Now if this layer is registered to suppress this keypress from being processed further,
    // mark it handled.
    if (suppressedKeys.Find(key.keyCode) != suppressedKeys.End())
        key.handled = true;
}

void InputContext::TriggerKeyReleaseEvent(Key keyCode)
{
    auto iter = heldKeysBuffered.Find(keyCode);
    // If this key has not even been pressed down in this context, ignore it.
    if (iter == heldKeysBuffered.End())
        return;

    KeyEvent release(this);
    release.keyCode = keyCode;
    release.keyPressCount = iter->second_.keyPressCount;
    release.eventType = KeyEvent::KeyReleased;
    release.timestamp = inputApi->Fw()->Frame()->WallClockTime();
    TriggerKeyEvent(release);
}

void InputContext::TriggerMouseEvent(MouseEvent &mouse)
{
    MouseEventReceived.Emit(&mouse);

    switch(mouse.eventType)
    {
    case MouseEvent::MouseMove: MouseMove.Emit(&mouse); break;
    case MouseEvent::MouseScroll: MouseScroll.Emit(&mouse); break;
    case MouseEvent::MousePressed:
        switch(mouse.button)
        {
        case MouseEvent::LeftButton: MouseLeftPressed.Emit(&mouse); break;
        case MouseEvent::RightButton: MouseRightPressed.Emit(&mouse); break;
        case MouseEvent::MiddleButton: MouseMiddlePressed.Emit(&mouse); break;
            ///\todo XButton1 and XButton2 support?
        }
        break;
    case MouseEvent::MouseReleased: 
        switch(mouse.button)
        {
        case MouseEvent::LeftButton: MouseLeftReleased.Emit(&mouse); break;
        case MouseEvent::RightButton: MouseRightReleased.Emit(&mouse); break;
        case MouseEvent::MiddleButton: MouseMiddleReleased.Emit(&mouse); break;
            ///\todo XButton1 and XButton2 support?
        }
        break;
    case MouseEvent::MouseDoubleClicked: MouseDoubleClicked.Emit(&mouse); break;
    default:
        assert(false);
        break;
    }
}

void InputContext::SetPriority(int newPriority)
{
    assert(inputApi);
    inputApi->SetPriority(InputContextPtr(this), newPriority);
}

int InputContext::KeyPressedCount(Key keyCode) const
{
    auto iter = heldKeysBuffered.Find(keyCode);
    if (iter == heldKeysBuffered.End())
        return 0; // The key is not being held down.

    if (iter->second_.keyState != KeyEvent::KeyPressed && 
        iter->second_.keyState != KeyEvent::KeyDown)
        return 0;

    return iter->second_.keyPressCount;
}

bool InputContext::IsKeyDown(Key keyCode) const
{
    auto iter = heldKeysBuffered.Find(keyCode);
    if (iter == heldKeysBuffered.End())
        return false;

    return iter->second_.keyState == KeyEvent::KeyPressed || 
        iter->second_.keyState == KeyEvent::KeyDown;
}

bool InputContext::IsKeyReleased(Key keyCode) const
{
    auto iter = heldKeysBuffered.Find(keyCode);
    if (iter == heldKeysBuffered.End())
        return false;

    return iter->second_.keyState == KeyEvent::KeyReleased;
}

void InputContext::SetKeySuppressed(Key keyCode, bool isSuppressed)
{
    if (isSuppressed)
        suppressedKeys.Insert(keyCode);
    else
        suppressedKeys.Erase(keyCode);
}

void InputContext::UpdateFrame()
{
    // Update the buffered events.
    auto iter = heldKeysBuffered.Begin();
    while(iter != heldKeysBuffered.End())
    {
        if (iter->second_.keyState == KeyEvent::KeyPressed)
        {
            iter->second_.keyState = KeyEvent::KeyDown;
            ++iter;
        }
        else if (iter->second_.keyState == KeyEvent::KeyReleased)
          {
            HeldKeysMap::Iterator next = iter;
            ++next;
            heldKeysBuffered.Erase(iter);
            iter = next;
          }
        else 
            ++iter;
    }

    // Put all new events to the buffer.
    for(auto iter = newKeyEvents.Begin(); iter != newKeyEvents.End(); ++iter)
        heldKeysBuffered[iter->first_] = iter->second_;

    newKeyEvents.Clear();
}

void InputContext::ReleaseAllKeys()
{
    Vector<Key> keysToRelease;

    // We double buffer the to-be-released keys first to a temporary list, since invoking the trigger
    // function will add new entries to newKeyEvents.

    for(auto iter = heldKeysBuffered.Begin(); iter != heldKeysBuffered.End(); ++iter)
        keysToRelease.Push(iter->first_);

    for(auto iter = newKeyEvents.Begin(); iter != newKeyEvents.End(); ++iter)
        keysToRelease.Push(iter->first_);

    for(auto iter = keysToRelease.Begin(); iter != keysToRelease.End(); ++iter)
        TriggerKeyReleaseEvent(*iter);
}


bool InputContext::IsMouseButtonDown(int mouseButton) const
{
    return GetSubsystem<Urho3D::Input>()->GetMouseButtonDown(mouseButton);
}

bool InputContext::IsMouseButtonPressed(int mouseButton) const
{
    return GetSubsystem<Urho3D::Input>()->GetMouseButtonPress(mouseButton);
}

bool InputContext::IsMouseButtonReleased(int mouseButton) const
{
    return inputApi->IsMouseButtonReleased(mouseButton);
}

unsigned InputContext::GetNumTouches() const
{
    return inputApi->GetNumTouches();
}

Urho3D::TouchState* InputContext::GetTouch(unsigned index) const
{
    return inputApi->GetTouch(index);
}

bool InputContext::IsKeyDownImmediate(Key keyCode) const
{
    auto iter = newKeyEvents.Find(keyCode);
    if (iter != newKeyEvents.End())
        return iter->second_.keyState == KeyEvent::KeyDown ||
            iter->second_.keyState == KeyEvent::KeyPressed;

    iter = heldKeysBuffered.Find(keyCode);
    if (iter != heldKeysBuffered.End())
        return iter->second_.keyState == KeyEvent::KeyDown ||
            iter->second_.keyState == KeyEvent::KeyPressed;

    return false;
}
}

