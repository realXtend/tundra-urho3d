// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "HttpPluginFwd.h"

#include <Engine/Core/Timer.h>

/// @cond PRIVATE

struct curl_slist;

namespace Tundra
{
namespace Http
{

enum Method 
{
    MethodUnknown = 0,
    MethodHead,
    MethodOptions,
    MethodGet,
    MethodPut,
    MethodPatch,
    MethodPost,
    MethodDelete
};

int MethodCurlOption(Method method);
Variant MethodCurlOptionValue(Method method);

struct RequestData
{
    Curl::RequestHandle *curlHandle;
    Curl::OptionMap options;

    HttpHeaderMap headers;
    curl_slist *curlHeaders;

    Vector<u8> bodyBytes;
    Vector<u8> headersBytes;

    // Time spent executing request and processing data.
    uint msecSpent;

    // Defalt ctor
    RequestData();

    // Create curl object from current headers
    curl_slist *CreateCurlHeaders(bool print);

    // Return option value as string
    String OptionValueString(const String &name);
};

struct ResponseData
{
    int httpVersionMajor;
    int httpVersionMinor;

    int status;
    String statusText;

    HttpHeaderMap headers;
    String lastHeaderField;

    Vector<u8> bodyBytes;
    Vector<u8> headersBytes;

    // Default ctor
    ResponseData();
};

}
}

/// @endcond
