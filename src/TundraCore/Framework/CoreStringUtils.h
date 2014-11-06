// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"
#include "CoreTypes.h"

#include <Engine/Container/Str.h>

namespace kNet { class DataSerializer; class DataDeserializer; }

namespace Tundra
{

/// Can be used as a custom comparator in std::map or similar when wanting case-insensitive comparison.
struct TUNDRACORE_API StringCompareCaseInsensitive
{
    bool operator()(const String &a, const String &b) const
    {
        return a.Compare(b, false) < 0;
    }
};

/// Reads an UTF-8 encoded String from a data stream
String TUNDRACORE_API ReadUtf8String(kNet::DataDeserializer &dd);

/// Writes String to a data stream using UTF-8 encoding.
/** The maximum allowed length for the string is 65535 characters. */
void TUNDRACORE_API WriteUtf8String(kNet::DataSerializer &ds, const String &str);

/// Parses a input string to a command and optional parameters.
/** Works on both forms of: "MyFunction(one, to, three)" and "MyFunction one to three" */
void TUNDRACORE_API ParseCommand(String input, String &command, StringVector &parameters);

/// Pad a string with whitespace.
/** Negative @pad will insert the whitespace to the left, positive to the right. */
String TUNDRACORE_API PadString(String str, int pad);

/// Templated string padding overload that accepts anything the Urho3d::String ctor accepts.
template <typename T>
String PadString(T val, int pad)
{
    return PadString(String(val), pad);
}

String TUNDRACORE_API Join(const StringVector &list, const String &sseparator);

}
