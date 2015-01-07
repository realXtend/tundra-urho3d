// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "HttpPluginApi.h"
#include "HttpPluginFwd.h"
#include "CoreTypes.h"

#include <Urho3D/Container/Str.h>
#include <Urho3D/Container/HashMap.h>
#include <Urho3D/Core/Variant.h>

#include <curl/curl.h>

namespace Tundra
{
    namespace Options
    {
        const String Url    = "Url";
        const String Method = "Method";
    }

    namespace Curl
    {
        struct Option
        {
            CURLoption option;
            Variant value;

            Option() {}
            Option(CURLoption option, const Variant &value) : 
                option(option),
                value(value)
            {
            }
        };
        typedef HashMap<String, Option> OptionMap;
    }
}
