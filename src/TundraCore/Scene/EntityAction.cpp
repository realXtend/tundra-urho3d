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

void EntityAction::Trigger(const StringVector &parameters)
{
    Triggered.Emit(parameters);
}

EntityAction::EntityAction(const String &name_)
:name(name_)
{
}

}
