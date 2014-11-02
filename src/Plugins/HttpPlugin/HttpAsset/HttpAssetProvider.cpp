// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "HttpClient.h"
#include "HttpRequest.h"
#include "HttpAssetProvider.h"
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

AssetStoragePtr HttpAssetProvider::TryDeserializeStorageFromString(const String &storage, bool fromNetwork)
{
    return AssetStoragePtr(); /// @todo
}

}
