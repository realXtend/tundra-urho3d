// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "CoreTypes.h"
#include "CoreStringUtils.h"

#include <Engine/Container/Ptr.h>
#include <Engine/Container/HashMap.h>

#include <map>

namespace Tundra
{
    class HttpAssetProvider;
    class HttpAssetTransfer;

    typedef SharedPtr<HttpAssetProvider> HttpAssetProviderPtr;
    typedef SharedPtr<HttpAssetTransfer> HttpAssetTransferPtr;

    class HttpPlugin;
    class HttpWorkQueue;
    class HttpClient;
    class HttpRequest;

    typedef SharedPtr<HttpWorkQueue> HttpWorkQueuePtr;
    typedef WeakPtr<HttpWorkQueue> HttpWorkQueueWeakPtr;
    typedef SharedPtr<HttpClient> HttpClientPtr;
    typedef WeakPtr<HttpClient> HttpClientWeakPtr;
    typedef SharedPtr<HttpRequest> HttpRequestPtr;
    typedef Vector<HttpRequestPtr> HttpRequestPtrList;

    typedef std::map<String, String, StringCompareCaseInsensitive> HttpHeaderMap;

    namespace Curl
    {
        struct Option;
        
        typedef HashMap<String, Option> OptionMap;
        typedef void RequestHandle;
        typedef void EngineHandle;
    }

    /// @cond PRIVATE
    namespace Http
    {
        struct Stats;
    }
    class HttpWorkThread;
    typedef Vector<HttpWorkThread*> HttpWorkThreadList;
    class HttpHudPanel;
    /// @endcond
}
