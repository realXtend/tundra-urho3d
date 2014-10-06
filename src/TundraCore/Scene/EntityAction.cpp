/**
 *  For conditions of distribution and use, see copyright notice in LICENSE
 *
 *  @file   EntityAction.cpp
 *  @brief  Represent an executable command on an Entity.
 */

#include "StableHeaders.h"
#include "EntityAction.h"

namespace Tundra
{

void EntityAction::Trigger(const String &param1, const String &param2, const String &param3, const StringVector &params)
{
    Triggered.Emit(param1, param2, param3, params);
}

EntityAction::EntityAction(const String &name_)
:name(name_)
{
}

}
