// For conditions of distribution and use, see copyright notice in LICENSE

//#include "StableHeaders.h"
#include "HttpAssetTransfer.h"
#include "HttpAssetProvider.h"
#include "HttpRequest.h"

#include "AssetAPI.h"
#include "AssetCache.h"

#include "Framework.h"
#include "LoggingFunctions.h"

namespace Tundra
{

HttpAssetTransfer::HttpAssetTransfer(HttpAssetProvider *provider, HttpRequestPtr &request, const String &assetRef_, const String &assetType_) :
    provider_(provider)
{
    // Prepare IAssetTransfer
    source.ref = assetRef_;
    assetType = assetType_;

    /* At this point we don't know if we can use the cached asset. 
       Once 304 response is detected, this will be changed to Cached. */
    diskSourceType = IAsset::Original; 

    // Cache destination path and source for 'If-Modified-Since' for a '304 Not Modified' response.
    if (provider_->Fw()->Asset()->Cache())
    {
        String cacheFile = provider_->Fw()->Asset()->Cache()->DiskSourceByRef(source.ref);
        request->SetCacheFile(cacheFile, true);
        /* Indicated so AssetAPI that we will take care of writing the cache, but it can find
           the source file from this path. */
        SetCachingBehavior(false, cacheFile);
    }

    // Connect to finished signal
    request->Finished.Connect(this, &HttpAssetTransfer::OnFinished);
}

HttpAssetTransfer::~HttpAssetTransfer()
{
}

void HttpAssetTransfer::OnFinished(HttpRequestPtr &request, int status, const String &error)
{
    // We can consider 200 and 304 as success. Other 3xx codes may represent redirects to the real location,
    // but these redirects are automatically detected and executed by HttpRequest.
    if ((status == 200 || status == 304) && error.Empty())
    {
        /* 304 Not Modified
           1) HttpRequest has already written file to asset cache file set with HttpRequest::SetCacheFile()
           2) Mark disk source as cached. Previous SetCachingBehavior already marked so that AssetAPI wont
              rewrite the disk file even if we provide rawAssetData.
           3) If we do not load 'rawAssetData' with a valid disk source. AssetAPI will do the right thing and load
              bytes from disk. This was desirable with Ogre as its threading mechanisms let us pass a filepath.
              If it was a certain type of Ogre asset, the disk read was skipped and path was used. With Urho it is more efficient 
              to do a memcpy here and not re-read from disk, which is slower as we already have the file in memory here.
              @todo In other works for above: AssetAPI and its 'data shuffling' could be re-thinked with Urho. */
        if (status == 304)
            diskSourceType = IAsset::Cached;

        request->CopyResponseBodyTo(rawAssetData);

        provider_->Fw()->Asset()->AssetTransferCompleted(this);
    }
    else
        provider_->Fw()->Asset()->AssetTransferFailed(this, (!error.Empty() ? error : Urho3D::ToString("%d %s", status, request->Status().CString())));
}

}
