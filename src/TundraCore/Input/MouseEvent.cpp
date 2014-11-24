// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "MouseEvent.h"
#include "InputAPI.h"


namespace Tundra
{

MouseEvent::MouseEvent(InputAPI *owner) :
    Object(owner->GetContext()),
    eventType(MouseEventInvalid),
    button(MouseEvent::NoButton),
    origin(PressOriginNone),
    x(-1), y(-1), z(-1),
    relativeX(-1), relativeY(-1), relativeZ(-1),
    globalX(-1), globalY(-1),
    otherButtons(0),
    modifiers(0),
    handled(false),
    timestamp(0.f)
{
}

static int UrhoMouseButtonEnumToIndex(int mouseButton)
{
    switch(mouseButton)
    {
    case Urho3D::MOUSEB_LEFT:  return 0;
    case Urho3D::MOUSEB_RIGHT: return 1;
    case Urho3D::MOUSEB_MIDDLE:   return 2;
    case Urho3D::MOUSEB_X1:    return 3;
    case Urho3D::MOUSEB_X2:    return 4;
    default: assert(false); return 0;
    }
}

MouseEvent::PressPositions::PressPositions()
{
    for(int i = 0; i < 5; ++i)
    {
        x[i] = y[i] = -1;
        origin[i] = PressOriginNone;
    }
}

Point MouseEvent::PressPositions::Pos(int mouseButton) const
{
    int i = UrhoMouseButtonEnumToIndex(mouseButton);
    return Point(x[i], y[i]);
}

void MouseEvent::PressPositions::Set(int mouseButton, int x_, int y_, PressOrigin origin_)
{

    int i = UrhoMouseButtonEnumToIndex(mouseButton);
    x[i] = x_;
    y[i] = y_;
    origin[i] = origin_;
}

MouseEvent::PressOrigin MouseEvent::PressPositions::Origin(int mouseButton) const
{
    int i = UrhoMouseButtonEnumToIndex(mouseButton);
    return origin[i];
}

Point MouseEvent::MousePressedPos(int mouseButton) const
{
    return mousePressPositions.Pos(mouseButton);
}

bool MouseEvent::IsButtonDown(MouseButton button_) const
{
    return button == button_ || ((otherButtons & button_) != 0);
}

}
