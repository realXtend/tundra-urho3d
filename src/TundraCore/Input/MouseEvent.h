// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"
#include "CoreTypes.h"
#include "InputFwd.h"
#include "Math/Point.h"

#include <Urho3D/Core/Object.h>
#include <Urho3D/Input/InputEvents.h>

namespace Tundra
{

/// This data structure is passed as a parameter in all mouse-related input events.
class TUNDRACORE_API MouseEvent : public Object
{
    OBJECT(MouseEvent);

public:
    MouseEvent(InputAPI *owner);
    virtual ~MouseEvent() {}

    /// These correspond to Urho3D enums
    enum MouseButton
    {
        NoButton = 0,
        LeftButton = Urho3D::MOUSEB_LEFT,
        RightButton = Urho3D::MOUSEB_RIGHT,
        MiddleButton = Urho3D::MOUSEB_MIDDLE,
        Button4 = Urho3D::MOUSEB_X1,
        Button5 = Urho3D::MOUSEB_X2,
        MaxButtonMask = 32
    };

    enum EventType
    {
        MouseEventInvalid = 0, ///< An invalid event. Used to check for improperly filled structure.
        MouseMove, ///< The mouse cursor moved. This event is passed independent of which buttons were down.
        MouseScroll, ///< The mouse wheel position moved.
        MousePressed, ///< A mouse button was pressed down.
        MouseReleased, ///< A mouse button was released.
        MouseDoubleClicked
        /// @todo Offer this additional event:
        ///        MouseClicked  ///< A mouse click occurs when mouse button is pressed and released inside a short interval.
        /// @note Must be the last one in the enum so that we don't break scripts or other instances that could be using number instead of the symbol.
    };

    /// PressOrigin tells whether a mouse press originated on top of a Qt widget or on top of the 3D scene area.
    enum PressOrigin
    {
        PressOriginNone = 0, ///< No press of the given type has been registered.
        PressOriginScene,
        PressOriginQtWidget
    };

    EventType eventType;

    /// The button that this event corresponds to.
    MouseButton button;

    /// If eventType==MousePress, this field tells whether the click originates on the 3D scene or on a Qt widget.
    PressOrigin origin;

    // The mouse coordinates in the client coordinate area.
    int x;
    int y;
    /// The mouse wheel absolute position.
    int z;

    // The difference that occurred during this event and the previous one.
    int relativeX;
    int relativeY;
    // The difference in the mouse wheel position (for most mouses).
    int relativeZ;

    // The global mouse coordinates where the event occurred.
    int globalX;
    int globalY;

    /// What other mouse buttons were held down when this event occurred. This is a combination of MouseButton flags.
    unsigned long otherButtons;

    /// A bitfield of the keyboard modifiers (Ctrl, Shift, ...) associated with this key event.
    /// Use Qt::KeyboardModifier, http://qt-project.org/doc/qt-4.8/qt.html#KeyboardModifier-enum to access these.
    /// @sa HasShiftModifier, HasCtrlModifier, HasAltModifier
    unsigned long modifiers;

    /// Which keyboard keys were held down when this event occurred.
    Vector<Key> heldKeys;

    /// The coordinates in window client coordinate space denoting where the mouse buttons were pressed down.
    /** Buttons: left [0], middle [1], right [2], XButton1 [3]  and XButton2 [4].
        These are useful in mouse drag situations where it is necessary to know the coordinates where the mouse dragging started. */
    struct PressPositions
    {
        PressPositions();

        PressOrigin origin[5];

        int x[5];
        int y[5];

        /// Returns the mouse coordinates in local client coordinate frame denoting where the given mouse button was last pressed
        /// down. Note that this does not tell whether the mouse button is currently held down or not.
        Point Pos(int mouseButton) const;
        PressOrigin Origin(int mouseButton) const;

        void Set(int mouseButton, const Point &pt, PressOrigin origin) { Set(mouseButton, pt.x, pt.y, origin); }
        void Set(int mouseButton, int x, int y, PressOrigin origin);
    };

    PressPositions mousePressPositions;

    /// This field is used as an accept/suppression flag. When you are handling this event, settings this to true signals that
    /// your module handled this event, and it should not be passed on to Qt for further processing. Of course you can
    /// leave this as false even after you handle this event, in case you don't care whether Qt gets this event as well or not.
    /// By default, this field is set to false when the event is fired to the event queue.
    bool handled;

    /// Wall clock time when the event occurred.
    float timestamp;

    int X() const { return x; }
    int Y() const { return y; }
    int Z() const { return z; }

    int RelativeX() const { return relativeX; }
    int RelativeY() const { return relativeY; }
    int RelativeZ() const { return relativeZ; }

    int GlobalX() const { return globalX; }
    int GlobalY() const { return globalY; }

    unsigned long OtherButtons() const { return otherButtons; }

    EventType Type() const { return eventType; }
    MouseButton Button() const { return button; }
    PressOrigin Origin() const { return origin; }
    float Timestamp() const { return timestamp; }

    /// Marks this event as having been handled already, which will suppress this event from
    /// going on to lower input context levels.
    void Suppress() { handled = true; }

    /// @return True if the key with the given keycode was held down when this event occurred.
    bool HadKeyDown(Key keyCode) const { return heldKeys.Find(keyCode) != heldKeys.End(); }//  std::find(heldKeys.begin(), heldKeys.end(), keyCode) != heldKeys.end(); }

    /// Returns the mouse coordinates in local client coordinate frame denoting where the given mouse button was last pressed
    /// down. Note that this does not tell whether the mouse button is currently held down or not.
    Point MousePressedPos(int mouseButton) const;

    /// Returns true if the given mouse button is held down in this event.
    bool IsButtonDown(MouseButton button_) const;

    // Conveniency functions for above. Note that these do not tell if this event was a mouse press event for the given button,
    // only that the button was detected to be down during the time this event was generated.
    bool IsLeftButtonDown() const { return IsButtonDown(LeftButton); }
    bool IsMiddleButtonDown() const { return IsButtonDown(MiddleButton); }
    bool IsRightButtonDown() const { return IsButtonDown(RightButton); }

    // Modifier check functions
    bool HasShiftModifier() const { return (modifiers & Urho3D::QUAL_SHIFT) != 0; }
    bool HasCtrlModifier() const { return (modifiers & Urho3D::QUAL_CTRL) != 0; }
    bool HasAltModifier() const { return (modifiers & Urho3D::QUAL_ALT) != 0; }
    ///\todo No support for meta key in Urho3D
    bool HasMetaModifier() const { return false; } // On Windows, this is associated to the Win key.
};

}
