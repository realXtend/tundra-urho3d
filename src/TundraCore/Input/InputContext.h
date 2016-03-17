// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"
#include "InputFwd.h"
#include "CoreTypes.h"
#include "KeyEvent.h"
#include "Signals.h"

#include <Urho3D/Core/Object.h>
#include <Urho3D/Container/HashSet.h>

namespace Tundra
{

/// Holds information about pressed key.
struct TUNDRACORE_API KeyPressInformation
{
    /// Identifies the press count for the key. 0 denotes the key is not being held down.
    /// 1 means that the key is being held down, and no key repeat signals have yet occurred.
    /// A value > 1 means that the key has been held down for a longer period, and this field
    /// tells how many repeated presses have been received already for the key.
    int keyPressCount;

    /// Specifies in which state the key is in.
    KeyEvent::EventType keyState;

    /// The absolute timestamp (in seconds) telling when the key was first pressed down.
    float firstPressTime;
};
typedef HashMap<Key, KeyPressInformation> HeldKeysMap;

/// Provides clients with input events in a priority order.
class TUNDRACORE_API InputContext : public Object
{
    URHO3D_OBJECT(InputContext, Object);

public:
    InputContext(InputAPI *owner, const char *name, int priority);
    ~InputContext();

    /// Emitted for each key code, for each event type.
    Signal1<KeyEvent*> KeyEventReceived;

    /// Emitted for each mouse event (move, scroll, button press/release).
    Signal1<MouseEvent*> MouseEventReceived;

    /// This signal is emitted when any key is pressed in this context.
    Signal1<KeyEvent*> KeyPressed;

    /// This signal is emitted for each application frame when this key is pressed down in this context.
    Signal1<KeyEvent*> KeyDown;

    /// This signal is emitted when any key is released in this context.
    Signal1<KeyEvent*> KeyReleased;

    /// Emitted when the mouse cursor is moved, independent of whether any buttons are down.
    Signal1<MouseEvent*> MouseMove;

    /// Mouse wheel was scrolled.
    Signal1<MouseEvent*> MouseScroll;

    /// Mouse double click
    Signal1<MouseEvent*> MouseDoubleClicked;

    // The following signals are emitted on the appropriate events. It is guaranteed that each press event
    // will follow a corresponding release event (although if it gets lost from Qt, it might get delayed
    // until we actually notice it).
    Signal1<MouseEvent*> MouseLeftPressed;
    Signal1<MouseEvent*> MouseMiddlePressed;
    Signal1<MouseEvent*> MouseRightPressed;

    Signal1<MouseEvent*> MouseLeftReleased;
    Signal1<MouseEvent*> MouseMiddleReleased;
    Signal1<MouseEvent*> MouseRightReleased;

    /// Creates a new signal object that will be triggered when the given
    /// key sequence occurs in this context. To actually receive the events,
    /// connect to one of the member signals of the returned object.
    /// You do not need to hold on to the received signal
    /// object, it will be remembered and freed along with the context.
    KeyEventSignal *RegisterKeyEvent(KeySequence keySequence);

    /// Stops signals from being triggered for the given key sequence in this context.
    /// This is optional. KeyEventSignals are freed properly when the context is destroyed.
    void UnregisterKeyEvent(KeySequence keySequence);

    /// Returns the user-defined name of this InputContext. [property]
    /** The name cannot be used as an unique identifier, it is only present for human-readable purposes. */
    String Name() const { return name; }

    /// Sets the name of this InputContext. The name cannot be used as an unique identifier, it is only
    /// present for human-readable purposes.
    void SetName(const String &name_) { name = name_; }

    /// Adjusts the priority of this input context.
    void SetPriority(int newPriority);

    /// Tests whether the given key was pressed down in this context.
    /// @return The keypress count for the given keycode. If 0, means that
    /// the key was not pressed down during this frame, or that it is being
    /// held down. If 1, it means that the key was pressed down during this
    /// frame. If > 1, it means that key has been held down for a longer
    /// duration, and key repeat was triggered for it this frame. The value
    /// denotes the repeat count.
    int KeyPressedCount(Key keyCode) const;

