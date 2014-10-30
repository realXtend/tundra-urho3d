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
    /// Common request and response headers.
    namespace Header
    {
        // Common headers for both request and response
        const String CacheControl               = "Cache-Control";
        const String ContentLength              = "Content-Length";
        const String ContentMD5                 = "Content-MD5";
        const String ContentType                = "Content-Type";
        const String Date                       = "Date";
        const String Pragma                     = "Pragma";
        const String Upgrade                    = "Upgrade";
        const String Via                        = "Via";
        const String Warning                    = "Warning";

        // Common request headers
        const String Accept                     = "Accept";
        const String AcceptCharset              = "Accept-Charset";
        const String AcceptEncoding             = "Accept-Encoding";
        const String AcceptLanguage             = "Accept-Language";
        const String AcceptDatetime             = "Accept-Datetime";
        const String Authorization              = "Authorization";
        const String Connection                 = "Connection";
        const String Cookie                     = "Cookie";
        const String Expect                     = "Expect";
        const String From                       = "From";
        const String Host                       = "Host";
        const String IfMatch                    = "If-Match";
        const String IfModifiedSince            = "If-Modified-Since";
        const String IfNoneMatch                = "If-None-Match";
        const String IfRange                    = "If-Range";
        const String IfUnmodifiedSince          = "If-Unmodified-Since";
        const String MaxForwards                = "Max-Forwards";
        const String Origin                     = "Origin";
        const String ProxyAuthorization         = "Proxy-Authorization";
        const String Range                      = "Range";
        const String Referer                    = "Referer";
        const String TE                         = "TE";
        const String UserAgent                  = "User-Agent";

        // Common response headers
        const String AccessControlAllowOrigin   = "Access-Control-Allow-Origin";
        const String AcceptRanges               = "Accept-Ranges";
        const String Age                        = "Age";
        const String Allow                      = "Allow";
        const String ContentEncoding            = "Content-Encoding";
        const String ContentLanguage            = "Content-Language";
        const String ContentLocation            = "Content-Location";        
        const String ContentDisposition         = "Content-Disposition";
        const String ContentRange               = "Content-Range";
        const String ETag                       = "ETag";
        const String Expires                    = "Expires";
        const String LastModified               = "Last-Modified";
        const String Link                       = "Link";
        const String Location                   = "Location";
        const String P3P                        = "P3P";
        const String ProxyAuthenticate          = "Proxy-Authenticate";
        const String Refresh                    = "Refresh";
        const String RetryAfter                 = "Retry-After";
        const String Server                     = "Server";
        const String SetCookie                  = "Set-Cookie";
        const String Status                     = "Status";
        const String StrictTransportSecurity    = "Strict-Transport-Security";
        const String Trailer                    = "Trailer";
        const String TransferEncoding           = "Transfer-Encoding";
        const String Vary                       = "Vary";
        const String WWWAuthenticate            = "WWW-Authenticate";
        const String XFrameOptions              = "X-Frame-Options";
    }

    /// Common request Content-Type headers.
    namespace ContentType
    {
        const String Binary                     = "application/octet-stream";
        const String JSON                       = "application/json";
        const String XML                        = "application/xml";
        const String Text                       = "text/plain";
        const String HTML                       = "text/html";
        const String CSS                        = "text/css";
        const String ImageJPEG                  = "image/jpeg";
        const String ImagePNG                   = "image/png";
        const String ImageBMP                   = "mage/bmp";
        const String ImageGIF                   = "image/gif";
    }

    /// @cond PRIVATE
    /// Implementation detail. Not visible in the API.
    namespace Method 
    {
        const int Head      = 1;
        const int Options   = 2;
        const int Get       = 3;
        const int Put       = 4;
        const int Patch     = 5;
        const int Post      = 6;
        const int Delete    = 7;

        int CurlOption(int method);
        Variant CurlOptionValue(int method);
        String ToString(int method);
    };
    /// @endcond

    struct RequestData
    {
        Curl::RequestHandle *curlHandle;
        Curl::OptionMap options;

        HttpHeaderMap headers;
        curl_slist *curlHeaders;

        Vector<u8> bodyBytes;
        Vector<u8> headersBytes;

        String error;

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

        double bytesPerSec;

        // Default ctor
        ResponseData();
    };
}
}

/// @endcond
