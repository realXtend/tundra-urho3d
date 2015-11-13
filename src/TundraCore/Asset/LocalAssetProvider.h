// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"
#include "IAssetProvider.h"
#include "AssetFwd.h"

namespace Tundra
{

/// Provides access to files on the local file system using the 'local://' URL specifier.
class TUNDRACORE_API LocalAssetProvider : public IAssetProvider
{
    URHO3D_OBJECT(LocalAssetProvider, IAssetProvider);

public:
    explicit LocalAssetProvider(Framework* framework);
    ~LocalAssetProvider();

    /// Adds the given directory as an asset storage.
    /** @param directory The path name for the directory to add.
        @param storageName An identifier for the storage. Remember that Asset Storage names are case-insensitive.
        @param recursive If true, all the subfolders of the given folder are added as well.
        @param writable If true, assets can be uploaded to the storage.
        @param liveUpdate If true, assets will be reloaded when the underlying file changes.
        @param autoDiscoverable If true, a recursive directory search will be initially performed to know which assets reside inside the storage.
        Returns the newly created storage, or 0 if a storage with the given name already existed, or if some other error occurred. */
    LocalAssetStoragePtr AddStorageDirectory(String directory, String storageName, bool recursive, bool writable = true, bool liveUpdate = true, bool autoDiscoverable = true, bool replicated = false, const String &trustStateStr = "");

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
    void Update(float frametime) override;
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
    AssetUploadTransferPtr UploadAssetFromFileInMemory(const u8 *data, uint numBytes, AssetStoragePtr destination, const String &assetName) override;

private:
    /// IAssetProvider override.
    AssetStoragePtr TryCreateStorage(HashMap<String, String> &storageParams, bool fromNetwork) override;

    /// Returns LocalAssetStorage for specific @c path. The @c path can be root directory of storage or any of its subdirectories.
    LocalAssetStoragePtr FindStorageForPath(const String &path) const;

    /// Generates a unique storage name for local storages.
    String GenerateUniqueStorageName() const;

    /// Finds a path where the file localFilename can be found. Searches through all local storages.
    /// @param storage [out] Receives the local storage that contains the asset.
    String GetPathForAsset(const String &localFilename, LocalAssetStoragePtr *storage) const;

    /// Takes all the pending file download transfers and finishes them.
    void CompletePendingFileDownloads();

    /// Takes all the pending file upload transfers and finishes them.
    void CompletePendingFileUploads();

    /// Checks for pending file systems changes and updates 
    void CheckForPendingFileSystemChanges();

    /// Return current directory for making local asset directories absolute if specified as relative. On Android this will refer to the apk instead of the actual current directory
    String AbsoluteBaseDirectory() const;
     
    Framework *framework;
    Vector<LocalAssetStoragePtr> storages;          ///< Asset directories to search, may be recursive or not
    Vector<AssetUploadTransferPtr> pendingUploads;  ///< The following asset uploads are pending to be completed by this provider.
    Vector<AssetTransferPtr> pendingDownloads;      ///< The following asset downloads are pending to be completed by this provider.

    /// If true, assets outside any known local storages are allowed. Otherwise, requests to them will fail.
    bool enableRequestsOutsideStorages;

    void OnFileChanged(const String &path);
    void OnDirectoryChanged(const String &path);
};

}