    /// A convenience method to test key presses ignoring repeats.
    bool IsKeyPressed(Key keyCode) const { return KeyPressedCount(keyCode) == 1; }

    /// Returns true if the given key is being held down in this context.
    bool IsKeyDown(Key keyCode) const;

    /// Returns true if the given key was released in this context during
    /// this frame.
    bool IsKeyReleased(Key keyCode) const;

    /// Constructs a key release event for the given key and calls TriggerKeyEvent for it.
    /// Use to forcibly release a key that is being held down.
    void TriggerKeyReleaseEvent(Key keyCode);

    /// Tells the InputContext whether the given keyCode is automatically
    /// suppressed. This can be used to avoid having to manually set the 
    /// handled-flag to true for each received event.
    /// @param keyCode The keyboard code to set the suppression state for.
    /// @param isSuppressed If true, events related to this keycode will 
    /// be automatically suppressed after this context has handled them,
    /// without passing the event on to lower layers.
    void SetKeySuppressed(Key keyCode, bool isSuppressed);

    /// Forces all held down keys to be released, and the appropriate release events to be sent.
    void ReleaseAllKeys();

    /// Returns true if the given mouse button is being held down.
    ///\todo Bypasses context and queries mouse state directly from Urho3D. Use events instead.
    bool IsMouseButtonDown(int mouseButton) const;

    /// Returns true if the given mouse button was pressed down during this frame.
    ///\todo Bypasses context and queries mouse state directly from Urho3D. Use events instead.
    bool IsMouseButtonPressed(int mouseButton) const;

    /// Returns true if the given mouse button was released during this frame.
    ///\todo Bypasses context and queries mouse state directly from Urho3D. Use events instead.
    bool IsMouseButtonReleased(int mouseButton) const;

    /// Returns number of active touches [property]
    ///\todo Bypasses context and queries touch state directly from Urho3D. Use events instead.
    unsigned GetNumTouches() const;

    /// Return active touch by index.
    ///\todo Bypasses context and queries touch state directly from Urho3D. Use events instead.
    Urho3D::TouchState* GetTouch(unsigned index) const;

    /// Returns the priority value this context has with respect to the other input contexts. [property]
    /** Higher = more urgent. Used to determine the order in which input is received by the input contexts.
        The priority is assigned when the context is created and may not be changed afterwards. */
    int Priority() const { return priority; }

private:
    /// Updates the buffered key presses. Called by the input API to proceed on to the next input frame.
    void UpdateFrame();

    /// Same as TriggerKeyEvent, but for mouse events.
    void TriggerMouseEvent(MouseEvent &mouse);

    /// This function is called by the InputAPI whenever there is a new
    /// key event for this context to handle. The event is emitted through
    /// all relevant signals. You may call this function yourself to inject 
    /// keyboard events to this context manually.
    void TriggerKeyEvent(KeyEvent &key);

    typedef HashMap<KeySequence, KeyEventSignal*> KeyEventSignalMap;
    /// Stores a signal object for each keyboard key code that the user
    /// has registered a signal-slot connection for.
    KeyEventSignalMap registeredKeyEventSignals;

    /// Stores the set of keycodes that this input automatically suppresses, 
    /// i.e. this context "grabs" these keys and does not pass them forward 
    /// to anyone else.
    HashSet<Key> suppressedKeys;
    
    /// Stores each new key event that is being held down.
    HeldKeysMap newKeyEvents;

    /// Stores a buffered version of all the key pressed. This is to avoid losing any press or
    /// release events in the case of different module Update() orders, or when accessing the
    /// context from Qt event handlers outside Update() cycle.
    HeldKeysMap heldKeysBuffered;

    /// Tests both newKeyEvents and heldKeysBuffered and looks if this context is aware of the
    /// given key being pressed down.
    bool IsKeyDownImmediate(Key keyCode) const;

    String name;

    /// The priority value this context has with respect to the other input contexts. Higher = more urgent. Used
    /// to determine the order in which input is received by the input contexts.
    int priority;

    InputAPI *inputApi;
    friend class InputAPI;
};

}
