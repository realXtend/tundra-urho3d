// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "HttpDefines.h"
#include "HttpCurlInterop.h"
#include "LoggingFunctions.h"

#include <Engine/Core/StringUtils.h>

namespace Tundra
{

HttpRequestData::HttpRequestData() :
    curlHandle(0),
    curlHeaders(0),
    msecSpent(0)
{
}

curl_slist *HttpRequestData::CreateCurlHeaders(bool print)
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

String HttpRequestData::OptionValueString(const String &name)
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

HttpResponseData::HttpResponseData() :
    HttpVersionMajor(-1),
    HttpVersionMinor(-1),
    Status(-1)
{
}

}