// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"
#include "CoreTypes.h"
#include "AssetFwd.h"

#include <Urho3D/Core/Object.h>

namespace Tundra
{

/// Implements a disk cache for asset files to avoid re-downloading assets between runs.
class TUNDRACORE_API AssetCache : public Object
{
    URHO3D_OBJECT(AssetCache, Object);

public:
    explicit AssetCache(AssetAPI *owner, String assetCacheDirectory);

    /// Returns the absolute path on the local file system that contains a cached copy of the given asset ref.
    /// If the given asset file does not exist in the cache, an empty string is returned.
    /// @param assetRef The asset reference URL, which must be of type AssetRefExternalUrl.
    String FindInCache(const String &assetRef);

    /// Returns the absolute path on the local file system for the cached version of the given asset ref.
    /// This function is otherwise identical to FindInCache, except this version does not check whether the asset exists 
    /// in the cache, but simply returns the absolute path where the asset would be stored in the cache.
    /// @param assetRef The asset reference URL, which must be of type AssetRefExternalUrl.
    String DiskSourceByRef(const String &assetRef);
    
    /// Saves the given asset to cache.
    /// @return String the absolute path name to the asset cache entry. If not successful returns an empty string.
    String StoreAsset(AssetPtr asset);

    /// Saves the specified data to the asset cache.
    /// @return String the absolute path name to the asset cache entry. If not successful returns an empty string.
    String StoreAsset(const u8 *data, uint numBytes, const String &assetName);

    /// Return the last modified time for assetRefs cache file as seconds since 1.1.1970.
    /// If cache file does not exist for assetRef return invalid QDateTime. You can check return value with QDateTime::isValid().
    /// @param String assetRef Asset reference of which cache file last modified date and time will be returned.
    /// @return QDateTime Last modified date and time of the cache file.
    unsigned LastModified(const String &assetRef);

    /// Sets the last modified date and time for the assetRefs cache file.
    /// @param String assetRef Asset reference thats cache file last modified date and time will be set.
    /// @param QDateTime The date and time to set.
    /// @return bool Returns true if successful, false otherwise.
    bool SetLastModified(const String &assetRef, unsigned dateTime);

    /// Deletes the asset with the given assetRef from the cache, if it exists.
    /// @param String asset reference.
    void DeleteAsset(const String &assetRef);

    /// Deletes all data and metadata files from the asset cache.
    /// Will not clear sub folders in the cache folders, or remove any folders.
    void ClearAssetCache();

    /// Get the cache directory. Returned path is guaranteed to have a trailing slash /.
    /// @return String absolute path to the caches data directory
    String CacheDirectory() const;
    
private:
#ifdef WIN32
    /// Windows specific helper to open a file handle to absolutePath
    void *OpenFileHandle(const String &absolutePath);
#endif
    /// Cache directory, passed here from AssetAPI in the ctor.
    String cacheDirectory;

    /// AssetAPI ptr.
    AssetAPI *assetAPI;
};

}
