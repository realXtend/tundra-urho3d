// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "HttpClient.h"
#include "HttpRequest.h"

#include "HttpAssetProvider.h"
#include "HttpAssetStorage.h"
#include "HttpAssetTransfer.h"

#include "AssetAPI.h"
#include "AssetCache.h"
#include "IAssetTransfer.h" /// @todo HttpAssetTransfer

#include "Framework.h"

namespace Tundra
{

HttpAssetProvider::HttpAssetProvider(Framework *framework, const HttpClientPtr &client) :
    IAssetProvider(framework->GetContext()),
    framework_(framework),
    client_(client)
{
}

HttpAssetProvider::~HttpAssetProvider()
{
}

AssetStoragePtr HttpAssetProvider::StorageForBaseURL(const String &url) const
{
    AssetStorageVector storages = framework_->Asset()->AssetStorages();
    foreach(const AssetStoragePtr &storage, storages)
    {
        HttpAssetStorage *httpStorage = dynamic_cast<HttpAssetStorage*>(storage.Get());
        if (httpStorage && httpStorage->BaseURL().Compare(url, true) == 0)
            return storage;
    }
    return AssetStoragePtr();
}

String HttpAssetProvider::UniqueName(String prefix) const
{
    if (prefix.Empty())
        prefix = "Web";
    int counter = 0;

    AssetStorageVector storages = framework_->Asset()->AssetStorages();
    while(counter < 1000)
    {
        bool reserved = false;
        foreach(const AssetStoragePtr &storage, storages)
        {
            HttpAssetStorage *httpStorage = dynamic_cast<HttpAssetStorage*>(storage.Get());
            if (httpStorage && httpStorage->Name().Compare(prefix, false) == 0)
            {
                reserved = true;
                break;
            }
        }
        if (!reserved)
            return prefix;
        prefix = Urho3D::ToString("%s %d", prefix, counter++);
    }
    return "";
}

// IAssetProvider implementaion

String HttpAssetProvider::Name() const
{
    return "HttpAssetProvider";
}

bool HttpAssetProvider::IsValidRef(String assetRef, String assetType) const
{
    // http and https are valid
    assetRef = assetRef.Trimmed().ToLower();
    return (assetRef.StartsWith("http://") || assetRef.StartsWith("https://"));
}

AssetTransferPtr HttpAssetProvider::RequestAsset(String assetRef, String assetType)
{
    assetRef = assetRef.Trimmed();

    HttpRequestPtr request = client_->Get(assetRef);
    if (!request)
        return AssetTransferPtr();
    
    AssetTransferPtr transfer(new HttpAssetTransfer(this, request, assetRef, assetType));
    transfer->provider = this;

    return transfer;
}

bool HttpAssetProvider::AbortTransfer(IAssetTransfer *transfer)
{
     return false; /// @todo
}

void HttpAssetProvider::Update(float UNUSED_PARAM(frametime))
{
     /// @todo
}

void HttpAssetProvider::DeleteAssetFromStorage(String assetRef)
{
     /// @todo
}

bool HttpAssetProvider::RemoveAssetStorage(String storageName)
{
    return false; /// @todo
}

Vector<AssetStoragePtr> HttpAssetProvider::Storages() const
{
    return Vector<AssetStoragePtr>(); /// @todo
}

AssetStoragePtr HttpAssetProvider::StorageByName(const String &name) const
{
    return AssetStoragePtr(); /// @todo
}

AssetStoragePtr HttpAssetProvider::StorageForAssetRef(const String &assetRef) const
{
    return AssetStoragePtr(); /// @todo
}

AssetUploadTransferPtr HttpAssetProvider::UploadAssetFromFileInMemory(const u8 *data, uint numBytes,
    AssetStoragePtr destination, const String &assetName)
{
    return AssetUploadTransferPtr(); /// @todo
}

AssetStoragePtr HttpAssetProvider::TryCreateStorage(HashMap<String, String> &storageParams, bool fromNetwork)
{
    if (!storageParams.Contains("src") || !IsValidRef(storageParams["src"], ""))
        return AssetStoragePtr();
    if (storageParams.Contains("type") && storageParams["type"].Compare("HttpAssetStorage", false) != 0)
        return AssetStoragePtr();

    String baseUrl = storageParams["src"];
    if (!baseUrl.EndsWith("/") && baseUrl.Contains("/"))
        baseUrl = baseUrl.Substring(0, baseUrl.FindLast('/')+1);
    if (!baseUrl.EndsWith("/"))
        return AssetStoragePtr();

    String name = UniqueName(storageParams["name"]);

    // @todo liveupdate, liveupload, autodiscoverable etc. when actually needed
    AssetStoragePtr storage = StorageForBaseURL(baseUrl);
    if (!storage)
        storage = AssetStoragePtr(new HttpAssetStorage(framework_->GetContext(), name, baseUrl, storageParams["localdir"]));

    storage->SetReplicated(Urho3D::ToBool(storageParams["replicated"]));
    return storage;
}

}
