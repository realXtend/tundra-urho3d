// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "HttpPluginApi.h"
#include "HttpPluginFwd.h"

#include "IAssetProvider.h"

namespace Tundra
{

/// HTTP asset provider.
class TUNDRA_HTTP_API HttpAssetProvider : public IAssetProvider
{
    OBJECT(HttpAssetProvider);

public:
    HttpAssetProvider(Framework *framework, const HttpClientPtr &client);
    ~HttpAssetProvider();

    Framework *Fw() { return framework_; }

    /// IAssetProvider override.
    String Name() const override;
    /// IAssetProvider override.
    bool IsValidRef(String assetRef, String assetType) const override;
    /// IAssetProvider override.
    AssetTransferPtr CreateTransfer(String assetRef, String assetType) override;
    /// IAssetProvider override.
    void ExecuteTransfer(AssetTransferPtr transfer) override;
    /// IAssetProvider override.
    bool AbortTransfer(IAssetTransfer *transfer) override;
    /// IAssetProvider override.
    void DeleteAssetFromStorage(String assetRef) override;
    /// IAssetProvider override.
    bool RemoveAssetStorage(String storageName) override;
    /// IAssetProvider override.
    Vector<AssetStoragePtr> Storages() const override;
    /// IAssetProvider override.
    AssetStoragePtr StorageByName(const String &name) const override;
    /// IAssetProvider override.
    AssetStoragePtr StorageForAssetRef(const String &assetRef) const override;
    /// IAssetProvider override.
    AssetUploadTransferPtr UploadAssetFromFileInMemory(const u8 *data, uint numBytes,
        AssetStoragePtr destination, const String &assetName) override;

private:
    /// IAssetProvider override.
    AssetStoragePtr TryCreateStorage(HashMap<String, String> &storageParams, bool fromNetwork) override;

    /// Returns existing HTTP storage for base URL.
    AssetStoragePtr StorageForBaseURL(const String &url) const;

    /// Returns a uniqeu HTTP storage name.
    String UniqueName(String prefix = "Web") const;

    Framework *framework_;
    HttpClientPtr client_;

    Vector<AssetStoragePtr> httpStorages_;
};

}
