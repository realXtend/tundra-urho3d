// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

#include "InputAPI.h"
#include "Framework.h"
#include "LoggingFunctions.h"
#include "CoreDefines.h"
#include "ConfigAPI.h"
#include "Profiler.h"
#include "FrameAPI.h"

#include <Input.h>

namespace Tundra
{

// using 'this' in initializer list which is technically UB but safe in our case, as the InputContext constructor just uses the base class to get the Urho context from InputAPI
#pragma warning(disable:4355)

InputAPI::InputAPI(Framework *framework_) :
    Object(framework_->GetContext()),
    lastMouseX(0),
    lastMouseY(0),
    mouseCursorVisible(false),
    gesturesEnabled(false),
    releaseInputWhenApplicationInactive(true),
    mouseFPSModeEnterX(0),
    mouseFPSModeEnterY(0),
    topLevelInputContext(this, "TopLevel", 100000), // The priority value for the top level context does not really matter, just put an arbitrary big value for display.
    heldMouseButtons(0),
    pressedMouseButtons(0),
    releasedMouseButtons(0),
    newMouseButtonsPressedQueue(0),
    newMouseButtonsReleasedQueue(0),
    currentModifiers(0),
    numTouchPoints(0),
    framework(framework_)
{
    if (framework_->IsHeadless())
        return;

    SubscribeToEvent(Urho3D::E_KEYDOWN, HANDLER(InputAPI, EventFilter));
    SubscribeToEvent(Urho3D::E_KEYUP, HANDLER(InputAPI, EventFilter));
    SubscribeToEvent(Urho3D::E_MOUSEBUTTONDOWN, HANDLER(InputAPI, EventFilter));
    SubscribeToEvent(Urho3D::E_MOUSEBUTTONUP, HANDLER(InputAPI, EventFilter));
    SubscribeToEvent(Urho3D::E_MOUSEMOVE, HANDLER(InputAPI, EventFilter));
    SubscribeToEvent(Urho3D::E_MOUSEWHEEL, HANDLER(InputAPI, EventFilter));
    SubscribeToEvent(Urho3D::E_TOUCHBEGIN, HANDLER(InputAPI, EventFilter));
    SubscribeToEvent(Urho3D::E_TOUCHMOVE, HANDLER(InputAPI, EventFilter));
    SubscribeToEvent(Urho3D::E_TOUCHEND, HANDLER(InputAPI, EventFilter));

    LoadKeyBindingsFromFile();
}

InputAPI::~InputAPI()
{
    Reset();
}

void InputAPI::Reset()
{
    untrackedInputContexts.Clear();
    registeredInputContexts.Clear();
    keyboardMappings.Clear();
}

void InputAPI::SetMouseCursorVisible(bool visible)
{
    if (framework->IsHeadless())
        return;
        
    if (mouseCursorVisible == visible)
        return;

    Urho3D::Input* input = GetSubsystem<Urho3D::Input>();

    mouseCursorVisible = visible;
    input->SetMouseVisible(visible);
    
    Urho3D::IntVector2 mousePos = input->GetMousePosition();
    lastMouseX = mousePos.x_;
    lastMouseY = mousePos.y_;
}

bool InputAPI::IsMouseCursorVisible() const
{ 
    return mouseCursorVisible;
}

bool InputAPI::IsKeyDown(Key keyCode) const
{
    return heldKeys.Find(keyCode) != heldKeys.End();
}

bool InputAPI::IsKeyPressed(Key keyCode) const
{
    return pressedKeys.Find(keyCode) != pressedKeys.End();
}

bool InputAPI::IsKeyReleased(Key keyCode) const
{
    return releasedKeys.Find(keyCode) != releasedKeys.End();
}

bool InputAPI::IsMouseButtonDown(int mouseButton) const
{
    assert((mouseButton & (mouseButton-1)) == 0); // Must only contain a single '1' bit.

    return (heldMouseButtons & mouseButton) != 0;
}

bool InputAPI::IsMouseButtonPressed(int mouseButton) const
{
    assert((mouseButton & (mouseButton-1)) == 0); // Must only contain a single '1' bit.

    return (pressedMouseButtons & mouseButton) != 0;
}

bool InputAPI::IsMouseButtonReleased(int mouseButton) const
{
    assert((mouseButton & (mouseButton-1)) == 0); // Must only contain a single '1' bit.

    return (releasedMouseButtons & mouseButton) != 0;
}

Point InputAPI::MousePressedPos(int mouseButton) const
{
    return mousePressPositions.Pos(mouseButton);
}

Point InputAPI::MousePos() const
{
    return Point(lastMouseX, lastMouseY);
}

int InputAPI::GetMouseMoveX() const
{
    return GetSubsystem<Urho3D::Input>()->GetMouseMoveX();
}

int InputAPI::GetMouseMoveY() const
{
    return GetSubsystem<Urho3D::Input>()->GetMouseMoveY();
}

unsigned InputAPI::GetNumTouches() const
{
    return GetSubsystem<Urho3D::Input>()->GetNumTouches();
}

Urho3D::TouchState* InputAPI::GetTouch(unsigned index) const
{
    return GetSubsystem<Urho3D::Input>()->GetTouch(index);
}

static void DumpInputContext(int &idx, const InputContextPtr &ic)
{
    if (ic)
        LogInfo("Context " + String(idx++) + ":\"" + String(ic->Name()) + "\", priority " + String(ic->Priority()));
    else
        LogInfo("Context " + String(idx++) + ": expired weak_ptr.");
}

void InputAPI::DumpInputContexts()
{
    LogInfo("TopLevelInputContext:");
    ///\todo
    //LogInfo(QString("    KeyEventReceived: %1 receivers").arg(topLevelInputContext.receivers(SIGNAL(KeyEventReceived(KeyEvent *)))));
    //LogInfo(QString("    MouseEventReceived: %1 receivers").arg(topLevelInputContext.receivers(SIGNAL(MouseEventReceived(MouseEvent *)))));
    //LogInfo(QString("    KeyPressed: %1 receivers").arg(topLevelInputContext.receivers(SIGNAL(KeyPressed(KeyEvent *)))));
    //LogInfo(QString("    KeyDown: %1 receivers").arg(topLevelInputContext.receivers(SIGNAL(KeyDown(KeyEvent *)))));
    //LogInfo(QString("    KeyReleased: %1 receivers").arg(topLevelInputContext.receivers(SIGNAL(KeyReleased(KeyEvent *)))));
    //LogInfo(QString("    MouseMove: %1 receivers").arg(topLevelInputContext.receivers(SIGNAL(MouseMove(MouseEvent *)))));
    //LogInfo(QString("    MouseScroll: %1 receivers").arg(topLevelInputContext.receivers(SIGNAL(MouseScroll(MouseEvent *)))));
    //LogInfo(QString("    MouseDoubleClicked: %1 receivers").arg(topLevelInputContext.receivers(SIGNAL(MouseDoubleClicked(MouseEvent *)))));
    //LogInfo(QString("    MouseLeftPressed: %1 receivers").arg(topLevelInputContext.receivers(SIGNAL(MouseLeftPressed(MouseEvent *)))));
    //LogInfo(QString("    MouseMiddlePressed: %1 receivers").arg(topLevelInputContext.receivers(SIGNAL(MouseMiddlePressed(MouseEvent *)))));
    //LogInfo(QString("    MouseRightPressed: %1 receivers").arg(topLevelInputContext.receivers(SIGNAL(MouseRightPressed(MouseEvent *)))));
    //LogInfo(QString("    MouseLeftReleased: %1 receivers").arg(topLevelInputContext.receivers(SIGNAL(MouseLeftReleased(MouseEvent *)))));
    //LogInfo(QString("    MouseMiddleReleased: %1 receivers").arg(topLevelInputContext.receivers(SIGNAL(MouseMiddleReleased(MouseEvent *)))));
    //LogInfo(QString("    MouseRightReleased: %1 receivers").arg(topLevelInputContext.receivers(SIGNAL(MouseRightReleased(MouseEvent *)))));
    //// NOTE not interested in gesture events, at least for now.

    int idx = 0;
    for (auto iter = registeredInputContexts.Begin() ; iter != registeredInputContexts.End() ; ++iter)
    {
        InputContextPtr ic = iter->Lock();
        DumpInputContext(idx, ic);
    }

    if (!untrackedInputContexts.Empty())
        LogInfo("Untracked input contexts: ");
    for (auto iter = untrackedInputContexts.Begin() ; iter != untrackedInputContexts.End() ; ++iter)
        DumpInputContext(idx, (*iter));
}

void InputAPI::SetPriority(InputContextPtr inputContext, int newPriority)
{
    if (!inputContext)
        return;

    // When the priority of the input context changes, it must be re-inserted in sorted order into the input context list.
    for(auto iter = registeredInputContexts.Begin();
        iter != registeredInputContexts.End(); ++iter)
        if ((*iter).Lock() == inputContext)
        {
            registeredInputContexts.Erase(iter);
            break;
        }

    // Do a sorted insert: Iterate and skip through all the input contexts that have a higher
    // priority than the desired new priority.
    auto iter = registeredInputContexts.Begin();
    for(; iter != registeredInputContexts.End(); ++iter)
    {
        SharedPtr<InputContext> inputContext = iter->Lock();
        if (!inputContext)
            continue;

        if (inputContext->Priority() <= newPriority)
            break;
    }

    // iter now points to the proper spot w.r.t the priority order. Insert there.
    registeredInputContexts.Insert(iter, WeakPtr<InputContext>(inputContext));

    inputContext->priority = newPriority;
}

InputContextPtr InputAPI::RegisterInputContext(const String &name, int priority)
{
    InputContextPtr newInputContext = InputContextPtr(new InputContext(this, name.CString(), priority));
    SetPriority(newInputContext, priority);
    return newInputContext;
}

InputContext *InputAPI::RegisterInputContextRaw(const String &name, int priority)
{
    InputContextPtr context = RegisterInputContext(name, priority);
    untrackedInputContexts.Push(context);
    return context.Get();
}

void InputAPI::UnregisterInputContextRaw(const String &name)
{
    for(auto iter = untrackedInputContexts.Begin();
        iter != untrackedInputContexts.End(); ++iter)
        if ((*iter)->Name() == name)
        {
            untrackedInputContexts.Erase(iter);
            return;
        }
    LogWarning("Failed to delete non-refcounted Input Context \"" + name + "\": an Input Context with that name doesn't exist!");
}

void InputAPI::SceneReleaseAllKeys()
{
    for(auto iter = registeredInputContexts.Begin(); iter != registeredInputContexts.End(); ++iter)
    {
        InputContextPtr inputContext = iter->Lock();
        if (inputContext)
            inputContext->ReleaseAllKeys();
    }
}

void InputAPI::SceneReleaseMouseButtons()
{
    for(int i = 1; i < MouseEvent::MaxButtonMask; i <<= 1)
        if ((heldMouseButtons & i) != 0)
        {
            // Just like with key release events, we send a very bare-bone release message here as well.
            MouseEvent mouseEvent(this);
            mouseEvent.eventType = MouseEvent::MouseReleased;
            mouseEvent.button = (MouseEvent::MouseButton)i;
            mouseEvent.x = lastMouseX;
            mouseEvent.y = lastMouseY;
            mouseEvent.z = 0;
            mouseEvent.relativeX = 0;
            mouseEvent.relativeY = 0;
            mouseEvent.relativeZ = 0;

            mouseEvent.globalX = 0;
            mouseEvent.globalY = 0;

            mouseEvent.otherButtons = 0;

            mouseEvent.timestamp = framework->Frame()->WallClockTime();

            TriggerMouseEvent(mouseEvent);
        }
}

void InputAPI::UpdateTouchPoints()
{
    Urho3D::Input* input = GetSubsystem<Urho3D::Input>();
    numTouchPoints = input->GetNumTouches();
}

void InputAPI::PruneDeadInputContexts()
{
    auto iter = registeredInputContexts.Begin();

    while(iter != registeredInputContexts.End())
    {
        if (iter->Expired())
            iter = registeredInputContexts.Erase(iter);
        else
            ++iter;
    }
}

void InputAPI::TriggerSceneKeyReleaseEvent(InputContextList::Iterator start, Key keyCode)
{
    for(; start != registeredInputContexts.End(); ++start)
    {
        InputContextPtr context = start->Lock();
        if (context)
            context->TriggerKeyReleaseEvent(keyCode);
    }

    if (heldKeys.Find(keyCode) != heldKeys.End())
    {
        KeyEvent keyEvent(this);
        keyEvent.keyCode = keyCode;
        keyEvent.eventType = KeyEvent::KeyReleased;
    }
}

void InputAPI::TriggerKeyEvent(KeyEvent &key)
{
    assert(key.eventType != KeyEvent::KeyEventInvalid);
    assert(key.handled == false);

    // First, we pass the key to the global top level input context, which operates above Qt widget input.
    topLevelInputContext.TriggerKeyEvent(key);
    if (key.handled) // Convert a Pressed event to a Released event if it was suppressed, so that lower contexts properly let go of the key.
        key.eventType = KeyEvent::KeyReleased;

    // If the mouse cursor is hidden, we treat each InputContext as if it had TakesKeyboardEventsOverQt true.
    // This is because when the mouse cursor is hidden, no key input should go to the main 2D UI window.

    // Pass the event to all input contexts in the priority order.
    for(auto iter = registeredInputContexts.Begin(); iter != registeredInputContexts.End(); ++iter)
    {
        InputContextPtr context = iter->Lock();
        if (context.Get())// && (!qtWidgetHasKeyboardFocus || context->TakesKeyboardEventsOverQt() || !IsMouseCursorVisible()))
            context->TriggerKeyEvent(key);
        if (key.handled)
            key.eventType = KeyEvent::KeyReleased;
    }

    // If the mouse cursor is hidden, all key events should go to the 'scene' - In that case, suppress all key events from going to the main 2D Qt window.
    if (!IsMouseCursorVisible())
        key.Suppress();
}

void InputAPI::TriggerMouseEvent(MouseEvent &mouse)
{
    assert(mouse.handled == false);

    // Remember where this press occurred, for tracking drag situations.
    if (mouse.eventType == MouseEvent::MousePressed)
        mousePressPositions.Set(mouse.button, mouse.x, mouse.y, mouse.origin);

    // Copy over the set of tracked mouse press positions into the event structure, so that 
    // the client can do proper drag tracking.
    mouse.mousePressPositions = mousePressPositions;

    // First, we pass the event to the global top level input context, which operates above Qt widget input.
    topLevelInputContext.TriggerMouseEvent(mouse);
    if (mouse.handled)
        return;

    // If the mouse cursor is hidden, we treat each InputContext as if it had TakesMouseEventsOverQt true.
    // This is because when the mouse cursor is hidden, no mouse input should go to the main 2D UI window.

    // Pass the event to all input contexts in the priority order.
    for(auto iter = registeredInputContexts.Begin(); iter != registeredInputContexts.End(); ++iter)
    {
        if (mouse.handled)
            break;
        InputContextPtr context = iter->Lock();
        if (context.Get())
            context->TriggerMouseEvent(mouse);
    }

    // If the mouse cursor is hidden, all mouse events should go to the 'scene' - In that case, suppress all mouse events from going to the main 2D Qt window.
    if (!IsMouseCursorVisible())
        mouse.Suppress();
}

void InputAPI::SetKeyBinding(const String &actionName, const KeySequence &key)
{
    keyboardMappings[actionName] = key;
}

KeySequence InputAPI::KeyBinding(const String &actionName) const
{
    auto iter = keyboardMappings.Find(actionName);
    return iter != keyboardMappings.End() ? iter->second_ : KeySequence();
}

KeySequence InputAPI::KeyBinding(const String &actionName, const KeySequence &defaultKey)
{
    auto iter = keyboardMappings.Find(actionName);
    if (iter == keyboardMappings.End())
    {
        SetKeyBinding(actionName, defaultKey);
        return defaultKey;
    }
    return iter->second_;
}

void InputAPI::RemoveKeyBinding(const String &actionName)
{
    int ret = keyboardMappings.Erase(actionName);
    if (ret == 0)
        LogWarning("InputAPI::RemoveKeyBinding: Could not find " + actionName + ".");
}

void InputAPI::LoadKeyBindingsFromFile()
{
    ConfigAPI &cfg = *framework->Config();
    ConfigData inputConfig(ConfigAPI::FILE_FRAMEWORK, "input");
    for(int i = 0; ; ++i)
    {
        StringList bindings = cfg.Read(inputConfig, String("keybinding") + String(i)).GetString().Split('|');
        if (bindings.Size() != 2)
            break;
        SetKeyBinding(bindings[0], ToInt(bindings[1]));
    }
}

void InputAPI::SaveKeyBindingsToFile()
{
    ConfigAPI &cfg = *framework->Config();
    ConfigData inputConfig(ConfigAPI::FILE_FRAMEWORK, "input");
    int i = 0;
    for(auto iter = keyboardMappings.Begin(); iter != keyboardMappings.End(); ++iter)
        cfg.Write(inputConfig, String("keybinding") + String(i++), iter->first_ + '|' + String(iter->second_));
}

//bool InputAPI::eventFilter(Object *obj, QEvent *event)
void InputAPI::EventFilter(StringHash eventType, VariantMap& eventData)
{
    Urho3D::Input* input = GetSubsystem<Urho3D::Input>();

    if (eventType == Urho3D::E_KEYDOWN)
    {
        KeyEvent keyEvent(this);
        keyEvent.urhoEvent = eventData;
        keyEvent.keyCode = eventData[Urho3D::KeyDown::P_KEY].GetInt();// StripModifiersFromKey(e->key());
        keyEvent.sequence = KeySequence(eventData[Urho3D::KeyDown::P_KEY].GetInt() | eventData[Urho3D::KeyDown::P_QUALIFIERS].GetInt());
        keyEvent.keyPressCount = 1;
        keyEvent.modifiers = eventData[Urho3D::KeyDown::P_QUALIFIERS].GetInt();
        //keyEvent.text = e->text(); ///\todo
        keyEvent.eventType = KeyEvent::KeyPressed;
        keyEvent.timestamp = framework->Frame()->WallClockTime();
        // Assign the keys from the heldKeys map to the keyEvent.otherHeldKeys vector
        for (auto current = heldKeys.Begin(); current != heldKeys.End(); ++ current)
            keyEvent.otherHeldKeys.Push((*current).first_);

        keyEvent.handled = false;

        currentModifiers = eventData[Urho3D::KeyDown::P_QUALIFIERS].GetInt(); // local tracking for mouse events

        auto keyRecord = heldKeys.Find(keyEvent.keyCode);
        if (keyRecord == heldKeys.End())
        {
            KeyPressInformation info;
            info.keyPressCount = 1;
            info.keyState = KeyEvent::KeyPressed;
            //info.firstPressTime = now; ///\todo
            heldKeys[keyEvent.keyCode] = info;
        }

        // Queue up the press event for the polling API
        if (keyEvent.keyPressCount == 1) /// \todo The polling API does not get key repeats at all. Should it?
            newKeysPressedQueue.Push(eventData[Urho3D::KeyDown::P_KEY].GetInt());

        TriggerKeyEvent(keyEvent);

        return; // If we got true here, need to suppress this event from going to Qt.
    }

    else if (eventType == Urho3D::E_KEYUP)
    {
        HeldKeysMap::Iterator existingKey = heldKeys.Find(eventData[Urho3D::KeyDown::P_KEY].GetInt());

        // If we received a release on an unknown key we haven't received a press for, don't pass it to the scene,
        // since we didn't pass the press to the scene either (or we've already passed the release before, so don't 
        // pass duplicate releases).
        if (existingKey == heldKeys.End())
            return;

        KeyEvent keyEvent(this);
        keyEvent.urhoEvent = eventData;
        keyEvent.keyCode = eventData[Urho3D::KeyDown::P_KEY].GetInt();
        keyEvent.keyPressCount = existingKey->second_.keyPressCount;
        keyEvent.modifiers = eventData[Urho3D::KeyDown::P_QUALIFIERS].GetInt();
        //keyEvent.text = e->text(); ///\todo
        keyEvent.eventType = KeyEvent::KeyReleased;
        //keyEvent.otherHeldKeys = heldKeys; ///\todo
        keyEvent.handled = false;
        keyEvent.timestamp = framework->Frame()->WallClockTime();

        heldKeys.Erase(existingKey);
        currentModifiers = eventData[Urho3D::KeyDown::P_QUALIFIERS].GetInt(); // local tracking for mouse events

        // Queue up the release event for the polling API, independent of whether any Qt widget has keyboard focus.
        if (keyEvent.keyPressCount == 1) /// \todo The polling API does not get key repeats at all. Should it?
            newKeysReleasedQueue.Push(eventData[Urho3D::KeyDown::P_KEY].GetInt());

        TriggerKeyEvent(keyEvent);
        
        return; // Suppress this event from going forward.
    }

    else if (eventType == Urho3D::E_MOUSEBUTTONDOWN || eventType == Urho3D::E_MOUSEBUTTONUP)
    {
        // We always update the global polled input states, independent of whether any the mouse cursor is
        // on top of any Qt widget.
        if (eventType == Urho3D::E_MOUSEBUTTONDOWN)
        {
            heldMouseButtons |= eventData[Urho3D::MouseButtonDown::P_BUTTON].GetInt();  //(MouseEvent::MouseButton)e->button();
            newMouseButtonsPressedQueue |= eventData[Urho3D::MouseButtonDown::P_BUTTON].GetInt();
        }
        else
        {
            heldMouseButtons &= ~eventData[Urho3D::MouseButtonDown::P_BUTTON].GetInt();
            newMouseButtonsReleasedQueue |= eventData[Urho3D::MouseButtonDown::P_BUTTON].GetInt();
        }

        // The mouse coordinates we receive can come from different widgets, and we are interested only in the coordinates
        // in the QGraphicsView client area, so we need to remap them.
        Point mousePos(input->GetMousePosition().x_, input->GetMousePosition().y_);//MapPointToMainGraphicsView(obj, e->pos());

        MouseEvent mouseEvent(this);
        mouseEvent.eventType = (eventType == Urho3D::E_MOUSEBUTTONDOWN) ? MouseEvent::MousePressed : MouseEvent::MouseReleased;

        mouseEvent.button = (MouseEvent::MouseButton)eventData[Urho3D::MouseButtonDown::P_BUTTON].GetInt();
        mouseEvent.x = mousePos.x;
        mouseEvent.y = mousePos.y;
        mouseEvent.z = 0;
        // Mouse presses do not carry relative mouse movement information at the same time.
        // (separate relative movement messages are passed for first-person mode moves)
        mouseEvent.relativeX = 0;
        mouseEvent.relativeY = 0;
        
        mouseEvent.relativeZ = 0;
        mouseEvent.modifiers = eventData[Urho3D::MouseButtonDown::P_QUALIFIERS].GetInt();//currentModifiers;

        lastMouseX = mouseEvent.x;
        lastMouseY = mouseEvent.y;

        mouseEvent.globalX = mouseEvent.x;
        mouseEvent.globalY = mouseEvent.y;

        mouseEvent.otherButtons = eventData[Urho3D::MouseButtonDown::P_BUTTONS].GetUInt();
        mouseEvent.modifiers = eventData[Urho3D::MouseButtonDown::P_QUALIFIERS].GetInt();
        for (auto current = heldKeys.Begin(); current != heldKeys.End(); ++ current)
            mouseEvent.heldKeys.Push((*current).first_);
        mouseEvent.handled = false;
        mouseEvent.timestamp = framework->Frame()->WallClockTime();

        TriggerMouseEvent(mouseEvent);
        return;
    }

    //case Urho3D::E_MOUSEMOVE:
    else if (eventType == Urho3D::E_MOUSEMOVE)
    {
        MouseEvent mouseEvent(this);
        mouseEvent.eventType = MouseEvent::MouseMove;
        mouseEvent.button = (MouseEvent::MouseButton)eventData[Urho3D::MouseMove::P_BUTTONS].GetInt();

        // The mouse coordinates we receive can come from different widgets, and we are interested only in the coordinates
        // in the QGraphicsView client area, so we need to remap them.
        Point mousePos(input->GetMousePosition().x_, input->GetMousePosition().y_);

        mouseEvent.z = 0;
        mouseEvent.relativeX = eventData[Urho3D::MouseMove::P_DX].GetInt();
        mouseEvent.relativeY = eventData[Urho3D::MouseMove::P_DY].GetInt();
        mouseEvent.relativeZ = 0;

        mouseEvent.x = mousePos.x;
        mouseEvent.y = mousePos.y;

        // If there wasn't any change to the mouse relative coords in FPS mode, ignore this event.
        if (mouseEvent.relativeX == 0 && mouseEvent.relativeY == 0)
            return;
        
        mouseEvent.globalX = mousePos.x;
        mouseEvent.globalY = mousePos.y;
        mouseEvent.otherButtons = eventData[Urho3D::MouseMove::P_BUTTONS].GetInt();
        for (auto current = heldKeys.Begin(); current != heldKeys.End(); ++ current)
            mouseEvent.heldKeys.Push((*current).first_);
        mouseEvent.handled = false;
        mouseEvent.timestamp = framework->Frame()->WallClockTime();

        // Save the absolute coordinates to be able to compute the proper relative movement values in the next
        // mouse event.
        lastMouseX = mouseEvent.x;
        lastMouseY = mouseEvent.y;

        TriggerMouseEvent(mouseEvent);

        return;
    }

    else if (eventType == Urho3D::E_MOUSEWHEEL)
    {
        MouseEvent mouseEvent(this);
        mouseEvent.eventType = MouseEvent::MouseScroll;
        
        mouseEvent.button = MouseEvent::NoButton;
        mouseEvent.otherButtons = eventData[Urho3D::MouseWheel::P_BUTTONS].GetInt();
        mouseEvent.x = input->GetMousePosition().x_;
        mouseEvent.y = input->GetMousePosition().y_;
        mouseEvent.z = 0; // Mouse wheel does not have an absolute z position, only relative.
        mouseEvent.relativeX = 0;
        mouseEvent.relativeY = 0;
        mouseEvent.relativeZ = eventData[Urho3D::MouseWheel::P_WHEEL].GetInt();
        mouseEvent.globalX = input->GetMousePosition().x_;
        mouseEvent.globalY = input->GetMousePosition().y_;

        mouseEvent.modifiers = eventData[Urho3D::MouseWheel::P_QUALIFIERS].GetInt();
        for (auto current = heldKeys.Begin(); current != heldKeys.End(); ++ current)
            mouseEvent.heldKeys.Push((*current).first_);
        mouseEvent.handled = false;
        mouseEvent.timestamp = framework->Frame()->WallClockTime();

        TriggerMouseEvent(mouseEvent);
        return;
    }
    else if (eventType == Urho3D::E_TOUCHBEGIN)
    {
        UpdateTouchPoints();
        TouchBegin.Emit(eventData);
        return;
    }
    else if (eventType == Urho3D::E_TOUCHMOVE)
    {
        UpdateTouchPoints();
        TouchUpdate.Emit(eventData);
        return;
    }

    else if (eventType == Urho3D::E_TOUCHEND)
    {
        UpdateTouchPoints();
        TouchEnd.Emit(eventData);
        return;
        
    } // ~switch
}

void InputAPI::Update(float /*frametime*/)
{
    PROFILE(InputAPI_Update);

    // Move all the double-buffered input events to current events.
    pressedKeys = newKeysPressedQueue;
    newKeysPressedQueue.Clear();

    releasedKeys = newKeysReleasedQueue;
    newKeysReleasedQueue.Clear();

    pressedMouseButtons = newMouseButtonsPressedQueue;
    newMouseButtonsPressedQueue = 0;

    releasedMouseButtons = newMouseButtonsReleasedQueue;
    newMouseButtonsReleasedQueue = 0;

    // Update the new frame start to all input contexts.
    topLevelInputContext.UpdateFrame();

    for(InputContextList::Iterator iter = registeredInputContexts.Begin();
        iter != registeredInputContexts.End(); ++iter)
    {
        InputContextPtr inputContext = (*iter).Lock();
        if (inputContext)
            inputContext->UpdateFrame();
    }

    PruneDeadInputContexts();
}
}

