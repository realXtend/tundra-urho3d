// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"
#include "CoreTypes.h"
#include "FrameworkFwd.h"
#include "Math/Point.h"
#include "InputFwd.h"
#include "KeyEvent.h"
#include "MouseEvent.h"
#include "InputContext.h"

#include <Urho3D/Core/Object.h>
#include <Urho3D/Container/List.h>


class Framework;

namespace Tundra
{

/// Provides keyboard and mouse input events.
/** The Input API works with the notion of 'input contexts', which are objects that modules acquire
    to receive input events. The contexts have a priority that determine the order in which the input 
    events are posted.

    Input events are processed in the following order:
        1) Very first, when a new input event is received, it is posted to the top level input context.
           See TopLevelInputContext(). This is already before any Qt widgets get the
           chance to process the event.
        2) If the event is a mouse event that occurs on top of a Qt widget, or the event is a key event
           and a Qt widget has keyboard focus, the event is passed to Qt, and suppressed from going
           further.
        3) The event is posted to all the registered input contexts in their order of priority. See 
           RegisterInputContext().
        4) The event is posted to the system-wide event tree. See KeyEvent, MouseEvent and GestureEvent.

    At any level, the handler may set the handled member of a KeyEvent or MouseEvent to true to suppress
    the event from going forward to the lower levels.

    In addition to the abovementioned methods, a module may use a polling API to directly query input 
    state. This API operates on the input level (1) above. See the functions IsKeyDown, IsKeyPressed, 
    IsKeyReleased, IsMouseButtonDown, IsMouseButtonPressed and IsMouseButtonReleased.

    The InputContext -based API utilizes signals. The polling API can be used by any object that
    has access to InputAPI, and the event tree -based API can be used by all modules. */
class TUNDRACORE_API InputAPI : public Object
{
    URHO3D_OBJECT(InputAPI, Object);

public:
    /// Initializes the API and hooks it into the main application window.
    explicit InputAPI(Framework *owner);

    ~InputAPI();

    void Reset();

    /// Proceeds the input system one application frame forward (Ages all double-buffered input data).
    /// Called internally by the Framework to update the polling Input API. Not for client use.
    void Update(float frametime);

    typedef HashMap<String, KeySequence> KeyBindingMap; ///< Maps an arbitrary string identifier to a key sequence.

    /// Changes the priority of the given input context to the new priority.
    void SetPriority(InputContextPtr inputContext, int newPriority);

    /// Return the current key bindings.
    const KeyBindingMap &KeyBindings() const { return keyboardMappings; }

    /// Sets new set of key bindings.
    void SetKeyBindings(const KeyBindingMap &actionMap) { keyboardMappings = actionMap; }

    /// Sets if held keyboard and mouse keys are released automatically when application is not active (has no active window).
    void SetReleaseInputWhenApplicationInactive(bool releaseInput) { releaseInputWhenApplicationInactive = releaseInput; }

    /// Called internally for each generated KeyEvent.
    /** This function passes the event forward to all registered input contexts. You may generate KeyEvent objects
        yourself and call this function directly to inject a custom KeyEvent to the system. */
    void TriggerKeyEvent(KeyEvent &key);

    /// This is the same as TriggerKeyEvent, but for mouse events.
    void TriggerMouseEvent(MouseEvent &mouse);

    /// This emits gesture events to the input contexts
    ///\todo Implement gestures
    //void TriggerGestureEvent(GestureEvent &gesture);

    Framework *Fw() const { return framework; }

    /// Creates a new input context with the given name.
    /** The name is not an ID, i.e. it does not have to be unique with 
        existing contexts (although it is encouraged). When you no longer need the context, free all refcounts to it.
        Remember to hold on to a shared_ptr of the input context as long as you are using the context. */
    InputContextPtr RegisterInputContext(const String &name, int priority);

    /// Creates an untracked InputContext for use from scripting languages that cannot hold a strong reference to an input context.
    /** Use UnregisterInputContextRaw() to free the input context.
        @todo Rename to more descriptive RegisterUntrackedInputContext */
    InputContext *RegisterInputContextRaw(const String &name, int priority);

    /// Deletes an untracked input context used from scripting languages.
    /** @see RegisterInputContextRaw.
        @todo Rename to more descriptive UnregisterUntrackedInputContext */
    void UnregisterInputContextRaw(const String &name);

    /// Sets the mouse cursor in absolute (the usual default) or relative movement (FPS-like) mode.
    /** @param visible If true, shows mouse cursor and allows free movement. If false, hides the mouse cursor 
                       and switches into relative mouse movement input mode. */
    void SetMouseCursorVisible(bool visible);

