// For conditions of distribution and use, see copyright notice in LICENSE

//#include "StableHeaders.h"
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
                return static_cast<int>(CURLOPT_CUSTOMREQUEST);
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

int HttpWeekDay(String &day)
{
    // 0   1   2   3   4   5   6
    // Sun Mon Tue Wed Thu Fri Sat
    const char d0 = day[0];
    const char d1 = day[1];
    if (d0 == 'S')
        return (d1 == 'u' ? 0 : 6); // Sun|Sat
    else if (d0 == 'M')
        return 1; // Mon
    else if (d0 == 'T')
        return (d1 == 'u' ? 2 : 4); // Tue|Thu
    else if (d0 == 'W')
        return 3; // Wed
    else if (d0 == 'F')
        return 5; // Fri
    return -1;
}

int HttpMonthDay(String &day)
{
    if (day[0] == '0')
        return Urho3D::ToInt(&day[1]);
    return Urho3D::ToInt(day);
}

int HttpMonth(String &month)
{
    // 0   1   2   3   4   5    6    7   8    9   10  11
    // Jan Feb Mar Apr May June July Aug Sept Oct Nov Dec
    const char m0 = month[0];
    const char m1 = month[1];
    const char m2 = month[1];
    if (m0 == 'J')
        return (m1 == 'a' ? 0 : (m2 == 'n' ? 5 : 6)); // Jan|June|July
    else if (m0 == 'F')
        return 1; // Feb
    else if (m0 == 'M')
        return (m2 == 'r' ? 2 : 4); // Mar/May
    else if (m0 == 'A')
        return (m1 == 'p' ? 3 : 7); // Apr/Aug
    else if (m0 == 'S')
        return 8; // Sept
    else if (m0 == 'O')
        return 9; // Oct
    else if (m0 == 'N')
        return 10; // Nov
    else if (m0 == 'D')
        return 11; // Dec
    return -1;
}

int HttpYear(String &year)
{
    if (year.Length() != 4)
        return -1;
    return Urho3D::ToInt(year) - 1900;
}

String LocalEpochToHttpDate(time_t epoch)
{
    char buf[80];
    struct tm timeFormat = *gmtime(&epoch);
    strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", &timeFormat);
    return String(buf);
}

time_t HttpDateToUtcEpoch(const String &date)
{
    StringVector parts = date.Trimmed().Split(' ');
    if (parts.Size() != 6)
    {
        LogErrorF("Invalid date '%s' format. Expecting 'Tue, 15 Nov 2010 08:12:31 GMT'.", date.CString());
        return 0;
    }

    /* Tue, 15 Nov 2010 08:12:31 GMT
       http://www.cplusplus.com/reference/ctime/tm/
       http://www.cplusplus.com/reference/ctime/mktime/ */
    struct tm timeFormat;
    timeFormat.tm_yday = 0;
    timeFormat.tm_wday = 0;
    timeFormat.tm_isdst = 0;
    timeFormat.tm_wday = HttpWeekDay(parts[0]); // Handles trailing ','
    timeFormat.tm_mday = HttpMonthDay(parts[1]);
    timeFormat.tm_mon = HttpMonth(parts[2]);
    timeFormat.tm_year = HttpYear(parts[3]);

    StringVector timeParts = parts[4].Trimmed().Split(':');
    if (timeParts.Size() != 3)
    {
        LogErrorF("Invalid time '%s' format. Expecting '08:12:31'.", parts[4].CString());
        return 0;
    }
    timeFormat.tm_hour = Urho3D::ToInt(timeParts[0]);
    timeFormat.tm_min = Urho3D::ToInt(timeParts[1]);
    timeFormat.tm_sec = Urho3D::ToInt(timeParts[2]);

    if (timeFormat.tm_wday == -1 || timeFormat.tm_mday == -1 || timeFormat.tm_mon == -1 || timeFormat.tm_year == -1)
    {
        LogErrorF("Failed to parse time format struct from date '%s'", date.CString());
        return 0;
    }
#ifdef WIN32
    return _mkgmtime(&timeFormat);
#else
    // @todo Is this portable for platforms we are targetting?
    // http://man7.org/linux/man-pages/man3/timegm.3.html
    // mktime will return the local timezone epoch which is not what we want,
    // the env variable hack described in the above will probably be slow.
    //return mktime(&timeFormat);
    return timegm(&timeFormat);
#endif
}

// RequestData

RequestData::RequestData() :
    curlHandle(0),
    curlHeaders(0),
    msecSpent(0),
    bodyWritePos(0),
    method(-1)
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
            PrintRaw(Urho3D::ToString("  '%s': '%s'\n", iter->first.CString(), iter->second.CString()));
    }
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
    downloadBytesPerSec(0.0),
    uploadBytesPerSec(0.0),
    headersParsed(false)
{
}

}
}
