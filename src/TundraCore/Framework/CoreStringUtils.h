// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"
#include "CoreTypes.h"

namespace kNet { class DataSerializer; class DataDeserializer; }

namespace Tundra
{

/// Reads an UTF-8 encoded String from a data stream
String TUNDRACORE_API ReadUtf8String(kNet::DataDeserializer &dd);

/// Writes String to a data stream using UTF-8 encoding.
void TUNDRACORE_API WriteUtf8String(kNet::DataSerializer &ds, const String &str);

}
