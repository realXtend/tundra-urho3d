// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"
#include "IAssetStorage.h"
#include "CoreStringUtils.h"

#include <map>

namespace Tundra
{

class AssetAPI;

/// Represents a single (possibly recursive) directory on the local file system.
class TUNDRACORE_API LocalAssetStorage : public IAssetStorage
{
    OBJECT(LocalAssetStorage);

public:
    /// recursive, writable, liveUpdate and autoDiscoverable all set to true by default.
    LocalAssetStorage(Urho3D::Context* context, bool writable, bool liveUpdate, bool autoDiscoverable);
    ~LocalAssetStorage();

    /// Specifies the absolute path of the storage.
    String directory;

    /// Specifies a human-readable name for this storage.
    String name;

    /// If true, all subdirectories of the storage directory are automatically looked in when loading an asset.
    bool recursive;
    
    /// Starts listening on the local directory this asset storage points to.
    void SetupWatcher();

    /// Stops and deallocates the directory change listener.
    void RemoveWatcher();

    /// Load all assets of specific suffix
    void LoadAllAssetsOfType(AssetAPI *assetAPI, const String &suffix, const String &assetType);

    ///\todo Evaluate if could be removed. Now both AssetAPI and LocalAssetStorage manage list of asset refs.
    StringVector assetRefs;

    SharedPtr<Urho3D::FileWatcher> changeWatcher;

    /// Local storages are always trusted.
    bool Trusted() const override { return true; }
    /// Local storages are always trusted.
    TrustState GetTrustState() const override { return StorageTrusted; }

    /// Returns the full local filesystem path name of the given asset in this storage, if it exists.
    /// Example: GetFullPathForAsset("my.mesh", true) might return "C:\Projects\Tundra\bin\data\assets".
    /// If the file does not exist, returns "".
    String GetFullPathForAsset(const String &assetname, bool recursive);

    /// Returns the URL that should be used in a scene asset reference attribute to refer to the asset with the given localName.
    /// Example: GetFullAssetURL("my.mesh") might return "local://my.mesh".
    /// @note LocalAssetStorage ignores all subdirectory specifications, so GetFullAssetURL("data/assets/my.mesh") would also return "local://my.mesh".
    String GetFullAssetURL(const String &localName);
    
    /// Returns the type of this storage: "LocalAssetStorage".
    virtual String Type() const;

    /// Returns all assetrefs currently known in this asset storage. Does not load the assets
    /// @deprecated Do not call this. Rather query for assets through AssetAPI.
    StringList GetAllAssetRefs() override { return assetRefs; }
    
    /// Refresh asset refs. Issues a directory query and emits AssetChanged signals immediately
    void RefreshAssetRefs() override;

    String Name() const { return name; }

    String BaseURL() const { return "local://"; }

    /// Returns a convenient human-readable representation of this storage.
    String ToString() const { return Name() + " (" + directory + ")"; }

    /// Serializes this storage to a string for machine transfer.
    String SerializeToString(bool networkTransfer = false) const override;

    /// If @c change is IAssetStorage::AssetCreate, adds file to the list of asset refs and signal
    void EmitAssetChanged(String absoluteFilename, IAssetStorage::ChangeType change);

    /// Walks through this storage on disk and creates a cached index of all the filenames inside this storage.
    void CacheStorageContents();

private:
    friend class LocalAssetProvider;

    /// Maps a file basename 'asset.mesh' to its full path 'c:\project\assets\asset.mesh'.
    /// Used to quickly lookup known assets by basename instead of having to do an expensive recursive directory search.
    std::map<String, String, StringCompareCaseInsensitive> cachedFiles;
};

}

