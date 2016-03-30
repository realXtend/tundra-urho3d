// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"
#include "InputFwd.h"
#include "CoreTypes.h"

#include <Urho3D/Core/Object.h>
#include <Urho3D/Input/InputEvents.h>

namespace Tundra
{

/// This data structure is passed as a parameter in all keyboard-related input events.
class TUNDRACORE_API KeyEvent : public Object
{
    URHO3D_OBJECT(KeyEvent, Object);

public:
    KeyEvent(Object *owner);

    virtual ~KeyEvent() {}
    
    /// Underlying Urho3D event data.
    VariantMap urhoEvent;

    /// The key code associated with this key event.
    /// @note This member stores the pressed key without keyboard modifiers attached.
    Key keyCode;

    /// How many times this key event has been pressed during the time the key has been held down. If 1, this means a new 
    /// keypress. If >1, this means that this event is being raised on a key repeat event, and this field increments by
    /// one for each time the repeat signal is received.
    int keyPressCount;

    /// A bitfield of the keyboard modifiers (Ctrl, Shift, ...) associated with this key event.
    /// Also see \see KeyEvent::HasShiftModifier.
    unsigned long modifiers;

    enum EventType
    {
        KeyEventInvalid = 0, ///< An invalid event. Used to check for improperly filled structure.
        KeyPressed = 1, ///< The key was pressed, or repeated by key repeat. Read the keyPressCount field to distinguish between these.
        KeyDown = 2, ///< The key is being held down. This event is generated once per frame. \todo Not yet implemented.
        KeyReleased = 3, ///< The key was released.
    };

    /// Identifies whether this was a key press or a release event.
    EventType eventType;

    /// The readable textual representation associated with this event, if applicable.
    ///\todo
    String text;

    /// The key sequence that has been produced by this keypress.
    /// @note Only valid/used for KeyPressed events.
    KeySequence sequence;

    /// Contains keycodes for all other keys that are being pressed when this key was pressed.
    /// This member is only valid when eventType==KeyPressed.
    Vector<Key> otherHeldKeys;

    /// This field is used as an accept/suppression flag. When you are handling this event, settings this to true signals that
    /// your module handled this keyevent, and it should not be passed on to further processing.
    bool handled;

    /// Wall clock time when the event occurred.
    float timestamp;
    ///\todo Add hold duration if this is a release/repeated press.

    KeySequence Sequence() const { return sequence; }
    String Text() const { return text; }
    unsigned long Modifiers() const { return modifiers; }
    int KeyPressCount() const { return keyPressCount; }
    Key KeyCode() const { return keyCode; }
    EventType Type() const { return eventType; }
    float Timestamp() const { return timestamp; }

    /// Marks this event as having been handled already, which will suppress this event from
    /// going on to lower input context levels.
    void Suppress() { handled = true; }

    /// Returns true if this event represents a repeated keypress.
    bool IsRepeat() const { return eventType == KeyPressed && keyPressCount > 1; }

    /// A conveniency accessor to otherHeldKeys field.
    /// @return True if the key with the given keycode was held down when this event occurred.
    bool HadKeyDown(int keyCode) const { return otherHeldKeys.Find(keyCode) != otherHeldKeys.End(); }//std::find(otherHeldKeys.begin(), otherHeldKeys.end(), keyCode) != otherHeldKeys.end(); }

    // Conveniency accessors for the keyboard modifiers.
    bool HasShiftModifier() const { return (modifiers & Urho3D::QUAL_SHIFT) != 0; }
    bool HasCtrlModifier() const { return (modifiers & Urho3D::QUAL_CTRL) != 0; }
    bool HasAltModifier() const { return (modifiers & Urho3D::QUAL_ALT) != 0; }

    /// On windows, this is associated with the Win key.
    ///\todo No support for meta key in Urho3D
    bool HasMetaModifier() const { return false; }

    /// Returns the pressed key with the modifier bits in it.
    int KeyWithModifier() const { return (int)keyCode | (int)modifiers; }
};

}

