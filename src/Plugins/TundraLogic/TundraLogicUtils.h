// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "CoreTypes.h"
#include <vector>

namespace Tundra
{

std::vector<s8> StringToBuffer(const String& str);

String BufferToString(const std::vector<s8>& buffer);

}