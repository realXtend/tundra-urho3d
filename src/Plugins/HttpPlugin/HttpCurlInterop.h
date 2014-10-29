// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "HttpPluginApi.h"
#include "HttpPluginFwd.h"
#include "CoreTypes.h"

#include <Engine/Container/Str.h>
#include <Engine/Container/HashMap.h>
#include <Engine/Core/Variant.h>

#include <curl/curl.h>

namespace Tundra
{
    namespace Options
    {
        const String Url = "Url";
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
