// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "TundraLogicUtils.h"

namespace Tundra
{

std::vector<s8> StringToBuffer(const String& str)
{
    std::vector<s8> ret;
    ret.resize(str.Length());
    if (str.Length())
        memcpy(&ret[0], &str[0], str.Length());
    return ret;
}

String BufferToString(const std::vector<s8>& buffer)
{
    if (!buffer.empty())
        return String((const char*)&buffer[0], buffer.size());
    else
        return String();
}

}
