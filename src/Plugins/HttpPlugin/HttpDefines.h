// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "HttpPluginFwd.h"

#include <Engine/Core/Timer.h>

/// @cond PRIVATE

struct curl_slist;

namespace Tundra
{
    struct HttpRequestData
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
        HttpRequestData();

        // Create curl object from current headers
        curl_slist *CreateCurlHeaders(bool print);

        // Return option value as string
        String OptionValueString(const String &name);
    };

    struct HttpResponseData
    {
        int HttpVersionMajor;
        int HttpVersionMinor;

        int Status;
        String StatusText;

        HttpHeaderMap Headers;
        String LastHeaderField;

        Vector<u8> BodyBytes;
        Vector<u8> HeadersBytes;

        HttpResponseData();
    };
}

/// @endcond
