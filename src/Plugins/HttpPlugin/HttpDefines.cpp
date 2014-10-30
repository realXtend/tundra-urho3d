// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "HttpDefines.h"
#include "HttpCurlInterop.h"
#include "LoggingFunctions.h"

#include <Engine/Core/StringUtils.h>

namespace Tundra
{
namespace Http
{

// Method

int MethodCurlOption(Method method)
{
    switch(method)
    {
        case MethodPatch:
        case MethodOptions:
        case MethodDelete:
            static_cast<int>(CURLOPT_CUSTOMREQUEST);
        case MethodHead:
            return static_cast<int>(CURLOPT_NOBODY);
        case MethodGet:
            return static_cast<int>(CURLOPT_HTTPGET);
        case MethodPut:
            return static_cast<int>(CURLOPT_PUT);
        case MethodPost:
            return static_cast<int>(CURLOPT_POST);
        default:
            break;
    }
    LogError(Urho3D::ToString("Http: Unknown HTTP::Method '%s'", static_cast<int>(method)));
    return 0;
}

Variant MethodCurlOptionValue(Method method)
{
    switch(method)
    {
        case MethodPatch:
            return Variant(String("PATCH"));
        case MethodOptions:
            return Variant(String("OPTIONS"));
        case MethodDelete:
            return Variant(String("DELETE"));
        case MethodHead:
        case MethodGet:
        case MethodPut:
        case MethodPost:
            return Variant(1L);
        default:
            break;
    }
    LogError(Urho3D::ToString("Http: Unknown HTTP::Method '%s'", static_cast<int>(method)));
    return Variant();
}

// RequestData

RequestData::RequestData() :
    curlHandle(0),
    curlHeaders(0),
    msecSpent(0)
{
}

curl_slist *RequestData::CreateCurlHeaders(bool print)
{
    if (curlHeaders)
    {
        curl_slist_free_all(curlHeaders);
        curlHeaders = 0;
    }
    if (headers.empty())
        return curlHeaders;
    if (print)
        PrintRaw("Request Headers\n");
    for (HttpHeaderMap::const_iterator iter = headers.begin(); iter != headers.end(); ++iter)
    {
        String value = Urho3D::ToString("%s: %s", iter->first.CString(), iter->second.CString());
        curlHeaders = curl_slist_append(curlHeaders, value.CString());
        if (print)
            PrintRaw(value + "\n");
    }
    if (print)
        PrintRaw("\n");
    return curlHeaders;
}

String RequestData::OptionValueString(const String &name)
{
    Curl::OptionMap::ConstIterator option = options.Find(name);
    if (option != options.End())
    {
        const Variant &value = option->second_.value;
        if (value.GetType() == Urho3D::VAR_STRING)
            return value.GetString();
    }
    return "";
}

// ResponseData

ResponseData::ResponseData() :
    httpVersionMajor(-1),
    httpVersionMinor(-1),
    status(-1)
{
}

}
}
