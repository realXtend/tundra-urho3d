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

/* Tundra note: Following ifdef block/function was copied from
   http://src.chromium.org/svn/trunk/src/base/os_compat_android.cc */
#if defined(ANDROID) && !defined(__LP64__)
// 32-bit Android has only timegm64() and not timegm().
// We replicate the behaviour of timegm() when the result overflows time_t.
#include <time64.h>

time_t timegm(struct tm* const t) {
  // time_t is signed on Android.
  static const time_t kTimeMax = ~(1L << (sizeof(time_t) * CHAR_BIT - 1));
  static const time_t kTimeMin = (1L << (sizeof(time_t) * CHAR_BIT - 1));
  time64_t result = timegm64(t);
  if (result < kTimeMin || result > kTimeMax)
    return 0;
  return result;
}
#endif

time_t HttpDateToUtcEpoch(const String &date)
{
    if (date.Empty())
        return 0;

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
    msecNetwork(-1),
    msecDiskRead(-1),
    msecDiskWrite(-1),
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
    downloadBytesPerSec(-1.0),
    uploadBytesPerSec(-1.0),
    headersParsed(false)
{
}

// Stats

Stats::Stats() :
    requests(0),
    errors(0),
    downloads(0),
    uploads(0),
    diskReads(0),
    diskWrites(0)
{   
}

String PadDouble(double value, uint pad, int digits = 4)
{
    String temp = Urho3D::ToString("%f", value);
    if (digits > -1)
    {
        uint i = temp.Find('.');
        if (i != String::NPOS && i + 1 + digits < temp.Length())
            temp = temp.Substring(0, i + 1 + digits);
    }
    String v = PadString(temp, pad);
    if (v.Length() >= pad) // cut digits
        v = v.Substring(0, pad);
    return v; 
}

void Stats::Dump(bool averages_, bool current_)
{
    StringVector lines = GetData(averages_, current_).Split('\n');
    foreach(auto &line, lines)
        LogInfo("[HttpStats] " + line);
}

String Stats::GetData(bool averages_, bool current_)
{
    String str;
    str.AppendWithFormat("%s %s %s %s %s\n\n",
        PadString("", 12).CString(),
        PadString("Download", 10).CString(),
        PadString("Upload", 10).CString(),
        PadString("Cache Read", 12).CString(),
        PadString("Cache Write", 12).CString()
    );
    str.AppendWithFormat("%s %s %s %s %s\n",
        PadString("Count", 12).CString(),
        PadString(downloads, 10).CString(),
        PadString(uploads, 10).CString(),
        PadString(diskReads, 12).CString(),
        PadString(diskWrites, 12).CString()
    );
    str.AppendWithFormat("%s %s %s %s %s MB\n",
        PadString("Size", 12).CString(),
        PadDouble((totals.downloadBytes / 1024.0) / 1024.0, 10).CString(),
        PadDouble((totals.uploadBytes / 1024.0) / 1024.0, 10).CString(),
        PadDouble((totals.diskReadBytes / 1024.0) / 1024.0, 12).CString(),
        PadDouble((totals.diskWriteBytes / 1024.0) / 1024.0, 12).CString()
    );
    if (averages_)
    {
        str.AppendWithFormat("%s %s %s %s %s msec\n",
            PadString("Avg. time", 12).CString(),
            PadDouble(averages.msecDownload > -1.0 ? averages.msecDownload : 0.0, 10).CString(),
            PadDouble(averages.msecUpload > -1.0 ? averages.msecUpload : 0.0, 10).CString(),
            PadDouble(averages.msecDiskRead > -1.0 ? averages.msecDiskRead : 0.0, 12).CString(),
            PadDouble(averages.msecDiskWrite > -1.0 ? averages.msecDiskWrite : 0.0, 12).CString()
        );
    }
    str.AppendWithFormat("%s %s %s %s %s seconds\n",
        PadString("Total time", 12).CString(),
        PadDouble(totals.msecDownload / 1000.0, 10).CString(),
        PadDouble(totals.msecUpload / 1000.0, 10).CString(),
        PadDouble(totals.msecDiskRead / 1000.0, 12).CString(),
        PadDouble(totals.msecDiskWrite / 1000.0, 12).CString()
    );
    if (averages_)
    {
        str.AppendWithFormat("%s %s %s %s %s kB/sec\n",
            PadString("Avg. speed", 12).CString(),
            PadDouble(averages.downloadBytesPerSec > -1.0 ? averages.downloadBytesPerSec / 1024.0 : 0.0, 10).CString(),
            PadDouble(averages.uploadBytesPerSec > -1.0 ? averages.uploadBytesPerSec / 1024.0 : 0.0, 10).CString(),
            PadString("", 12).CString(),
            PadString("", 12).CString()
        );
        str.AppendWithFormat("%s %s %s %s %s kB/sec\n",
            PadString("Best speed", 12).CString(),
            PadDouble(averages.bestDownloadBytesPerSec > -1.0 ? averages.bestDownloadBytesPerSec / 1024.0 : 0.0, 10).CString(),
            PadDouble(averages.bestUploadBytesPerSec > -1.0 ? averages.bestUploadBytesPerSec / 1024.0 : 0.0, 10).CString(),
            PadString("", 12).CString(),
            PadString("", 12).CString()
        );
    }
    if (current_)
    {
        str.AppendWithFormat("\n%s %d\n%s %d\n",
            PadString("Pending", 12).CString(), current.pending,
            PadString("Executing", 12).CString(), current.executing
        );
        if (current.idle >= 0.f)
        {
            str.AppendWithFormat("%s %s %s seconds idle\n",
                PadString("Threads", 12).CString(),
                PadString(current.threads, 10).CString(),
                PadDouble(current.idle, 10, 2).CString()
            );
        }
        else
            str.AppendWithFormat("%s %d\n", PadString("Threads", 12).CString(), current.threads);
    }
    return str;
}

// Stats::Current

Stats::Current::Current() :
    pending(0),
    executing(0),
    threads(0),
    idle(-1.f)
{
}

// Stats::IO

Stats::Totals::Totals() :
    msecDownload(0),
    msecUpload(0),
    msecDiskRead(0),
    msecDiskWrite(0),
    downloadBytes(0),
    uploadBytes(0),
    diskReadBytes(0),
    diskWriteBytes(0)
{
}

// Stats::Averages

Stats::Averages::Averages() :
    msecDownload(-1.0),
    msecUpload(-1.0),
    msecDiskRead(-1.0),
    msecDiskWrite(-1.0),
    downloadBytesPerSec(-1.0),
    uploadBytesPerSec(-1.0),
    bestDownloadBytesPerSec(-1.0),
    bestUploadBytesPerSec(-1.0)
{
}

}
}
