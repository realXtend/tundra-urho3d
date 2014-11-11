// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "HttpPluginApi.h"
#include "HttpPluginFwd.h"
#include "HttpDefines.h"
#include "FrameworkFwd.h"

#include "HttpWorkQueue.h"

#include <Engine/Container/RefCounted.h>

namespace Tundra
{

/// HTTP client
class TUNDRA_HTTP_API HttpClient : public Urho3D::RefCounted
{   
public:
    HttpClient(Framework *framework);
    ~HttpClient();

    /// Execute GET
    /** @see https://tools.ietf.org/html/rfc2616#section-9.3 */
    HttpRequestPtr Get(const String &url);

    /// Execute HEAD
    /** @see https://tools.ietf.org/html/rfc2616#section-9.4 */
    HttpRequestPtr Head(const String &url);

    /// Execute OPTIONS
    /** @see https://tools.ietf.org/html/rfc2616#section-9.2 */
    HttpRequestPtr Options(const String &url);

    /// Execute POST
    /** @see https://tools.ietf.org/html/rfc2616#section-9.5 */
    HttpRequestPtr Post(const String &url, const Vector<u8> &body = Vector<u8>(), const String &contentType = "application/octet-stream");
    HttpRequestPtr Post(const String &url, const String &body, const String &contentType = "text/plain"); ///< @overload

    /// Execute PUT
    /** @see https://tools.ietf.org/html/rfc2616#section-9.6 */
    HttpRequestPtr Put(const String &url, const Vector<u8> &body = Vector<u8>(), const String &contentType = "application/octet-stream");
    HttpRequestPtr Put(const String &url, const String &body, const String &contentType = "text/plain"); ///< @overload

    /// Execute PATCH
    /** @see https://tools.ietf.org/html/rfc2068#section-19.6.1.1 and https://tools.ietf.org/html/rfc5789 */
    HttpRequestPtr Patch(const String &url, const Vector<u8> &body = Vector<u8>(), const String &contentType = "application/octet-stream");
    HttpRequestPtr Patch(const String &url, const String &body, const String &contentType = "text/plain"); ///< @overload

    /// Execute DELETE
    /** @see https://tools.ietf.org/html/rfc2616#section-9.7 */
    HttpRequestPtr Delete(const String &url);

    /// HTTP client stats.
    Http::Stats *Stats() const;

private:
    /// Create a request without scheduling it.
    HttpRequestPtr Create(int method, const String &url);
    HttpRequestPtr Create(int method, const String &url, const Vector<u8> &body, const String &contentType);

    /// Schedule a request.
    HttpRequestPtr Schedule(int method, const String &url);
    HttpRequestPtr Schedule(int method, const String &url, const Vector<u8> &body, const String &contentType);
    bool Schedule(HttpRequestPtr request);

    friend class HttpPlugin;
    friend class HttpAssetProvider;
    void Update(float frametime);

    void DumpStats() const;

    Framework *framework_;
    HttpWorkQueuePtr queue_;    
};

}
