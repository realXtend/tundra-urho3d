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

namespace Method
{
    int CurlOption(int method)
    {
        switch(method)
        {
            case Method::Patch:
            case Method::Options:
            case Method::Delete:
                static_cast<int>(CURLOPT_CUSTOMREQUEST);
            case Method::Head:
                return static_cast<int>(CURLOPT_NOBODY);
            case Method::Get:
                return static_cast<int>(CURLOPT_HTTPGET);
            case Method::Put:
                return static_cast<int>(CURLOPT_PUT);
            case Method::Post:
                return static_cast<int>(CURLOPT_POST);
            default:
                break;
        }
        LogError(Urho3D::ToString("Http: Unknown HTTP::Method '%s'", static_cast<int>(method)));
        return 0;
    }

    Variant CurlOptionValue(int method)
    {
        switch(method)
        {
            case Method::Patch:
                return Variant(String("PATCH"));
            case Method::Options:
                return Variant(String("OPTIONS"));
            case Method::Delete:
                return Variant(String("DELETE"));
            case Method::Head:
            case Method::Get:
            case Method::Put:
            case Method::Post:
                return Variant(1);
            default:
                break;
        }
        LogError(Urho3D::ToString("Http: Unknown HTTP::Method '%s'", static_cast<int>(method)));
        return Variant();
    }

    String ToString(int method)
    {
        switch(method)
        {
            case Method::Patch:
                return "PATCH";
            case Method::Options:
                return "OPTIONS";
            case Method::Delete:
                return "DELETE";
            case Method::Head:
                return "HEAD";
            case Method::Get:
                return "GET";
            case Method::Put:
                return "PUT";
            case Method::Post:
                return "POST";
            default:
                break;
        }
        return "";
    }
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
    status(-1),
    bytesPerSec(0.0)
{
}

}
}
