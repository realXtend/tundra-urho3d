/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   ChangeRequest.h
    @brief  A result object to get return values from Qt signal handlers in the permission system (AboutToModifyEntity etc) */

#pragma once

#include "TundraCoreApi.h"

namespace Tundra
{

/// A result object to get return values from signal handlers in the permission system (AboutToModifyEntity etc)
class TUNDRACORE_API ChangeRequest
{
public:
    ChangeRequest();

    bool allowed;

    bool IsAllowed();
    void SetAllowed(bool allow);
    void Deny();
};

}