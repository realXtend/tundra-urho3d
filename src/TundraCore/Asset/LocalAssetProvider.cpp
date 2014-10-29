// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

//#include "Win.h"
#include "LocalAssetProvider.h"
#include "LocalAssetStorage.h"
#include "IAssetUploadTransfer.h"
#include "IAssetTransfer.h"
#include "AssetAPI.h"
#include "IAsset.h"

#include "Framework.h"
#include "LoggingFunctions.h"
#include "CoreStringUtils.h"

#include <Profiler.h>
#include <FileSystem.h>
#include <File.h>
#include <Timer.h>
#include <StringUtils.h>
#include <FileWatcher.h>

namespace Tundra
{

LocalAssetProvider::LocalAssetProvider(Framework* framework_) :
    IAssetProvider(framework_->GetContext()),
    framework(framework_)
{
    enableRequestsOutsideStorages = (framework_->HasCommandLineParameter("--acceptUnknownLocalSources") ||
        framework_->HasCommandLineParameter("--accept_unknown_local_sources"));  /**< @todo Remove support for the deprecated underscore version at some point. */
}

LocalAssetProvider::~LocalAssetProvider()
{
}

String LocalAssetProvider::Name() const
{
    static const String name("Local");
    return name;
}

bool LocalAssetProvider::IsValidRef(String assetRef, String) const
{
    AssetAPI::AssetRefType refType = AssetAPI::ParseAssetRef(assetRef.Trimmed());
    if (refType == AssetAPI::AssetRefLocalPath || refType == AssetAPI::AssetRefLocalUrl)
        return true;

    if (refType == AssetAPI::AssetRefRelativePath)
    {
        String path = GetPathForAsset(assetRef, 0);
        return !path.Empty();
    }
    else
        return false;
}

AssetTransferPtr LocalAssetProvider::RequestAsset(String assetRef, String assetType)
{
    PROFILE(LocalAssetProvider_RequestAsset);
    if (assetRef.Empty())
        return AssetTransferPtr();
    assetType = assetType.Trimmed();
    if (assetType.Empty())
        assetType = framework->Asset()->ResourceTypeForAssetRef(assetRef);

    if (!enableRequestsOutsideStorages)
    {
        AssetStoragePtr storage = StorageForAssetRef(assetRef);
        if (!storage)
        {
            LogError("LocalAssetProvider::RequestAsset: Discarding asset request to path \"" + assetRef +
                "\" because requests to sources outside registered LocalAssetStorages have been forbidden. (See --acceptUnknownLocalSources).");
            return AssetTransferPtr();
        }
    }

    AssetTransferPtr transfer(new IAssetTransfer);
    transfer->source.ref = assetRef.Trimmed();
    transfer->assetType = assetType;
    transfer->diskSourceType = IAsset::Original; // The disk source represents the original authoritative source for the asset.
    
    pendingDownloads.Push(transfer);

    return transfer;
}

bool LocalAssetProvider::AbortTransfer(IAssetTransfer *transfer)
{
    if (!transfer)
        return false;

    for (Vector<AssetTransferPtr>::Iterator iter = pendingDownloads.Begin(); iter != pendingDownloads.End(); ++iter)
    {
        AssetTransferPtr ongoingTransfer = (*iter);
        if (ongoingTransfer.Get() == transfer)
        {
            framework->Asset()->AssetTransferAborted(transfer);
            
            ongoingTransfer.Reset();
            pendingDownloads.Erase(iter);
            return true;
        }
    }
    return false;
}

String LocalAssetProvider::GetPathForAsset(const String &assetRef, LocalAssetStoragePtr *storage) const
{
    String path;
    String path_filename;
    AssetAPI::AssetRefType refType = AssetAPI::ParseAssetRef(assetRef.Trimmed(), 0, 0, 0, 0, &path_filename, &path);
    if (refType == AssetAPI::AssetRefLocalPath)
    {
        // If the asset ref has already been converted to an absolute path, simply return the assetRef as is.
        // However, lookup also the storage if wanted
        if (storage)
        {
            for (uint i = 0; i < storages.Size(); ++i)
            {
                if (path.StartsWith(storages[i]->directory, false))
                {
                    *storage = storages[i];
                    return path;
                }
            }
        }
        
        return path;
    }
    // Check first all subdirs without recursion, because recursion is potentially slow
    for (uint i = 0; i < storages.Size(); ++i)
    {
        String path = storages[i]->GetFullPathForAsset(path_filename, false);
        if (path != "")
        {
            if (storage)
                *storage = storages[i];
            return path;
        }
    }

    for (uint i = 0; i < storages.Size(); ++i)
    {
        String path = storages[i]->GetFullPathForAsset(path_filename, true);
        if (path != "")
        {
            if (storage)
                *storage = storages[i];
            return path;
        }
    }
    
    if (storage)
        *storage = LocalAssetStoragePtr();
    return "";
}

void LocalAssetProvider::Update(float /*frametime*/)
{
    PROFILE(LocalAssetProvider_Update);

    ///@note It is *very* important that below we first complete all uploads, and then the downloads.
    /// This is because it is a rather common code flow to upload an asset for an entity, and immediately after that
    /// generate a entity in the scene that refers to that asset, which means we do both an upload and a download of the
    /// asset into the same asset storage. If the download request was processed before the upload request, the download
    /// request would fail on missing file, and the entity would erroneously get an "asset not found" result.
    CompletePendingFileUploads();
    CompletePendingFileDownloads();
    CheckForPendingFileSystemChanges();
}

void LocalAssetProvider::DeleteAssetFromStorage(String assetRef)
{
    Urho3D::FileSystem* fileSystem = GetSubsystem<Urho3D::FileSystem>();

    if (!assetRef.Empty())
    {
        LocalAssetStoragePtr storage;
        String path = GetPathForAsset(assetRef, &storage);
        if (!storage)
        {
            LogError("LocalAssetProvider::DeleteAssetFromStorage: Could not verify the asset storage pointed by \"" + assetRef + "\"!");
            return;
        }

        String fullFilename = path + '/' + AssetAPI::ExtractFilenameFromAssetRef(assetRef);
        bool success = fileSystem->Delete(fullFilename);
        if (success)
        {
            LogInfo("LocalAssetProvider::DeleteAssetFromStorage: Deleted asset \"" + assetRef + "\", file " + fullFilename + " from disk.");
            framework->Asset()->EmitAssetDeletedFromStorage(assetRef);
        }
        else
        {
            LogError("Could not delete asset " + assetRef);
        }
    }
}

bool LocalAssetProvider::RemoveAssetStorage(String storageName)
{
    for(uint i = 0; i < storages.Size(); ++i)
        if (storages[i]->name.Compare(storageName, false) == 0)
        {
            storages.Erase(storages.Begin() + i);
            return true;
        }

    return false;
}

LocalAssetStoragePtr LocalAssetProvider::AddStorageDirectory(String directory, String storageName, bool recursive, bool writable, bool liveUpdate, bool autoDiscoverable)
{
    directory = directory.Trimmed();
    if (directory.Empty())
    {
        LogError("LocalAssetProvider: Cannot add storage \"" + storageName + "\" to an empty directory!");
        return LocalAssetStoragePtr();
    }
    directory = Urho3D::GetInternalPath(GuaranteeTrailingSlash(directory));
    if (!Urho3D::IsAbsolutePath(directory))
        directory = GetSubsystem<Urho3D::FileSystem>()->GetCurrentDir() + directory;

    storageName = storageName.Trimmed();
    if (storageName.Empty())
    {
        LogInfo("LocalAssetProvider: Cannot add storage with an empty name to directory \"" + directory + "\"!");
        return LocalAssetStoragePtr();
    }

    // Test if we already have a storage registered with this name.
    for(uint i = 0; i < storages.Size(); ++i)
        if (storages[i]->name.Compare(storageName, false) == 0)
        {
            if (storages[i]->directory != directory)
            {
                LogWarning("LocalAssetProvider: Storage '" + storageName + "' already exist in '" + storages[i]->directory + "', not adding with '" + directory + "'.");
                return LocalAssetStoragePtr();
            }
            else // We already have a storage with that name and target directory registered, just return that.
                return storages[i];
        }

    //LogInfo("LocalAssetProvider::AddStorageDirectory " + directory);
    LocalAssetStoragePtr storage(new LocalAssetStorage(GetContext(), writable, liveUpdate, autoDiscoverable));
    storage->directory = directory;
    storage->name = storageName;
    storage->recursive = recursive;
    storage->provider = this;
// On Android, we get spurious file change notifications. Disable watcher for now.
#ifndef ANDROID
    if (!framework->HasCommandLineParameter("--noFileWatcher"))
        storage->SetupWatcher(); // Start listening on file change notifications. Note: it's important that recursive is set before calling this!
#endif
    storages.Push(storage);

    // Tell the Asset API that we have created a new storage.
    framework->Asset()->EmitAssetStorageAdded(AssetStoragePtr(storage));

    // If autodiscovery is on, make the storage refresh itself immediately.
    if (storage->AutoDiscoverable())
        storage->RefreshAssetRefs();

    return storage;
}

Vector<AssetStoragePtr> LocalAssetProvider::Storages() const
{
    Vector<AssetStoragePtr> stores;
    for(uint i = 0; i < storages.Size(); ++i)
        stores.Push(AssetStoragePtr(storages[i]));
    return stores;
}

AssetUploadTransferPtr LocalAssetProvider::UploadAssetFromFileInMemory(const u8 *data, uint numBytes, AssetStoragePtr destination, const String &assetName)
{
    assert(data);
    if (!data)
    {
        LogError("LocalAssetProvider::UploadAssetFromFileInMemory: Null source data pointer passed to function!");
        return AssetUploadTransferPtr();
    }

    LocalAssetStorage *storage = dynamic_cast<LocalAssetStorage*>(destination.Get());
    if (!storage)
    {
        LogError("LocalAssetProvider::UploadAssetFromFileInMemory: Invalid destination asset storage type! Was not of type LocalAssetStorage!");
        return AssetUploadTransferPtr();
    }

    AssetUploadTransferPtr transfer(new IAssetUploadTransfer(GetContext()));
    transfer->sourceFilename = "";
    transfer->destinationName = assetName;
    transfer->destinationStorage = destination;
    transfer->assetData.Insert(transfer->assetData.End(), data, data + numBytes);

    pendingUploads.Push(transfer);

    return transfer;
}

void LocalAssetProvider::CompletePendingFileDownloads()
{
    // If we have any uploads running, first wait for each of them to complete, until we download any more.
    // This is because we might want to download the same asset that we uploaded, so they must be done in
    // the proper order.
    if (pendingUploads.Size() > 0)
        return;

    const int maxLoadMSecs = 16;
    Urho3D::HiresTimer downloadTimer;

    while(pendingDownloads.Size() > 0)
    {
        PROFILE(LocalAssetProvider_ProcessPendingDownload);

        AssetTransferPtr transfer = pendingDownloads.Back();
        pendingDownloads.Pop();
            
        String ref = transfer->source.ref;

        String path_filename;
        String file;
        AssetAPI::AssetRefType refType = AssetAPI::ParseAssetRef(ref.Trimmed(), 0, 0, 0, 0, &path_filename);

        LocalAssetStoragePtr storage;

        if (refType == AssetAPI::AssetRefLocalPath)
        {
            file = path_filename;
        }
        else // Using a local relative path, like "local://asset.ref" or "asset.ref".
        {
            AssetAPI::AssetRefType urlRefType = AssetAPI::ParseAssetRef(path_filename);
            if (urlRefType == AssetAPI::AssetRefLocalPath)
                file = path_filename; // 'file://C:/path/to/asset/asset.png'.
            else // The ref is of form 'file://relativePath/asset.png'.
            {
                String path = GetPathForAsset(path_filename, &storage);
                if (path.Empty())
                {
                    String reason = "Failed to find local asset with filename \"" + ref + "\"!";
                    framework->Asset()->AssetTransferFailed(transfer.Get(), reason);
                    // Also throttle asset loading here. This is needed in the case we have a lot of failed refs.
                    //if (GetCurrentClockTime() - startTime >= GetCurrentClockFreq() * maxLoadMSecs / 1000)
                    //    break;
                    continue;
                }
            
                file = GuaranteeTrailingSlash(path) + path_filename;
            }
        }

        bool success = LoadFileToVector(file, transfer->rawAssetData);
        if (!success)
        {
            String reason = "Failed to read asset data for asset \"" + ref + "\" from file \"" + file + "\"";
            framework->Asset()->AssetTransferFailed(transfer.Get(), reason);
            // Also throttle asset loading here. This is needed in the case we have a lot of failed refs.
            if (downloadTimer.GetUSec(false) / 1000 > maxLoadMSecs)
                break;
            continue;
        }
        
        // Tell the Asset API that this asset should not be cached into the asset cache, and instead the original filename should be used
        // as a disk source, rather than generating a cache file for it.
        transfer->SetCachingBehavior(false, file);
        transfer->storage = storage;

        // Signal the Asset API that this asset is now successfully downloaded.
        framework->Asset()->AssetTransferCompleted(transfer.Get());

        // Throttle asset loading to at most 16 msecs/frame.
        if (downloadTimer.GetUSec(false) / 1000 > maxLoadMSecs)
            break;
    }
}

AssetStoragePtr LocalAssetProvider::TryDeserializeStorageFromString(const String &storage, bool /*fromNetwork*/)
{
    Urho3D::FileSystem* fileSystem = GetSubsystem<Urho3D::FileSystem>();

    HashMap<String, String> s = AssetAPI::ParseAssetStorageString(storage);
    if (s.Contains("type") && s["type"].Compare("LocalAssetStorage", false) != 0)
        return AssetStoragePtr();
    if (!s.Contains("src"))
        return AssetStoragePtr();

    String path;
    String protocolPath;
    AssetAPI::AssetRefType refType = AssetAPI::ParseAssetRef(s["src"], 0, 0, &protocolPath, 0, 0, &path);

    if (refType == AssetAPI::AssetRefRelativePath)
    {
        path = GuaranteeTrailingSlash(fileSystem->GetCurrentDir()) + path;
        refType = AssetAPI::AssetRefLocalPath;
    }
    if (refType != AssetAPI::AssetRefLocalPath)
        return AssetStoragePtr();

    String name = (s.Contains("name") ? s["name"] : GenerateUniqueStorageName());

    bool recursive = true;
    bool writable = true;
    bool liveUpdate = true;
    bool autoDiscoverable = true;
    if (s.Contains("recursive"))
        recursive = Urho3D::ToBool(s["recursive"]);

    if (s.Contains("readonly"))
        writable = !Urho3D::ToBool(s["readonly"]);

    if (s.Contains("liveupdate"))
        liveUpdate = Urho3D::ToBool(s["liveupdate"]);
    
    if (s.Contains("autodiscoverable"))
        autoDiscoverable = Urho3D::ToBool(s["autodiscoverable"]);

    LocalAssetStoragePtr storagePtr = AddStorageDirectory(path, name, recursive, writable, liveUpdate, autoDiscoverable);

    ///\bug Refactor these sets to occur inside AddStorageDirectory so that when the NewStorageAdded signal is emitted, these values are up to date.
    if (storagePtr)
    {
        if (s.Contains("replicated"))
            storagePtr->SetReplicated(Urho3D::ToBool(s["replicated"]));
        if (s.Contains("trusted"))
            storagePtr->trustState = IAssetStorage::TrustStateFromString(s["trusted"]);
    }

    return AssetStoragePtr(storagePtr);
}

String LocalAssetProvider::GenerateUniqueStorageName() const
{
    String name = "Scene";
    int counter = 2;
    while(StorageByName(name) != 0)
        name = "Scene" + String(counter++);
    return name;
}

LocalAssetStoragePtr LocalAssetProvider::FindStorageForPath(const String &path) const
{
    String normalizedPath = Urho3D::GetInternalPath(GuaranteeTrailingSlash(path));
    if (!IsAbsolutePath(normalizedPath))
        normalizedPath = GetSubsystem<Urho3D::FileSystem>()->GetCurrentDir() + normalizedPath;

    for(uint i = 0; i < storages.Size(); ++i)
        if (normalizedPath.Contains(GuaranteeTrailingSlash(storages[i]->directory)))
            return storages[i];
    return LocalAssetStoragePtr();
}

AssetStoragePtr LocalAssetProvider::StorageByName(const String &name) const
{
    for(uint i = 0; i < storages.Size(); ++i)
        if (storages[i]->name.Compare(name, false) == 0)
            return AssetStoragePtr(storages[i]);

    return AssetStoragePtr();
}

AssetStoragePtr LocalAssetProvider::StorageForAssetRef(const String &assetRef) const
{
    PROFILE(LocalAssetProvider_GetStorageForAssetRef);

    AssetAPI::AssetRefType refType = AssetAPI::ParseAssetRef(assetRef.Trimmed());
    if (refType != AssetAPI::AssetRefLocalPath && refType != AssetAPI::AssetRefLocalUrl)
        return AssetStoragePtr();

    LocalAssetStoragePtr storage;
    GetPathForAsset(assetRef, &storage);
    return AssetStoragePtr(storage);
}

void LocalAssetProvider::CompletePendingFileUploads()
{
    while(pendingUploads.Size() > 0)
    {
        PROFILE(LocalAssetProvider_ProcessPendingUpload);
        AssetUploadTransferPtr transfer = pendingUploads.Back();
        pendingUploads.Pop();

        LocalAssetStoragePtr storage = Urho3D::DynamicCast<LocalAssetStorage>(transfer->destinationStorage.Lock());
        if (!storage)
        {
            LogError("Invalid IAssetStorage specified for file upload in LocalAssetProvider!");
            transfer->EmitTransferFailed();
            continue;
        }

        if (transfer->sourceFilename.Length() == 0 && transfer->assetData.Size() == 0)
        {
            LogError("No source data present when trying to upload asset to LocalAssetProvider!");
            continue;
        }

        String fromFile = transfer->sourceFilename;
        String toFile = GuaranteeTrailingSlash(storage->directory) + transfer->destinationName;

        bool success;
        if (fromFile.Length() == 0)
            success = SaveAssetFromMemoryToFile(&transfer->assetData[0], transfer->assetData.Size(), toFile);
        else
            success = CopyAssetFile(fromFile, toFile);

        if (!success)
        {
            LogError("Asset upload failed in LocalAssetProvider: CopyAsset from \"" + fromFile + "\" to \"" + toFile + "\" failed!");
            transfer->EmitTransferFailed();
        }
        else
        {
            framework->Asset()->AssetUploadTransferCompleted(transfer.Get());
        }
    }
}

void LocalAssetProvider::CheckForPendingFileSystemChanges()
{
    PROFILE(LocalAssetProvider_CheckForPendingFileSystemChanges);
    Urho3D::FileSystem* fileSystem = GetSubsystem<Urho3D::FileSystem>();

    for(uint i = 0; i < storages.Size(); ++i)
    {
        LocalAssetStorage* storage = storages[i];
        if (!storage->changeWatcher)
            continue;
        String file;
        while (storage->changeWatcher->GetNextChange(file))
        {
            file = storage->directory + file;
            LogInfo(file);
            if (!storage->AutoDiscoverable())
            {
                LogWarning("Received file change notification for storage of which auto-discovery is false.");
                continue;
            }

            String assetRef = file;
            uint lastSlash = assetRef.FindLast('/');
            if (lastSlash != -1)
                assetRef = assetRef.Substring(lastSlash + 1);
            assetRef = "local://" + assetRef;

            bool exists = fileSystem->FileExists(file);
            if (!exists)
            {
                /// \todo Currently it seems that we do not get delete notifications at all
                // Tracked file was not found from the list of tracked files and it doesn't exist so 
                // it must be deleted (info about new files is retrieved by directoryChanged signal).
                LogInfo("Watched file " + file + " was deleted.");
                storage->EmitAssetChanged(file, IAssetStorage::AssetDelete);
            }
            else
            {
                // File was tracked and found from watched files: must've been modified.
                bool existing = framework->Asset()->FindAsset(assetRef) != nullptr;
                if (existing)
                {
                    LogInfo("Watched file " + file + " was modified. Asset ref: " + assetRef);
                    storage->EmitAssetChanged(file, IAssetStorage::AssetModify);
                }
                else
                {
                    LogInfo("New file " + file + " added to storage " + storage->ToString());
                    storage->EmitAssetChanged(file, IAssetStorage::AssetCreate);
                }
            }
        }
    }
}

}