    /// @return True if we are in absolute movement mode, and false if we are in relative mouse movement mode.
    bool IsMouseCursorVisible() const;

    /// Returns true if the given key is physically held down.
    /** This is done to the best knowledge of the input API, which may be wrong depending on whether Qt has managed
        to successfully deliver the information). This ignores all the grabs and contexts, e.g. you will get true 
        even if a text edit has focus in a Qt widget.
        @param keyCode Urho3D keycode */
    bool IsKeyDown(Key keyCode) const;

    /// Returns true if the given key was pressed down during this frame.
    /** A frame in this context means a period between two subsequent calls to Update. During a single frame, calling this function
        several times will always return true if the key was pressed down this frame, i.e. the pressed-bit is not
        cleared after the first query.
        Key repeats will not be reported through this function.
        @param keyCode Urho3D keycode */
    bool IsKeyPressed(Key keyCode) const;

    /// Functions like IsKeyPressed, but for key releases.
    /** @param keyCode Urho3D keycode */
    bool IsKeyReleased(Key keyCode) const;

    /// Returns true if the given mouse button is being held down.
    /** This ignores all mousegrabbers and contexts and returns the actual state of the given button.
        @param mouseButton Urho3D code */
    bool IsMouseButtonDown(int mouseButton) const;

    /// Returns true if the given mouse button was pressed down during this frame.
    /** Behaves like IsKeyPressed, but for mouse presses.
        @param mouseButton Urho3D code */
    bool IsMouseButtonPressed(int mouseButton) const;

    /// Returns true if the given mouse button was released during this frame.
    /** @param mouseButton Urho3D code */
    bool IsMouseButtonReleased(int mouseButton) const;

    /// Get mouse movement this frame
    int GetMouseMoveX() const;

    /// Get mouse movement this frame
    int GetMouseMoveY() const;

    /// Returns number of active touches
    unsigned GetNumTouches() const;

    /// Return active touch by index.
    Urho3D::TouchState* GetTouch(unsigned index) const;

    ///\todo Actually ask if touch input hardware is available. Now is set to True once the first gesture
    /// comes to our event filter. Find a way to inspect hardware in the constructor, best would be to find out touch device via qt if at all possible 

    /// Return if input is handling gestures from a touch input device.
    /** If true InputContext is emitting the gesture specific signals. Do not trust this at the moment,
        see todo comment. Just connect to the gesture signals and wait if they are being emitted. */
    bool IsGesturesEnabled() const { return gesturesEnabled; }

    /// Returns the mouse coordinates in local client coordinate frame denoting where the given mouse button was last pressed down.
    /** Note that this does not tell whether the mouse button is currently held down or not. 
        @param mouseButton Urho3D code */
    Point MousePressedPos(int mouseButton) const;

    /// Returns the current mouse position in the main window coordinate space.
    Point MousePos() const;

    /// Returns the highest-priority input context that gets all events first to handle (even before going to Qt widgets).
    /** You may register your own keyboard and mouse handlers in this context and block events from going to the main window
        (by setting the .handled member of the event to true), but be very careful when doing so. */
    InputContext *TopLevelInputContext() { return &topLevelInputContext; }

    /// Associates the given custom action with the given key.
    void SetKeyBinding(const String &actionName, const KeySequence &key);

    /// Returns the key associated with the given action.
    /** @param actionName The custom action name to query. The convention is to use two-part names, separated with a period, i.e.
               "category.name". For example, "Avatar.WalkForward" might control avatar movement.
        If the action does not exist, null sequence is returned. */
    KeySequence KeyBinding(const String &actionName) const;
    /// @overload
    /** @param defaultKey If the action does not exist, the default key sequence is registered for it and returned. */
    KeySequence KeyBinding(const String &actionName, const KeySequence &defaultKey);

    /// Removes a key binding.
    void RemoveKeyBinding(const String &actionName);
    
    /// Sends key release messages for each currently tracked pressed key and clears the record of all pressed keys.
    void SceneReleaseAllKeys();
    
    /// Sends mouse button release messages for each mouse button that was held down.
    void SceneReleaseMouseButtons();

    /// Loads key bindings from config.
    void LoadKeyBindingsFromFile();

    /// Saves current key bindings to config.
    void SaveKeyBindingsToFile();

    /// Prints the list of input contexts, for debugging purposes.
    void DumpInputContexts();

    /// Returns the number of currently active touch points, if touch input is active.
    int NumTouchPoints() const { return numTouchPoints; }

