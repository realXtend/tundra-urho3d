// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "CoreStringUtils.h"

#include <kNet/DataSerializer.h>
#include <kNet/DataDeserializer.h>

namespace Tundra
{

String ReadUtf8String(kNet::DataDeserializer &dd)
{
    String ret;
    ret.Resize(dd.Read<u16>());
    if (ret.Length())
        dd.ReadArray<u8>((u8*)&ret[0], ret.Length());
    return ret;
}

void WriteUtf8String(kNet::DataSerializer& ds, const String& str)
{
    ds.Add<u16>(static_cast<u16>(str.Length()));
    if (str.Length())
        ds.AddArray<u8>((const u8*)&str[0], str.Length());
}

}