    /// Handle events received from Urho
    void EventFilter(StringHash eventType, VariantMap& eventData);


    Signal1<TouchEvent&> TouchBegin;
    Signal1<TouchEvent&> TouchUpdate;
    Signal1<TouchEvent&> TouchEnd;

private:
    /// Stores all InputContexts that have been registered from a script and can't hold strong refs on their own.
    List<InputContextPtr> untrackedInputContexts;

    /// Goes through the list of input contexts and removes from the list all contexts that have been destroyed.
    void PruneDeadInputContexts();

    /// Goes through a TouchEvent and updates the touch point information.
    void UpdateTouchPoints();

    typedef List<WeakPtr<InputContext> > InputContextList;

    /// Starting from the input context 'start', triggers key release events to that context and all lower ones.
    void TriggerSceneKeyReleaseEvent(InputContextList::Iterator start, Key keyCode);

    // The coordinates in window client coordinate space denoting where the mouse left [0] /middle [1] /right [2] /XButton1 [3] /XButton2 [4] 
    // buttons were pressed down.
    MouseEvent::PressPositions mousePressPositions;

    // The last known mouse coordinates in window client coordinate space. These are not necessarily the coordinates from previous frame,
    // but from the previous Qt mouse input event.
    int lastMouseX;
    int lastMouseY;

    /// If true, the mouse cursor is visible and free to move around as usual.
    /// If false, we use mouse in relative movement mode, meaning we hide the cursor and force it to stay in the middle of the application screen.
    /// In relative mode, only mouse relative coordinate updates are posted as events.
    bool mouseCursorVisible;

    /// If true, the InputContext instances will emit gesture related events. Do not trust this bool at this point, it will only be set to true
    /// once we receive the first QEvent::Gesture type event. This needs to be fixed so that we ask the OS if it has a touch input device.
    /// If you find a way to do this, please fix and remove todo from IsGesturesEnabled() comments too.
    bool gesturesEnabled;

    /// If true, all currently help keyboard and mouse buttons will be released if the application is inactive (no active window).
    /// Set to false if you are implementing input event injection while the application is not guaranteed to be active.
    bool releaseInputWhenApplicationInactive;

    // When mouse mode is transitioned from absolute to relative, we store the mouse coordinates of where that happened so that
    // we can nicely restore the mouse to the original coordinates when relative->absolute transition is made again.
    int mouseFPSModeEnterX;
    int mouseFPSModeEnterY;

    /// Stores all the currently registered input contexts. The order these items are stored in the list corresponds to the
    /// priority at which each context will get the events. Highest priority context is in front of the list, lowest priority is at the back.
    InputContextList registeredInputContexts;

    /// This input context is processed immediately as the first thing in the system. I.e. before even checking whether the
    /// input event should be passed to Qt.
    InputContext topLevelInputContext;

    /// Stores all the keyboard mappings we have gathered.
    KeyBindingMap keyboardMappings;

    /// Stores the currently held down keyboard buttons.
    HashMap<Key, KeyPressInformation> heldKeys;

    /// Lists the keycodes of buttons that are taken to have been pressed down during this Update() cycle.
    Vector<Key> pressedKeys;

    /// Lists the keycodes of buttons that are taken to have been pressed down during this Update() cycle.
    Vector<Key> releasedKeys;

    /// An internal queue where all new Qt keypress events are stored before they are advertised for callers of IsKeyPressed().
    /// This queue is used to avoid key presses being missed between Update() cycles, i.e. so that the module Updates
    /// may get called in any order.
    Vector<Key> newKeysPressedQueue;

    /// An internal queue where all new Qt keyrelease events are stored before they are advertised for callers of IsKeyReleased().
    /// This queue is used to avoid key releases being missed between Update() cycles, i.e. so that the module Updates
    /// may get called in any order.
    Vector<Key> newKeysReleasedQueue;

    /// A bitmask of the currently held down mouse buttons.
    unsigned long heldMouseButtons;
    /// A bitmask for the mouse buttons pressed down this frame.
    unsigned long pressedMouseButtons;
    /// A bitmask for the mouse buttons released this frame.
    unsigned long releasedMouseButtons;

    /// Keep track of key modifiers to be sent with mouse pressed/released
    unsigned long currentModifiers;

    // The following variables double-buffer the mouse button presses and releases, so that a module will not lose these
    // events depending on what order the module Updates are called.
    unsigned long newMouseButtonsPressedQueue;
    unsigned long newMouseButtonsReleasedQueue;

    /// Amount of currently active touch points
    int numTouchPoints;

    Framework *framework;
};
}

